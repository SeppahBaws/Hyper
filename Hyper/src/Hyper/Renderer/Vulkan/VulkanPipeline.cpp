#include "HyperPCH.h"
#include "VulkanPipeline.h"

#include "Vertex.h"
#include "VulkanDebug.h"
#include "VulkanShader.h"
#include "VulkanUtility.h"
#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	VulkanPipeline::VulkanPipeline(RenderContext* pRenderCtx, const vk::Pipeline& pipeline,
		const vk::PipelineCache& cache, const vk::PipelineLayout& layout)
		: m_pRenderCtx(pRenderCtx)
		, m_Pipeline(pipeline)
		, m_Cache(cache)
		, m_Layout(layout)
	{
	}

	VulkanPipeline::~VulkanPipeline()
	{
		if (m_Pipeline)
			m_pRenderCtx->device.destroyPipeline(m_Pipeline);
		if (m_Cache)
			m_pRenderCtx->device.destroyPipelineCache(m_Cache);
		if (m_Layout)
			m_pRenderCtx->device.destroyPipelineLayout(m_Layout);
	}

	VulkanPipeline::VulkanPipeline(VulkanPipeline&& other) noexcept
		: m_pRenderCtx(other.m_pRenderCtx)
		, m_Pipeline(other.m_Pipeline)
		, m_Cache(other.m_Cache)
		, m_Layout(other.m_Layout)
	{
		// Invalidate other object
		other.m_pRenderCtx = nullptr;
		other.m_Pipeline = nullptr;
		other.m_Cache = nullptr;
		other.m_Layout = nullptr;
	}

	VulkanPipeline& VulkanPipeline::operator=(VulkanPipeline&& other) noexcept
	{
		// Invalidate other object
		m_pRenderCtx = other.m_pRenderCtx;
		m_Pipeline = other.m_Pipeline;
		m_Cache = other.m_Cache;
		m_Layout = other.m_Layout;

		other.m_pRenderCtx = nullptr;
		other.m_Pipeline = nullptr;
		other.m_Cache = nullptr;
		other.m_Layout = nullptr;

		return *this;
	}

	PipelineBuilder::PipelineBuilder(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
	}

	PipelineBuilder& PipelineBuilder::SetDebugName(const std::string& debugName)
	{
		m_DebugName = debugName;

		return *this;
	}

	PipelineBuilder& PipelineBuilder::SetShader(VulkanShader* pShader)
	{
		m_pShader = pShader;

		return *this;
	}

	PipelineBuilder& PipelineBuilder::SetInputAssembly(vk::PrimitiveTopology topology, bool primitiveRestartEnable)
	{
		m_InputAssembly = vk::PipelineInputAssemblyStateCreateInfo{
			{},
			topology,
			primitiveRestartEnable
		};

		return *this;
	}

	PipelineBuilder& PipelineBuilder::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
	{
		m_Viewport = vk::Viewport{
			x, y,
			width, height,
			minDepth, maxDepth
		};

		return *this;
	}

	PipelineBuilder& PipelineBuilder::SetScissor(const vk::Offset2D& offset, const vk::Extent2D& extent)
	{
		m_Scissor = vk::Rect2D{
			offset,
			extent
		};

		return *this;
	}

	PipelineBuilder& PipelineBuilder::SetRasterizer(vk::PolygonMode polygonMode, vk::CullModeFlagBits cullMode, vk::FrontFace frontFace)
	{
		m_Rasterizer = vk::PipelineRasterizationStateCreateInfo{};
		m_Rasterizer.depthClampEnable = false;
		m_Rasterizer.rasterizerDiscardEnable = false;
		m_Rasterizer.polygonMode = polygonMode;
		m_Rasterizer.lineWidth = 1.0f;
		m_Rasterizer.cullMode = cullMode;
		m_Rasterizer.frontFace = frontFace;
		m_Rasterizer.depthBiasEnable = false;
		m_Rasterizer.depthBiasConstantFactor = 0.0f;
		m_Rasterizer.depthBiasClamp = 0.0f;
		m_Rasterizer.depthBiasSlopeFactor = 0.0f;

		return *this;
	}

	PipelineBuilder& PipelineBuilder::SetMultisampling()
	{
		m_Multisampling = vk::PipelineMultisampleStateCreateInfo{};
		m_Multisampling.sampleShadingEnable = false;
		m_Multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
		m_Multisampling.minSampleShading = 1.0f;
		m_Multisampling.pSampleMask = nullptr;
		m_Multisampling.alphaToCoverageEnable = false;
		m_Multisampling.alphaToOneEnable = false;

		return *this;
	}

	PipelineBuilder& PipelineBuilder::SetDepthStencil(bool depthTest, bool depthWrite, vk::CompareOp compareOp)
	{
		m_DepthStencil = vk::PipelineDepthStencilStateCreateInfo{};
		m_DepthStencil.depthTestEnable = depthTest;
		m_DepthStencil.depthWriteEnable = depthWrite;
		m_DepthStencil.depthCompareOp = depthTest ? compareOp : vk::CompareOp::eAlways;
		m_DepthStencil.depthBoundsTestEnable = false;
		m_DepthStencil.minDepthBounds = 0.0f;
		m_DepthStencil.maxDepthBounds = 1.0f;
		m_DepthStencil.stencilTestEnable = false;

		return *this;
	}

	PipelineBuilder& PipelineBuilder::SetColorBlend(bool blendEnable, vk::BlendOp colorBlendOp, vk::BlendOp alphaBlendOp,
		bool logicOpEnable, vk::LogicOp logicOp)
	{
		m_ColorBlendAttachment = vk::PipelineColorBlendAttachmentState{};
		m_ColorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA;
		m_ColorBlendAttachment.blendEnable = blendEnable;
		m_ColorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		m_ColorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		m_ColorBlendAttachment.colorBlendOp = colorBlendOp;
		m_ColorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
		m_ColorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		m_ColorBlendAttachment.alphaBlendOp = alphaBlendOp;

		m_ColorBlending = vk::PipelineColorBlendStateCreateInfo{};
		m_ColorBlending.logicOpEnable = logicOpEnable;
		m_ColorBlending.logicOp = logicOp;
		m_ColorBlending.setAttachments(m_ColorBlendAttachment);
		m_ColorBlending.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

		return *this;
	}

	PipelineBuilder& PipelineBuilder::SetDescriptorSetLayout(const std::vector<vk::DescriptorSetLayout>& layouts,
		const std::vector<vk::PushConstantRange>& pushConstants)
	{
		m_PipelineLayoutInfo = vk::PipelineLayoutCreateInfo{};
		m_PipelineLayoutInfo.setLayoutCount = static_cast<u32>(layouts.size());
		m_PipelineLayoutInfo.pSetLayouts = layouts.data();

		m_PipelineLayoutInfo.pushConstantRangeCount = static_cast<u32>(pushConstants.size());
		m_PipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

		return *this;
	}

	PipelineBuilder& PipelineBuilder::SetDynamicStates(const std::vector<vk::DynamicState>& dynamicStates)
	{
		m_DynamicStates = dynamicStates;

		return *this;
	}

	PipelineBuilder& PipelineBuilder::SetFormats(const std::vector<vk::Format>& colorFormats, vk::Format depthStencilFormat)
	{
		m_ColorFormats = colorFormats;
		m_DepthStencilFormat = depthStencilFormat;

		return *this;
	}

	PipelineBuilder& PipelineBuilder::SetRayTracingShaderGroups(const std::vector<vk::RayTracingShaderGroupCreateInfoKHR>& shaderGroupCreateInfos)
	{
		m_RayTracingShaderGroups = shaderGroupCreateInfos;

		return *this;
	}

	PipelineBuilder& PipelineBuilder::SetMaxRayRecursionDepth(u32 maxRayRecursionDepth)
	{
		m_MaxRayRecursionDepth = maxRayRecursionDepth;

		return *this;
	}

	VulkanPipeline PipelineBuilder::BuildGraphics(vk::PipelineCreateFlags flags)
	{
		static auto containsDepth = [](vk::Format format)
		{
			switch (format)
			{
			case vk::Format::eD16UnormS8Uint:
			case vk::Format::eX8D24UnormPack32:
			case vk::Format::eD32Sfloat:
			case vk::Format::eD16Unorm:
			case vk::Format::eD24UnormS8Uint:
			case vk::Format::eD32SfloatS8Uint:
				return true;
			default:
				return false;
			}
		};

		static auto containsStencil = [](vk::Format format)
		{
			switch (format)
			{
			case vk::Format::eS8Uint:
			case vk::Format::eD16UnormS8Uint:
			case vk::Format::eD24UnormS8Uint:
			case vk::Format::eD32SfloatS8Uint:
				return true;
			default:
				return false;
			}
		};

		auto bindingDescription = VertexPosNormTex::GetBindingDescription();
		auto attributeDescriptions = VertexPosNormTex::GetAttributeDescriptions();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.setVertexBindingDescriptions(bindingDescription);
		vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);

		vk::PipelineViewportStateCreateInfo viewportState{};
		viewportState.setViewports(m_Viewport);
		viewportState.setScissors(m_Scissor);

		// First, create the pipeline layout
		vk::PipelineLayout layout = VulkanUtils::Check(m_pRenderCtx->device.createPipelineLayout(m_PipelineLayoutInfo));

		vk::PipelineRenderingCreateInfo pipelineRendering{};
		pipelineRendering.setColorAttachmentFormats(m_ColorFormats);
		// TODO: get these from the swap chain
		if (containsDepth(m_DepthStencilFormat))
		{
			pipelineRendering.depthAttachmentFormat = m_DepthStencilFormat;
		}
		if (containsStencil(m_DepthStencilFormat))
		{
			pipelineRendering.stencilAttachmentFormat = m_DepthStencilFormat;
		}

		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = m_pShader->GetAllShaderStages();
		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.pNext = &pipelineRendering;
		pipelineInfo.flags = flags;
		pipelineInfo.setStages(shaderStages);

		vk::PipelineDynamicStateCreateInfo dynamicStateInfo{};
		dynamicStateInfo.setDynamicStates(m_DynamicStates);

		if (!m_DynamicStates.empty())
		{
			pipelineInfo.setPDynamicState(&dynamicStateInfo);
		}
		else
		{
			pipelineInfo.pDynamicState = nullptr;
		}

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &m_InputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &m_Rasterizer;
		pipelineInfo.pMultisampleState = &m_Multisampling;
		pipelineInfo.pDepthStencilState = &m_DepthStencil;
		pipelineInfo.pColorBlendState = &m_ColorBlending;

		pipelineInfo.layout = layout;
		pipelineInfo.renderPass = nullptr;
		pipelineInfo.subpass = 0;

		const vk::PipelineCache cache = VulkanUtils::Check(m_pRenderCtx->device.createPipelineCache(vk::PipelineCacheCreateInfo{}));
		const vk::ResultValue<vk::Pipeline> pipelineResult = m_pRenderCtx->device.createGraphicsPipeline(cache, pipelineInfo);

		if (pipelineResult.result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create graphics pipeline");
		}

		if (!m_DebugName.empty())
		{
			VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::ePipeline, pipelineResult.value, m_DebugName);
		}

		return VulkanPipeline(m_pRenderCtx, pipelineResult.value, cache, layout);
	}

	VulkanPipeline PipelineBuilder::BuildRaytracing(vk::PipelineCreateFlags flags)
	{
		// First, create the pipeline layout
		vk::PipelineLayout layout = VulkanUtils::Check(m_pRenderCtx->device.createPipelineLayout(m_PipelineLayoutInfo));

		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = m_pShader->GetAllShaderStages();

		vk::RayTracingPipelineCreateInfoKHR pipelineInfo{};
		pipelineInfo.flags = flags;

		vk::PipelineDynamicStateCreateInfo dynamicStateInfo{};
		dynamicStateInfo.setDynamicStates(m_DynamicStates);

		if (!m_DynamicStates.empty())
		{
			pipelineInfo.setPDynamicState(&dynamicStateInfo);
		}
		else
		{
			pipelineInfo.pDynamicState = nullptr;
		}

		pipelineInfo.setStages(shaderStages);
		pipelineInfo.setGroups(m_RayTracingShaderGroups);
		pipelineInfo.maxPipelineRayRecursionDepth = m_MaxRayRecursionDepth;
		pipelineInfo.layout = layout;

		const vk::PipelineCache cache = VulkanUtils::Check(m_pRenderCtx->device.createPipelineCache(vk::PipelineCacheCreateInfo{}));
		const vk::ResultValue<vk::Pipeline> pipelineResult = m_pRenderCtx->device.createRayTracingPipelineKHR({}, cache, pipelineInfo);

		if (pipelineResult.result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create raytracing pipeline");
		}

		if (!m_DebugName.empty())
		{
			VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::ePipeline, pipelineResult.value, m_DebugName);
		}

		return VulkanPipeline(m_pRenderCtx, pipelineResult.value, cache, layout);
	}
}
