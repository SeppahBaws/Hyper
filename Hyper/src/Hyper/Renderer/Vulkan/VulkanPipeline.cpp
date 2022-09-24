#include "HyperPCH.h"
#include "VulkanPipeline.h"

#include "Vertex.h"
#include "VulkanDebug.h"
#include "VulkanShader.h"
#include "VulkanUtility.h"
#include "Hyper/Renderer/RenderContext.h"
#include "Hyper/Renderer/ShaderLibrary.h"

namespace Hyper
{
	VulkanGraphicsPipeline::VulkanGraphicsPipeline(RenderContext* pRenderCtx, const GraphicsPipelineSpecification& spec)
		: m_pRenderCtx(pRenderCtx), m_Specification(spec)
	{
		Create();
	}

	VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
	{
		Destroy();
	}

	VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other) noexcept
		: m_pRenderCtx(other.m_pRenderCtx)
		, m_Specification(other.m_Specification)
		, m_Pipeline(other.m_Pipeline)
		, m_Cache(other.m_Cache)
		, m_Layout(other.m_Layout)
	{
		// Invalidate other's data
		other.m_Pipeline = nullptr;
		other.m_Cache = nullptr;
		other.m_Layout = nullptr;
	}

	VulkanGraphicsPipeline& VulkanGraphicsPipeline::operator=(VulkanGraphicsPipeline&& other) noexcept
	{
		m_pRenderCtx = other.m_pRenderCtx;
		m_Specification = other.m_Specification;
		m_Pipeline = other.m_Pipeline;
		m_Cache = other.m_Cache;
		m_Layout = other.m_Layout;

		// Invalidate other's data
		other.m_Pipeline = nullptr;
		other.m_Cache = nullptr;
		other.m_Layout = nullptr;

		return *this;
	}

	void VulkanGraphicsPipeline::Recreate()
	{
		VulkanUtils::CheckResult(m_pRenderCtx->device.waitIdle());

		// TODO: handle pipeline creation errors
		Destroy();
		Create();
	}

	void VulkanGraphicsPipeline::Create()
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
		viewportState.setViewports(m_Specification.viewport);
		viewportState.setScissors(m_Specification.scissor);

		// First, create the pipeline layout
		std::vector descriptorSetLayouts = m_Specification.pShader->GetAllDescriptorSetLayouts();
		std::vector pushConstants = m_Specification.pShader->GetAllPushConstantRanges();

		vk::PipelineLayoutCreateInfo layoutCreateInfo{};
		// layoutCreateInfo.setLayoutCount = static_cast<u32>(descriptorSetLayouts.size());
		// layoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
		// layoutCreateInfo.pushConstantRangeCount = static_cast<u32>(pushConstants.size());
		// layoutCreateInfo.pPushConstantRanges = pushConstants.data();
		layoutCreateInfo.setSetLayouts(descriptorSetLayouts);
		layoutCreateInfo.setPushConstantRanges(pushConstants);
		m_Layout = VulkanUtils::Check(m_pRenderCtx->device.createPipelineLayout(layoutCreateInfo));

		vk::PipelineRenderingCreateInfo pipelineRendering{};
		pipelineRendering.setColorAttachmentFormats(m_Specification.colorFormats);
		// TODO: get these from the swap chain
		if (containsDepth(m_Specification.depthStencilFormat))
		{
			pipelineRendering.depthAttachmentFormat = m_Specification.depthStencilFormat;
		}
		if (containsStencil(m_Specification.depthStencilFormat))
		{
			pipelineRendering.stencilAttachmentFormat = m_Specification.depthStencilFormat;
		}

		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = m_Specification.pShader->GetAllShaderStages();
		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.pNext = &pipelineRendering;
		pipelineInfo.flags = m_Specification.flags;
		pipelineInfo.setStages(shaderStages);

		vk::PipelineDynamicStateCreateInfo dynamicStateInfo{};
		dynamicStateInfo.setDynamicStates(m_Specification.dynamicStates);

		if (!m_Specification.dynamicStates.empty())
		{
			pipelineInfo.setPDynamicState(&dynamicStateInfo);
		}
		else
		{
			pipelineInfo.pDynamicState = nullptr;
		}

		auto inputAssemblyState = vk::PipelineInputAssemblyStateCreateInfo{};
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
		inputAssemblyState.primitiveRestartEnable = false;

		auto rasterizerState = vk::PipelineRasterizationStateCreateInfo{};
		rasterizerState.depthClampEnable = false;
		rasterizerState.rasterizerDiscardEnable = false;
		rasterizerState.polygonMode = m_Specification.polygonMode;
		rasterizerState.lineWidth = 1.0f;
		rasterizerState.cullMode = m_Specification.cullMode;
		rasterizerState.frontFace = m_Specification.frontFace;
		rasterizerState.depthBiasEnable = false;
		rasterizerState.depthBiasConstantFactor = 0.0f;
		rasterizerState.depthBiasClamp = 0.0f;
		rasterizerState.depthBiasSlopeFactor = 0.0f;

		auto multisamplingState = vk::PipelineMultisampleStateCreateInfo{};
		multisamplingState.sampleShadingEnable = false;
		multisamplingState.rasterizationSamples = vk::SampleCountFlagBits::e1;
		multisamplingState.minSampleShading = 1.0f;
		multisamplingState.pSampleMask = nullptr;
		multisamplingState.alphaToCoverageEnable = false;
		multisamplingState.alphaToOneEnable = false;

		auto depthStencilState = vk::PipelineDepthStencilStateCreateInfo{};
		depthStencilState.depthTestEnable = m_Specification.depthTest;
		depthStencilState.depthWriteEnable = m_Specification.depthWrite;
		depthStencilState.depthCompareOp = m_Specification.depthTest ? m_Specification.compareOp : vk::CompareOp::eAlways;
		depthStencilState.depthBoundsTestEnable = false;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = false;

		auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState{};
		colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA;
		colorBlendAttachment.blendEnable = m_Specification.blendEnable;
		colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
		colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
		colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

		auto colorBlendingState = vk::PipelineColorBlendStateCreateInfo{};
		colorBlendingState.logicOpEnable = false;
		colorBlendingState.logicOp = vk::LogicOp::eCopy;
		colorBlendingState.setAttachments(colorBlendAttachment);
		colorBlendingState.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizerState;
		pipelineInfo.pMultisampleState = &multisamplingState;
		pipelineInfo.pDepthStencilState = &depthStencilState;
		pipelineInfo.pColorBlendState = &colorBlendingState;

		pipelineInfo.layout = m_Layout;
		pipelineInfo.renderPass = nullptr;
		pipelineInfo.subpass = 0;

		m_Cache = VulkanUtils::Check(m_pRenderCtx->device.createPipelineCache(vk::PipelineCacheCreateInfo{}));
		const vk::ResultValue<vk::Pipeline> pipelineResult = m_pRenderCtx->device.createGraphicsPipeline(m_Cache, pipelineInfo);

		if (pipelineResult.result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create graphics pipeline");
		}
		m_Pipeline = pipelineResult.value;

		if (!m_Specification.debugName.empty())
		{
			VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::ePipeline, pipelineResult.value, m_Specification.debugName);
			VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::ePipelineLayout, m_Layout, fmt::format("{} layout", m_Specification.debugName));
		}

		// Register shader dependency
		m_pRenderCtx->pShaderLibrary->RegisterShaderDependency(m_Specification.pShader->GetId(), this);
	}

	void VulkanGraphicsPipeline::Destroy()
	{
		if (m_Pipeline)
			m_pRenderCtx->device.destroyPipeline(m_Pipeline);
		if (m_Cache)
			m_pRenderCtx->device.destroyPipelineCache(m_Cache);
		if (m_Layout)
			m_pRenderCtx->device.destroyPipelineLayout(m_Layout);

		// Un-register shader dependency
		m_pRenderCtx->pShaderLibrary->UnRegisterShaderDependency(m_Specification.pShader->GetId(), this);
	}

	VulkanRayTracingPipeline::VulkanRayTracingPipeline(RenderContext* pRenderCtx, const RayTracingPipelineSpecification& spec)
		: m_pRenderCtx(pRenderCtx), m_Specification(spec)
	{
		Create();
	}

	VulkanRayTracingPipeline::~VulkanRayTracingPipeline()
	{
		Destroy();
	}

	VulkanRayTracingPipeline::VulkanRayTracingPipeline(VulkanRayTracingPipeline&& other) noexcept
		: m_pRenderCtx(other.m_pRenderCtx)
		, m_Specification(other.m_Specification)
		, m_Pipeline(other.m_Pipeline)
		, m_Cache(other.m_Cache)
		, m_Layout(other.m_Layout)
	{
		// Invalidate other's data
		other.m_Pipeline = nullptr;
		other.m_Cache = nullptr;
		other.m_Layout = nullptr;
	}

	VulkanRayTracingPipeline& VulkanRayTracingPipeline::operator=(VulkanRayTracingPipeline&& other) noexcept
	{
		m_pRenderCtx = other.m_pRenderCtx;
		m_Specification = other.m_Specification;
		m_Pipeline = other.m_Pipeline;
		m_Cache = other.m_Cache;
		m_Layout = other.m_Layout;

		// Invalidate other's data
		other.m_Pipeline = nullptr;
		other.m_Cache = nullptr;
		other.m_Layout = nullptr;

		return *this;
	}

	void VulkanRayTracingPipeline::Recreate()
	{
		VulkanUtils::CheckResult(m_pRenderCtx->device.waitIdle());

		// TODO: handle pipeline creation errors.
		Destroy();
		Create();
	}

	void VulkanRayTracingPipeline::Create()
	{
		// First, create the pipeline layout
		vk::PipelineLayoutCreateInfo layoutCreateInfo{};
		layoutCreateInfo.setLayoutCount = static_cast<u32>(m_Specification.layouts.size());
		layoutCreateInfo.pSetLayouts = m_Specification.layouts.data();
		layoutCreateInfo.pushConstantRangeCount = static_cast<u32>(m_Specification.pushConstants.size());
		layoutCreateInfo.pPushConstantRanges = m_Specification.pushConstants.data();
		m_Layout = VulkanUtils::Check(m_pRenderCtx->device.createPipelineLayout(layoutCreateInfo));

		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = m_Specification.pShader->GetAllShaderStages();

		vk::RayTracingPipelineCreateInfoKHR pipelineInfo{};
		pipelineInfo.flags = m_Specification.flags;

		vk::PipelineDynamicStateCreateInfo dynamicStateInfo{};
		dynamicStateInfo.setDynamicStates(m_Specification.dynamicStates);

		if (!m_Specification.dynamicStates.empty())
		{
			pipelineInfo.setPDynamicState(&dynamicStateInfo);
		}
		else
		{
			pipelineInfo.pDynamicState = nullptr;
		}

		pipelineInfo.setStages(shaderStages);
		pipelineInfo.setGroups(m_Specification.shaderGroupCreateInfos);
		pipelineInfo.maxPipelineRayRecursionDepth = m_Specification.rayRecursionDepth;
		pipelineInfo.layout = m_Layout;

		m_Cache = VulkanUtils::Check(m_pRenderCtx->device.createPipelineCache(vk::PipelineCacheCreateInfo{}));
		const vk::ResultValue<vk::Pipeline> pipelineResult = m_pRenderCtx->device.createRayTracingPipelineKHR({}, m_Cache, pipelineInfo);

		if (pipelineResult.result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to create raytracing pipeline");
		}
		m_Pipeline = pipelineResult.value;

		if (!m_Specification.debugName.empty())
		{
			VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::ePipeline, pipelineResult.value, m_Specification.debugName);
			VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::ePipelineLayout, m_Layout, fmt::format("{} layout", m_Specification.debugName));
		}

		// Register shader dependency
		m_pRenderCtx->pShaderLibrary->RegisterShaderDependency(m_Specification.pShader->GetId(), this);
	}

	void VulkanRayTracingPipeline::Destroy()
	{
		if (m_Pipeline)
			m_pRenderCtx->device.destroyPipeline(m_Pipeline);
		if (m_Cache)
			m_pRenderCtx->device.destroyPipelineCache(m_Cache);
		if (m_Layout)
			m_pRenderCtx->device.destroyPipelineLayout(m_Layout);

		// Un-register shader dependency
		m_pRenderCtx->pShaderLibrary->UnRegisterShaderDependency(m_Specification.pShader->GetId(), this);
	}
}
