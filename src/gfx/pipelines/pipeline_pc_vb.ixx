module;

export module vkl_gfx:pipeline_pc_vb;

import std;
import vkl_platform;

export namespace vkl::gfx
{
	class pipeline_pc_vb final
	{
	public:
		struct description
		{
			struct shader_binary
			{
				std::span<const uint8_t> vs;
				std::span<const uint8_t> fs;
			};
			struct vertex_description
			{
				std::span<const vk::VertexInputAttributeDescription> attributes;
				std::span<const vk::VertexInputBindingDescription> bindings;
			};

			shader_binary shaders;
			vertex_description vertex_input;
			vk::PrimitiveTopology topology;
			vk::PolygonMode polygon_mode;
			vk::CullModeFlags cull_mode;
			vk::FrontFace front_face;
			std::span<const vk::Format> color_formats;
			vk::Format depth_format;
			vk::PushConstantRange pc_rng;
		};

		pipeline_pc_vb(vk::Device device, const description &desc);
		~pipeline_pc_vb();

		[[nodiscard]] auto get_pipeline() const -> vk::Pipeline;
		[[nodiscard]] auto get_layout() const -> vk::PipelineLayout;

	private:
		[[nodiscard]] auto create_shader_module(const std::span<const uint8_t> shader_bin) -> vk::ShaderModule;

	private:
		vk::Device device;

		vk::PipelineLayout layout;
		vk::Pipeline pl;
	};
}

using namespace vkl;
using namespace vkl::gfx;
namespace rg = std::ranges;

namespace
{
	bool is_depth_only_format(vk::Format format)
	{
		return format == vk::Format::eD16Unorm ||
		       format == vk::Format::eD32Sfloat;
	}

	bool is_depth_stencil_format(vk::Format format)
	{
		return format == vk::Format::eD16UnormS8Uint ||
		       format == vk::Format::eD24UnormS8Uint ||
		       format == vk::Format::eD32SfloatS8Uint;
	}
}

pipeline_pc_vb::pipeline_pc_vb(vk::Device device_, const description &desc)
	: device(device_)
{
	// Assume desc::shaders will always have vertex and fragment shaders

	// Convert shader binary into shader modules
	using shader_stage_module = std::tuple<vk::ShaderStageFlagBits, vk::ShaderModule>;
	auto shaders              = std::vector<shader_stage_module>{
        { vk::ShaderStageFlagBits::eVertex, create_shader_module(desc.shaders.vs) },
        { vk::ShaderStageFlagBits::eFragment, create_shader_module(desc.shaders.fs) },
	};

	// Shader Stages
	// Assume all shaders will have main function as entry point
	auto shader_stage_infos = std::vector<vk::PipelineShaderStageCreateInfo>{};
	rg::transform(shaders,
	              std::back_inserter(shader_stage_infos),
	              [](const shader_stage_module &stg_module) {
		return vk::PipelineShaderStageCreateInfo{
			.stage  = std::get<vk::ShaderStageFlagBits>(stg_module),
			.module = std::get<vk::ShaderModule>(stg_module),
			.pName  = "main"
		};
	});

	// Vertex Input
	auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo{
		.vertexBindingDescriptionCount   = static_cast<uint32_t>(desc.vertex_input.bindings.size()),
		.pVertexBindingDescriptions      = desc.vertex_input.bindings.data(),
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(desc.vertex_input.attributes.size()),
		.pVertexAttributeDescriptions    = desc.vertex_input.attributes.data(),
	};

	// Input Assembly
	auto input_assembly_info = vk::PipelineInputAssemblyStateCreateInfo{
		.topology               = desc.topology,
		.primitiveRestartEnable = false,
	};

	// Viewport
	auto viewport_info = vk::PipelineViewportStateCreateInfo{
		.viewportCount = 1,
		.scissorCount  = 1,
	};

	// Rasterization
	auto rasterization_info = vk::PipelineRasterizationStateCreateInfo{
		.depthClampEnable        = false,
		.rasterizerDiscardEnable = false,
		.polygonMode             = desc.polygon_mode,
		.cullMode                = desc.cull_mode,
		.frontFace               = desc.front_face,
		.depthBiasEnable         = false,
		.lineWidth               = 1.0f,
	};

	// Multisample anti-aliasing
	auto multisample_info = vk::PipelineMultisampleStateCreateInfo{
		.rasterizationSamples = vk::SampleCountFlagBits::e1, // should this be higher for higher msaa?
		.sampleShadingEnable  = false,
	};

	// Color Blend Attachment
	auto color_blend_attach_st = vk::PipelineColorBlendAttachmentState{
		.blendEnable    = false,
		.colorWriteMask = vk::ColorComponentFlagBits::eR |
		                  vk::ColorComponentFlagBits::eG |
		                  vk::ColorComponentFlagBits::eB |
		                  vk::ColorComponentFlagBits::eA,
	};

	// Color Blend State
	auto color_blend_info = vk::PipelineColorBlendStateCreateInfo{
		.logicOpEnable   = false,
		.logicOp         = vk::LogicOp::eCopy,
		.attachmentCount = 1,
		.pAttachments    = &color_blend_attach_st,
		.blendConstants  = std::array{ 0.f, 0.f, 0.f, 0.f },
	};

	// Dynamic States
	auto dynamic_states = std::vector{
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};

	auto dynamic_state_info = vk::PipelineDynamicStateCreateInfo{
		.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
		.pDynamicStates    = dynamic_states.data(),
	};

	// Pipeline Layout
	auto pipeline_layout_info = vk::PipelineLayoutCreateInfo{
		.setLayoutCount         = 0,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges    = &desc.pc_rng,
	};
	layout = device.createPipelineLayout(pipeline_layout_info);

	auto pipeline_rendering_info = vk::PipelineRenderingCreateInfo{
		.colorAttachmentCount    = static_cast<uint32_t>(desc.color_formats.size()),
		.pColorAttachmentFormats = desc.color_formats.data(),
		.depthAttachmentFormat   = desc.depth_format,
	};
	if (not is_depth_only_format(desc.depth_format))
	{
		pipeline_rendering_info.stencilAttachmentFormat = desc.depth_format;
	}

	// Finally create Pipeline
	auto pipeline_info = vk::GraphicsPipelineCreateInfo{
		.pNext               = &pipeline_rendering_info,
		.stageCount          = static_cast<uint32_t>(shader_stage_infos.size()),
		.pStages             = shader_stage_infos.data(),
		.pVertexInputState   = &vertex_input_info,
		.pInputAssemblyState = &input_assembly_info,
		.pViewportState      = &viewport_info,
		.pRasterizationState = &rasterization_info,
		.pMultisampleState   = &multisample_info,
		.pColorBlendState    = &color_blend_info,
		.pDynamicState       = &dynamic_state_info,
		.layout              = layout,
		.subpass             = 0,
	};

	auto result_value = device.createGraphicsPipeline(nullptr, pipeline_info);
	assert(result_value.result == vk::Result::eSuccess);
	pl = result_value.value;

	// Destroy the shader modules
	for (auto &&[stg, mod] : shaders)
	{
		device.destroyShaderModule(mod);
	}

	std::println("{}Pipeline created.{}",
	             CLR::GRN, CLR::RESET);
}

pipeline_pc_vb::~pipeline_pc_vb()
{
	device.waitIdle();
	device.destroyPipelineLayout(layout);
	device.destroyPipeline(pl);
}

auto pipeline_pc_vb::get_pipeline() const -> vk::Pipeline
{
	return pl;
}

auto pipeline_pc_vb::get_layout() const -> vk::PipelineLayout
{
	return layout;
}

auto pipeline_pc_vb::create_shader_module(const std::span<const uint8_t> shader_bin) -> vk::ShaderModule
{
	auto shader_info = vk::ShaderModuleCreateInfo{
		.codeSize = shader_bin.size(),
		.pCode    = reinterpret_cast<const uint32_t *>(shader_bin.data()),
	};

	return device.createShaderModule(shader_info);
}
