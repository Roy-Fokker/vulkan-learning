module;

export module vkl_gfx:command_buffer;

import std;

export namespace vkl::gfx
{
	struct image_transition_info
	{
		vk::PipelineStageFlags src_stage_mask;
		vk::PipelineStageFlags dst_stage_mask;
		vk::AccessFlags src_access_mask;
		vk::AccessFlags dst_access_mask;
		vk::ImageLayout old_layout;
		vk::ImageLayout new_layout;
		vk::ImageSubresourceRange subresource_range;
	};

	struct command_buffer
	{
		vk::CommandBuffer cb;

		void begin();
		void end();

		void begin_rendering(const vk::RenderingInfo &rendering_info);
		void end_rendering();

		void bind(vk::PipelineBindPoint bind_point, const vk::Pipeline &pl);
		void set(std::span<vk::Viewport> viewports);
		void set(std::span<vk::Rect2D> scissors);

		void image_layout_transition(vk::Image image, const image_transition_info &transition_info);
		void image_layout_transition(vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, const vk::ImageSubresourceRange &subresource_range);

		void draw_model(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
	};
}

using namespace vkl::gfx;

namespace
{
	auto get_pipeline_stage_flags(vk::ImageLayout image_layout) -> vk::PipelineStageFlags
	{
		switch (image_layout)
		{
			using il = vk::ImageLayout;
			using pf = vk::PipelineStageFlagBits;

			case il::eUndefined:
				return pf::eTopOfPipe;
			case il::ePreinitialized:
				return pf::eHost;
			case il::eTransferSrcOptimal:
			case il::eTransferDstOptimal:
				return pf::eTransfer;
			case il::eColorAttachmentOptimal:
				return pf::eColorAttachmentOutput;
			case il::eDepthAttachmentOptimal:
				return pf::eEarlyFragmentTests | pf::eLateFragmentTests;
			case il::eFragmentShadingRateAttachmentOptimalKHR:
				return pf::eFragmentShadingRateAttachmentKHR;
			case il::eReadOnlyOptimal:
				return pf::eVertexShader | pf::eFragmentShader;
			case il::ePresentSrcKHR:
				return pf::eBottomOfPipe;
			case il::eGeneral:
				assert(false && "Don't know how to get a meaningful vk::PipelineStageFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
			default:
				assert(false && "Unknown layout flag");
		}
		return {};
	}

	auto get_access_flags(vk::ImageLayout image_layout) -> vk::AccessFlags
	{
		switch (image_layout)
		{
			using il = vk::ImageLayout;
			using af = vk::AccessFlagBits;

			case il::eUndefined:
			case il::ePresentSrcKHR:
				return af::eNone;
			case il::ePreinitialized:
				return af::eHostWrite;
			case il::eColorAttachmentOptimal:
				return af::eColorAttachmentRead | af::eColorAttachmentWrite;
			case il::eDepthAttachmentOptimal:
				return af::eDepthStencilAttachmentRead | af::eDepthStencilAttachmentWrite;
			case il::eFragmentShadingRateAttachmentOptimalKHR:
				return af::eFragmentShadingRateAttachmentReadKHR;
			case il::eShaderReadOnlyOptimal:
				return af::eShaderRead | af::eInputAttachmentRead;
			case il::eTransferSrcOptimal:
				return af::eTransferRead;
			case il::eTransferDstOptimal:
				return af::eTransferWrite;
			case il::eGeneral:
				assert(false && "Don't know how to get a meaningful vk::AccessFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
			default:
				assert(false && "Unknown layout flag");
		}
		return {};
	}
}

void command_buffer::begin()
{
	auto begin_info = vk::CommandBufferBeginInfo{};
	auto result     = cb.begin(&begin_info);

	assert(result == vk::Result::eSuccess);
}

void command_buffer::end()
{
	cb.end();
}

void command_buffer::begin_rendering(const vk::RenderingInfo &rendering_info)
{
	cb.beginRendering(rendering_info);
}

void command_buffer::end_rendering()
{
	cb.endRendering();
}

void command_buffer::bind(vk::PipelineBindPoint bind_point, const vk::Pipeline &pl)
{
	cb.bindPipeline(bind_point, pl);
}

void command_buffer::set(std::span<vk::Viewport> viewports)
{
	cb.setViewport(0, viewports);
}

void command_buffer::set(std::span<vk::Rect2D> scissors)
{
	cb.setScissor(0, scissors);
}

void command_buffer::image_layout_transition(vk::Image image, const image_transition_info &iti)
{
	auto image_memory_barrier = vk::ImageMemoryBarrier{
		.srcAccessMask       = iti.src_access_mask,
		.dstAccessMask       = iti.dst_access_mask,
		.oldLayout           = iti.old_layout,
		.newLayout           = iti.new_layout,
		.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
		.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
		.image               = image,
		.subresourceRange    = iti.subresource_range,
	};

	cb.pipelineBarrier(iti.src_stage_mask, iti.dst_stage_mask,
	                   vk::DependencyFlags{},
	                   {}, {},
	                   { image_memory_barrier });
}

void command_buffer::image_layout_transition(vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, const vk::ImageSubresourceRange &subresource_range)
{
	image_layout_transition(image,
	                        image_transition_info{
								.src_stage_mask    = get_pipeline_stage_flags(old_layout),
								.dst_stage_mask    = get_pipeline_stage_flags(new_layout),
								.src_access_mask   = get_access_flags(old_layout),
								.dst_access_mask   = get_access_flags(new_layout),
								.old_layout        = old_layout,
								.new_layout        = new_layout,
								.subresource_range = subresource_range,
							});
}

void command_buffer::draw_model(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
	cb.draw(
		vertex_count,
		instance_count,
		first_vertex,
		first_instance);
}