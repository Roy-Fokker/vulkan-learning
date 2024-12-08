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

	private:
		auto create_instance() -> vkb::Instance;
		void create_surface(const vkl::window::platform_data *internal_data);
		void pick_gpu_and_queues(vkb::Instance &vkb_inst);
		void create_gpu_memory_allocator();
		void create_swapchain();
		void create_command_pool();

		void destroy_instance();
		void destroy_surface();
		void destroy_gpu_memory_allocator();
		void destroy_swapchain();
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

		vk::CommandPool gfx_command_pool{};
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
		.bufferDeviceAddress                      = true,
	};

	auto descriptor_buffer_feature = vk::PhysicalDeviceDescriptorBufferFeaturesEXT{
		.descriptorBuffer                = true,
		.descriptorBufferPushDescriptors = true,
	};

	auto phy_dev_selector = vkb::PhysicalDeviceSelector{ vkb_inst };
	auto phy_dev_ret      = phy_dev_selector
	                       .set_minimum_version(1, 3)
	                       .set_required_features_13(features1_3)
	                       .set_required_features_12(features1_2)
	                       .add_required_extension(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME)
	                       .add_required_extension_features(descriptor_buffer_feature)
	                       .set_surface(surface)
	                       .select()
	                       .value();

	auto res = phy_dev_ret.enable_extension_features_if_present(descriptor_buffer_feature);
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

	std::println("{}Swapchain created.{}\n"
	             "\tImage size: {} x {}\n"
	             "\tImage count: {}",
	             CLR::GRN, CLR::RESET, width, height, sc_images.size());
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
}

device::~device()
{
	logical_device.waitIdle();

	destroy_command_pool();
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

void device::destroy_command_pool()
{
	logical_device.destroyCommandPool(gfx_command_pool);
}

void device::surface_resized()
{
	std::println("{}Swapchain window resized{}", CLR::YEL, CLR::RESET);

	logical_device.waitIdle();
	destroy_swapchain();

	create_swapchain();
}