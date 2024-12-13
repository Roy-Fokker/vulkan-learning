module;

export module vkl_gfx:renderer;

import std;
import vkl_platform;
import :device;
import :command_buffer;
import :pipeline_basic;
import :pipeline_pc_vb;
import :pipeline_pc_vb_cb;
import :buffer;

export namespace vkl::gfx
{
	using vertex_description = pipeline_pc_vb_cb::description::vertex_description;

	struct push_constant
	{
		alignas(16) vk::DeviceAddress vertex_buffer_address;
	};

	class renderer final : private vkl::non_copyable
	{
	public:
		renderer() = delete;

		GFX_API renderer(vkl::window::platform_data *data);
		GFX_API ~renderer();

		GFX_API void window_resized();
		GFX_API void update(const std::array<float, 4> &clear_color);
		GFX_API void draw();

		GFX_API void add_pipeline(std::span<const uint8_t> vs, std::span<const uint8_t> fs, const vertex_description &vert_desc);
		GFX_API void add_mesh(std::span<const std::byte> vertices, std::span<const std::byte> indicies);

	private:
		std::unique_ptr<vkl::gfx::device> device{ nullptr };

		// std::unique_ptr<pipeline_basic> pl_basic;
		// std::unique_ptr<pipeline_pc_vb> pl_pc_vb;
		std::unique_ptr<pipeline_pc_vb_cb> pl_pc_vb_cb;
		std::unique_ptr<buffer> vertex_buffer;
		std::unique_ptr<buffer> index_buffer;

		push_constant pc{};
	};
}

using namespace vkl;

namespace
{
}

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

	device->get_device().waitIdle();
}

void renderer::window_resized()
{
	device->surface_resized();
}

void renderer::add_pipeline(std::span<const uint8_t> vs, std::span<const uint8_t> fs, const vertex_description &vert_desc)
{
	pl_pc_vb_cb = std::make_unique<pipeline_pc_vb_cb>(
		device->get_device(),
		pipeline_pc_vb_cb::description{
			.shaders       = { vs, fs },
			.vertex_input  = vert_desc,
			.blend_type    = color_blend_type::alphablend,
			.topology      = vk::PrimitiveTopology::eTriangleList,
			.polygon_mode  = vk::PolygonMode::eFill,
			.cull_mode     = vk::CullModeFlagBits::eFront,
			.front_face    = vk::FrontFace::eCounterClockwise,
			.color_formats = std::array{ device->get_sc_format() },
			.depth_format  = vk::Format::eUndefined,
			.pc_rng        = vk::PushConstantRange{
					   .stageFlags = vk::ShaderStageFlagBits::eVertex,
					   .offset     = 0,
					   .size       = sizeof(pc) },
		});
}

void renderer::add_mesh(std::span<const std::byte> vertices, std::span<const std::byte> indicies)
{
	using bf = vk::BufferUsageFlagBits;

	auto allocator = device->get_allocator();

	vertex_buffer = std::make_unique<buffer>(
		allocator,
		buffer::description{
			.allocation_size = static_cast<uint32_t>(vertices.size()),
			.usage           = bf::eVertexBuffer | bf::eShaderDeviceAddress,
			.memory_usage    = VMA_MEMORY_USAGE_AUTO,
		});
	vertex_buffer->copy(vertices);

	pc.vertex_buffer_address = vertex_buffer->get_gpu_address(device->get_device());

	index_buffer = std::make_unique<buffer>(
		allocator,
		buffer::description{
			.allocation_size = static_cast<uint32_t>(indicies.size()),
			.usage           = bf::eIndexBuffer,
			.memory_usage    = VMA_MEMORY_USAGE_AUTO,
		});
	index_buffer->copy(indicies);
}

void renderer::update(const std::array<float, 4> &clear_color)
{
	auto current_frame = device->current_image_index();
	current_frame      = device->next_image(current_frame);

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

	auto pl        = pl_pc_vb_cb->get_pipeline();
	auto pl_layout = pl_pc_vb_cb->get_layout();

	cb.begin_rendering(rendering_info);
	cb.set(viewports);
	cb.set(scissors);
	cb.bind(vk::PipelineBindPoint::eGraphics, pl);
	cb.bind(0, vertex_buffer->get_buffer());
	cb.bind(index_buffer->get_buffer(), vk::IndexType::eUint32);
	cb.push_constants(
		pl_layout,
		vk::ShaderStageFlagBits::eVertex,
		0,
		as_byte_span(pc));

	cb.draw_indexed_model(3, 1, 0, 0, 0);

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
