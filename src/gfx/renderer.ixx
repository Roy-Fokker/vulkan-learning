module;

export module vkl_gfx:renderer;

import std;
import vkl_platform;
import :device;
import :command_buffer;
import :pipeline_basic;

export namespace vkl::gfx
{
	class renderer final : private vkl::non_copyable
	{
	public:
		renderer() = delete;

		GFX_API renderer(vkl::window::platform_data *data);
		GFX_API ~renderer();

		GFX_API void window_resized();
		GFX_API void update(const std::array<float, 4> &clear_color);
		GFX_API void draw();

		GFX_API void add_pipeline(std::span<const uint8_t> vs, std::span<const uint8_t> fs);

	private:
		std::unique_ptr<vkl::gfx::device> device{ nullptr };

		std::vector<pipeline_basic> pipelines;
	};
}

using namespace vkl;

renderer::renderer(vkl::window::platform_data *data)
{
	std::println("{}Create renderer.{}",
	             CLR::BLU, CLR::RESET);

	device = std::make_unique<vkl::gfx::device>(data);
}

renderer::~renderer()
{
	std::println("{}Destroy renderer.{}",
	             CLR::BLU, CLR::RESET);
}

void renderer::window_resized()
{
	device->surface_resized();
}

void renderer::add_pipeline(std::span<const uint8_t> vs, std::span<const uint8_t> fs)
{
	pipelines.emplace_back(
		device->get_device(),
		pipeline_basic::description{
			.shaders       = { vs, fs },
			.topology      = vk::PrimitiveTopology::eTriangleList,
			.polygon_mode  = vk::PolygonMode::eFill,
			.cull_mode     = vk::CullModeFlagBits::eFront,
			.front_face    = vk::FrontFace::eCounterClockwise,
			.color_formats = std::array{ device->get_sc_format() },
			.depth_format  = vk::Format::eUndefined,
		});
}

void renderer::update(const std::array<float, 4> &clear_color)
{
	auto current_frame = device->current_image_index();
	current_frame      = device->next_image(current_frame);

	auto &pl = pipelines.at(0);

	auto extent      = device->get_sc_extent();
	auto clear_value = vk::ClearValue{
		.color = clear_color,
	};
	auto viewport = vk::Viewport{
		.x        = 0.0f,
		.y        = static_cast<float>(extent.height),
		.width    = static_cast<float>(extent.width),
		.height   = static_cast<float>(extent.height) * -1.f,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	auto viewports = std::array{ viewport };
	auto scissor   = vk::Rect2D{
		  .offset = { 0, 0 },
		  .extent = extent,
	};
	auto scissors = std::array{ scissor };

	auto color_range = vk::ImageSubresourceRange{
		.aspectMask     = vk::ImageAspectFlagBits::eColor,
		.baseMipLevel   = 0,
		.levelCount     = vk::RemainingMipLevels,
		.baseArrayLayer = 0,
		.layerCount     = vk::RemainingArrayLayers,
	};
	auto color_attachment = vk::RenderingAttachmentInfo{
		.imageView   = device->get_sc_view(current_frame),
		.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		.resolveMode = vk::ResolveModeFlagBits::eNone,
		.loadOp      = vk::AttachmentLoadOp::eClear,
		.storeOp     = vk::AttachmentStoreOp::eStore,
		.clearValue  = clear_value,
	};

	/* TODO: Add depth stencil image buffer.
	auto depth_range = vk::ImageSubresourceRange{
	    .aspectMask = vk::ImageAspectFlagBits::eDepth,
	};
	auto depth_attachment = vk::RenderingAttachmentInfo{
	    .imageView   = depth_image_view,
	    .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
	    .resolveMode = vk::ResolveModeFlagBits::eNone,
	    .loadOp      = vk::AttachmentLoadOp::eClear,
	    .storeOp     = vk::AttachmentStoreOp::eDontCare,
	    .clearValue  = depth_clear_value,
	};
	*/

	auto rendering_info = vk::RenderingInfo{
		.renderArea           = scissor,
		.layerCount           = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments    = &color_attachment,
		// .pDepthAttachment = &depth_attachment,
	};

	auto draw_image = device->get_sc_image(current_frame);
	auto cb         = command_buffer{ device->get_cmd_buffer(current_frame) };

	cb.begin();

	cb.image_layout_transition(
		draw_image,
		image_transition_info{
			.src_stage_mask    = vk::PipelineStageFlagBits::eColorAttachmentOutput,
			.dst_stage_mask    = vk::PipelineStageFlagBits::eColorAttachmentOutput,
			.src_access_mask   = vk::AccessFlags{},
			.dst_access_mask   = vk::AccessFlagBits::eColorAttachmentWrite,
			.old_layout        = vk::ImageLayout::eUndefined,
			.new_layout        = vk::ImageLayout::eColorAttachmentOptimal,
			.subresource_range = color_range,
		});

	/* TODO: Add depth stencil image buffer
	cb.image_layout_transition(
	    depth_image,
	    vk::ImageLayout::eUndefined,
	    vk::ImageLayout::eDepthAttachmentOptimal,
	    depth_range);
	*/

	cb.begin_rendering(rendering_info);
	cb.set(viewports);
	cb.set(scissors);
	cb.bind(vk::PipelineBindPoint::eGraphics, pl.get_pipeline());

	cb.draw_model(3, 1, 0, 0);

	cb.end_rendering();

	cb.image_layout_transition(
		draw_image,
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::ImageLayout::ePresentSrcKHR,
		color_range);

	cb.end();
}

void renderer::draw()
{
	auto current_frame = device->current_image_index();

	device->submit(current_frame);
	device->present(current_frame);

	device->next_image_index();
}
