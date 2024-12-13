module;

export module vkl_gfx:device;

import std;
import vkl_platform;

export namespace vkl::gfx
{
	struct device_queue
	{
		vk::Queue queue;
		uint32_t family;
	};

	class device final : private vkl::non_copyable
	{
	public:
		device() = delete;

		device(const vkl::window::platform_data *internal_data);
		~device();

		void surface_resized();

		auto get_sc_extent() -> vk::Extent2D;
		auto get_sc_format() -> vk::Format;
		auto get_sc_view(uint32_t index) -> vk::ImageView;
		auto get_sc_image(uint32_t index) -> vk::Image;

		auto current_image_index() -> uint32_t;
		auto next_image_index() -> uint32_t;
		auto next_image(uint32_t current_index) -> uint32_t;
		void submit(uint32_t index);
		void present(uint32_t index);

		auto get_cmd_buffer(uint32_t index) -> vk::CommandBuffer;
		auto get_device() -> vk::Device;
		auto get_allocator() -> VmaAllocator;

	private:
		auto create_instance() -> vkb::Instance;
		void create_surface(const vkl::window::platform_data *internal_data);
		void pick_gpu_and_queues(vkb::Instance &vkb_inst);
		void create_gpu_memory_allocator();
		void create_swapchain();
		void create_sync_objects();
		void create_command_pool();

		void destroy_instance();
		void destroy_surface();
		void destroy_gpu_memory_allocator();
		void destroy_swapchain();
		void destroy_sync_objects();
		void destroy_command_pool();

	private:
		vk::Instance instance{};
		vk::DebugUtilsMessengerEXT debug_messenger{};
		vk::SurfaceKHR surface{};
		vk::PhysicalDevice chosen_gpu{};
		vk::Device logical_device{};

		device_queue gfx_queue{};
		device_queue transfer_queue{};
		device_queue present_queue{};
		device_queue compute_queue{};

		VmaAllocator mem_allocator{};

		vk::SwapchainKHR swap_chain{};
		vk::Extent2D sc_extent{};
		vk::Format sc_format{};
		std::vector<vk::Image> sc_images;
		std::vector<vk::ImageView> sc_views;

		struct image_sync
		{
			vk::Semaphore available;
			vk::Semaphore rendered;
			vk::Fence in_flight;
		};
		std::vector<image_sync> image_signals;

		vk::CommandPool gfx_command_pool{};
		std::vector<vk::CommandBuffer> gfx_command_buffers;

		uint32_t current_frame   = 0;
		uint32_t max_frame_count = 0;
	};
}

using namespace vkl;
using namespace vkl::gfx;
using namespace std::string_view_literals;

namespace
{
	constexpr auto use_vulkan_validation_layers{
#ifdef _DEBUG
		true
#else
		false
#endif
	};

	auto debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                    VkDebugUtilsMessageTypeFlagsEXT messageType,
	                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	                    [[maybe_unused]] void *pUserData)
		-> vk::Bool32
	{
		auto severity = vkb::to_string_message_severity(messageSeverity);
		auto type     = vkb::to_string_message_type(messageType);

		auto msgclr = ""sv;
		switch (messageSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				msgclr = CLR::BRED;
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				msgclr = CLR::BYEL;
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				msgclr = CLR::GRN;
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				msgclr = CLR::WHT;
				break;
		}

		std::println("{}[{}: {}]: {}{}", msgclr, severity, type, pCallbackData->pMessage, CLR::RESET);

		return VK_FALSE;
	}
}

device::device(const vkl::window::platform_data *internal_data)
{
	auto vkb_inst = create_instance();
	create_surface(internal_data);
	pick_gpu_and_queues(vkb_inst);
	create_gpu_memory_allocator();

	create_swapchain();
	create_sync_objects();

	create_command_pool();
}

auto device::create_instance() -> vkb::Instance
{
	auto builder = vkb::InstanceBuilder{};

	auto vkb_ib_ret = builder.set_app_name("vklGfx")
	                      .request_validation_layers(use_vulkan_validation_layers)
	                      .set_debug_callback(debug_callback)
	                      .require_api_version(1, 3, 0)
	                      .build();
	assert(vkb_ib_ret.has_value() == true);

	auto vkb_inst = vkb_ib_ret.value();

	instance        = vkb_inst.instance;
	debug_messenger = vkb_inst.debug_messenger;

	VULKAN_HPP_DEFAULT_DISPATCHER.init();         // Init base for Dynamic Dispatch
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance); // Get all the other function pointers

	std::println("{}Instance created. \n"
	             "Debug Messenger set.{}",
	             CLR::GRN, CLR::RESET);

	return vkb_inst;
}

void device::create_surface(const vkl::window::platform_data *data)
{
#ifdef WINDOWS
	auto create_info = vk::Win32SurfaceCreateInfoKHR{
		.hinstance = GetModuleHandle(nullptr),
		.hwnd      = data->hWnd
	};

	surface = instance.createWin32SurfaceKHR(create_info);
#endif

	std::println("{}Surface created.{}",
	             CLR::GRN, CLR::RESET);
}

void device::pick_gpu_and_queues(vkb::Instance &vkb_inst)
{
	auto features1_3 = vk::PhysicalDeviceVulkan13Features{
		.synchronization2 = true,
		.dynamicRendering = true,
	};

	auto features1_2 = vk::PhysicalDeviceVulkan12Features{
		.descriptorIndexing                       = true,
		.descriptorBindingPartiallyBound          = true,
		.descriptorBindingVariableDescriptorCount = true,
		.runtimeDescriptorArray                   = true,
		.timelineSemaphore                        = true,
		.bufferDeviceAddress                      = true,
	};

	auto features = vk::PhysicalDeviceFeatures{
		.shaderInt64 = true,
	};

	auto descriptor_buffer_feature = vk::PhysicalDeviceDescriptorBufferFeaturesEXT{
		.descriptorBuffer                = true,
		.descriptorBufferPushDescriptors = true,
	};

	auto phy_dev_selector = vkb::PhysicalDeviceSelector{ vkb_inst };
	auto phy_dev_select   = phy_dev_selector
	                          .set_minimum_version(1, 3)
	                          .set_required_features_13(features1_3)
	                          .set_required_features_12(features1_2)
	                          .set_required_features(features)
	                          .add_required_extension(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME)
	                          .add_required_extension_features(descriptor_buffer_feature)
	                          .set_surface(surface)
	                          .select();
	if (not phy_dev_select.has_value())
		std::println("Return value: {}", phy_dev_select.error().message());

	assert(phy_dev_select.has_value() == true);

	auto phy_dev_ret = phy_dev_select.value();
	auto res         = phy_dev_ret.enable_extension_features_if_present(descriptor_buffer_feature);
	assert(res == true);

	auto device_builder = vkb::DeviceBuilder{ phy_dev_ret };
	auto vkb_device     = device_builder.build().value();

	chosen_gpu     = phy_dev_ret.physical_device;
	logical_device = vkb_device.device;

	VULKAN_HPP_DEFAULT_DISPATCHER.init(logical_device); // get device specific function pointers

	gfx_queue.queue  = vkb_device.get_queue(vkb::QueueType::graphics).value();
	gfx_queue.family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

	transfer_queue.queue  = vkb_device.get_queue(vkb::QueueType::transfer).value();
	transfer_queue.family = vkb_device.get_queue_index(vkb::QueueType::transfer).value();

	present_queue.queue  = vkb_device.get_queue(vkb::QueueType::present).value();
	present_queue.family = vkb_device.get_queue_index(vkb::QueueType::present).value();

	compute_queue.queue = vkb_device.get_queue(vkb::QueueType::compute).value();

	auto dp = chosen_gpu.getProperties();
	std::println("{}Selected GPU: {}{}",
	             CLR::GRN, dp.deviceName.data(), CLR::RESET);

	std::println("{}Queues:{} Graphics: {}\n"
	             "\tPresent: {}\n"
	             "\tTransfer: {}\n"
	             "\tCompute: {}",
	             CLR::GRN, CLR::RESET,
	             gfx_queue.family,
	             present_queue.family,
	             transfer_queue.family,
	             compute_queue.family);
}

void device::create_gpu_memory_allocator()
{
	auto allocator_info = VmaAllocatorCreateInfo{
		.flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = chosen_gpu,
		.device         = logical_device,
		.instance       = instance,
	};

	vmaCreateAllocator(&allocator_info, &mem_allocator);

	std::println("{}GPU Memory Allocator created.{}",
	             CLR::GRN, CLR::RESET);
}

void device::create_swapchain()
{
	auto sfc_prop = chosen_gpu.getSurfaceCapabilitiesKHR(surface);
	auto width    = sfc_prop.currentExtent.width;
	auto height   = sfc_prop.currentExtent.height;

	auto sc_builder = vkb::SwapchainBuilder{ chosen_gpu, logical_device, surface };
	auto vkb_sc     = sc_builder
	                  .set_desired_format({
						  .format     = VK_FORMAT_B8G8R8A8_UNORM,
						  .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
					  })
	                  .set_desired_extent(width, height)
	                  .build()
	                  .value();

	swap_chain = vkb_sc.swapchain;
	sc_extent  = vkb_sc.extent;
	sc_format  = vk::Format{ vkb_sc.image_format };

	std::ranges::for_each(vkb_sc.get_images().value(), [&](auto &&img) {
		sc_images.push_back(img);
	});

	std::ranges::for_each(vkb_sc.get_image_views().value(), [&](auto &&img_vw) {
		sc_views.push_back(img_vw);
	});

	max_frame_count = static_cast<uint32_t>(sc_views.size());
	current_frame   = 0;

	std::println("{}Swapchain created.{}\n"
	             "\tImage size: {} x {}\n"
	             "\tImage count: {}",
	             CLR::GRN, CLR::RESET, width, height, sc_images.size());
}

void device::create_sync_objects()
{
	image_signals.resize(sc_views.size());
	for (auto &&[available, rendered, in_flight] : image_signals)
	{
		auto semaphore_info = vk::SemaphoreCreateInfo{};
		available           = logical_device.createSemaphore(semaphore_info);
		rendered            = logical_device.createSemaphore(semaphore_info);

		auto fence_info = vk::FenceCreateInfo{
			.flags = vk::FenceCreateFlagBits::eSignaled,
		};
		in_flight = logical_device.createFence(fence_info);
	}

	std::println("{}Semaphores and Fences created.{}",
	             CLR::GRN, CLR::RESET);
}

void device::create_command_pool()
{
	// Create Command Pool
	auto command_pool_info = vk::CommandPoolCreateInfo{
		.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = gfx_queue.family,
	};
	gfx_command_pool = logical_device.createCommandPool(command_pool_info);

	std::println("{}Graphics Command Pool created.{}",
	             CLR::GRN, CLR::RESET);

	// Create Command Buffer
	auto command_buffer_alloc_info = vk::CommandBufferAllocateInfo{
		.commandPool        = gfx_command_pool,
		.level              = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = static_cast<uint32_t>(sc_images.size()),
	};
	gfx_command_buffers = logical_device.allocateCommandBuffers(command_buffer_alloc_info);

	std::println("{}Command Buffers allocated.{}",
	             CLR::GRN, CLR::RESET);
}

device::~device()
{
	logical_device.waitIdle();

	destroy_command_pool();
	destroy_sync_objects();
	destroy_swapchain();
	destroy_gpu_memory_allocator();
	destroy_surface();
	destroy_instance();
}

void device::destroy_instance()
{
	logical_device.destroy();
	vkb::destroy_debug_utils_messenger(instance, debug_messenger);
	instance.destroy();
}

void device::destroy_surface()
{
	instance.destroySurfaceKHR(surface);
}

void device::destroy_gpu_memory_allocator()
{
	vmaDestroyAllocator(mem_allocator);
}

void device::destroy_swapchain()
{
	std::ranges::for_each(sc_views, [&](auto &&img_vw) {
		logical_device.destroyImageView(img_vw);
	});
	sc_views.clear();
	sc_images.clear();

	logical_device.destroySwapchainKHR(swap_chain);
}

void device::destroy_sync_objects()
{
	for (auto &&[available, rendered, in_flight] : image_signals)
	{
		logical_device.destroyFence(in_flight);
		logical_device.destroySemaphore(rendered);
		logical_device.destroySemaphore(available);
	}
	image_signals.clear();
}

void device::destroy_command_pool()
{
	gfx_command_buffers.clear();
	logical_device.destroyCommandPool(gfx_command_pool);
}

void device::surface_resized()
{
	std::println("{}Swapchain window resized{}", CLR::YEL, CLR::RESET);

	logical_device.waitIdle();
	destroy_swapchain();

	create_swapchain();
}

auto device::get_sc_extent() -> vk::Extent2D
{
	return sc_extent;
}

auto device::get_sc_format() -> vk::Format
{
	return sc_format;
}

auto device::get_sc_view(uint32_t index) -> vk::ImageView
{
	return sc_views.at(index);
}

auto device::get_sc_image(uint32_t index) -> vk::Image
{
	return sc_images.at(index);
}

auto device::current_image_index() -> uint32_t
{
	return current_frame;
}

auto device::next_image_index() -> uint32_t
{
	current_frame = (current_frame + 1) % max_frame_count;
	return current_frame;
}

auto device::next_image(uint32_t current_index) -> uint32_t
{
	constexpr auto wait_time = UINT_MAX;

	auto sync = image_signals.at(current_index);

	[[maybe_unused]] // TODO: check fence result
	auto fence_result = logical_device.waitForFences(sync.in_flight, true, wait_time);

	logical_device.resetFences(sync.in_flight);

	auto [result, image_index] = logical_device.acquireNextImageKHR(swap_chain,
	                                                                wait_time,
	                                                                sync.available,
	                                                                VK_NULL_HANDLE);
	// TODO: Validate the result value
	assert(result == vk::Result::eSuccess or      // Should normally be success
	       result == vk::Result::eSuboptimalKHR); // happens when window resizes or closes
	assert(image_index == current_frame);

	return image_index;
}

void device::submit(uint32_t index)
{
	auto sync = image_signals.at(index);
	auto cb   = get_cmd_buffer(index);

	auto wait_stage = vk::PipelineStageFlags{ vk::PipelineStageFlagBits::eColorAttachmentOutput };

	auto submit_info = vk::SubmitInfo{
		.waitSemaphoreCount   = 1,
		.pWaitSemaphores      = &sync.available,
		.pWaitDstStageMask    = &wait_stage,
		.commandBufferCount   = 1,
		.pCommandBuffers      = &cb,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores    = &sync.rendered,
	};

	gfx_queue.queue.submit({ submit_info }, sync.in_flight);
}

void device::present(uint32_t index)
{
	auto sync = image_signals.at(index);

	auto present_info = vk::PresentInfoKHR{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores    = &sync.rendered,
		.swapchainCount     = 1,
		.pSwapchains        = &swap_chain,
		.pImageIndices      = &index,
	};

	auto result = gfx_queue.queue.presentKHR(present_info);
	// TODO: validate result's other values
	assert(result == vk::Result::eSuccess or      // Should normally be success
	       result == vk::Result::eSuboptimalKHR); // happens when window resizes or closes
}

auto device::get_cmd_buffer(uint32_t index) -> vk::CommandBuffer
{
	return gfx_command_buffers.at(index);
}

auto device::get_device() -> vk::Device
{
	return logical_device;
}

auto device::get_allocator() -> VmaAllocator
{
	return mem_allocator;
}