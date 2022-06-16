#include "HyperPCH.h"
#include "VulkanRaytracer.h"

#include "VulkanAccelerationStructure.h"
#include "VulkanDebug.h"
#include "VulkanPipeline.h"
#include "VulkanUtility.h"
#include "Hyper/Renderer/FlyCamera.h"
#include "Hyper/Renderer/RenderContext.h"
#include "Hyper/Renderer/RenderTarget.h"

namespace Hyper
{
	VulkanRaytracer::VulkanRaytracer(RenderContext* pRenderCtx, vk::AccelerationStructureKHR accel, u32 numFrames)
		: m_pRenderCtx(pRenderCtx), m_Tlas(accel), m_NumFrames(numFrames)
	{
		m_pOutputImage = std::make_unique<RenderTarget>(m_pRenderCtx, "Raytracing output image", m_OutputWidth, m_OutputHeight);

		CreateDescriptorSet();
		CreatePipeline();
		CreateShaderBindingTable();
	}

	VulkanRaytracer::~VulkanRaytracer()
	{
		m_FrameDatas.clear();

		m_SbtBuffer.reset();
		m_RtPipeline.reset();
		m_pShader.reset();
		m_pOutputImage.reset();
		m_pPool.reset();
		m_pRenderCtx->device.destroyDescriptorSetLayout(m_DescLayout);
	}

	void VulkanRaytracer::RayTrace(vk::CommandBuffer cmd, FlyCamera* pCamera, u32 frameIdx)
	{
		m_CameraData.viewInv = pCamera->GetViewInverse();
		m_CameraData.projInv = pCamera->GetProjectionInverse();
		UpdateDescriptors(frameIdx);

		m_pOutputImage->GetColorImage()->TransitionLayout(cmd, vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, vk::ImageLayout::eGeneral,
			vk::PipelineStageFlagBits::eRayTracingShaderKHR);

		cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_RtPipeline->GetPipeline());
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, m_RtPipeline->GetLayout(), 0, { m_FrameDatas[frameIdx].descriptorSet }, {});
		cmd.traceRaysKHR(m_RGenRegion, m_MissRegion, m_HitRegion, m_CallRegion, m_OutputWidth, m_OutputHeight, 1);

		VkDebug::EndRegion(cmd);
	}

	void VulkanRaytracer::Resize(u32 width, u32 height)
	{
		m_OutputWidth = width;
		m_OutputHeight = height;
		m_pOutputImage->Resize(width, height);
	}

	void VulkanRaytracer::CreateDescriptorSet()
	{
		// Create descriptor set layout
		m_DescLayout = DescriptorSetLayoutBuilder(m_pRenderCtx->device)
			.AddBinding(vk::DescriptorType::eAccelerationStructureKHR, static_cast<u32>(RaytracerBindings::Acceleration), 1, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR)
			.AddBinding(vk::DescriptorType::eStorageImage, static_cast<u32>(RaytracerBindings::OutputImage), 1, vk::ShaderStageFlagBits::eRaygenKHR)
			.AddBinding(vk::DescriptorType::eUniformBuffer, static_cast<u32>(RaytracerBindings::CameraBuffer), 1, vk::ShaderStageFlagBits::eRaygenKHR)
			.Build();

		// Create descriptor pool
		m_pPool = std::make_unique<DescriptorPool>(DescriptorPool::Builder(m_pRenderCtx->device)
			.AddSize(vk::DescriptorType::eAccelerationStructureKHR, 1 * m_NumFrames)
			.AddSize(vk::DescriptorType::eStorageImage, 1 * m_NumFrames)
			.AddSize(vk::DescriptorType::eUniformBuffer, 1 * m_NumFrames)
			.SetMaxSets(m_NumFrames)
			.SetFlags({})
			.Build());

		// Allocate descriptor set
		for (u32 i = 0; i < m_NumFrames; i++)
		{
			auto descriptorSets = m_pPool->Allocate({ m_DescLayout });
			m_FrameDatas.emplace_back(VulkanBuffer{
				m_pRenderCtx, sizeof(RTCameraData), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, "RT Camera buffer"
				},
				descriptorSets[0]
			);

			UpdateDescriptors(i);
		}

		HPR_CORE_LOG_INFO("Created ray tracing descriptor set!");
	}

	void VulkanRaytracer::CreatePipeline()
	{
		m_pShader = std::make_unique<VulkanShader>(
			m_pRenderCtx,
			std::unordered_map<ShaderStageType, std::filesystem::path>{
				{ ShaderStageType::RayGen, "res/shaders/RTAO.rgen" },
				{ ShaderStageType::Miss, "res/shaders/RTAO.rmiss" },
				{ ShaderStageType::ClosestHit, "res/shaders/RTAO.rchit" },
			},
			false);

		vk::RayTracingShaderGroupCreateInfoKHR group = {};
		group.anyHitShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = VK_SHADER_UNUSED_KHR;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;

		// Raygen
		group.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		group.generalShader = static_cast<u32>(RaytracerShaderStageIndices::Raygen);
		m_ShaderGroups.push_back(group);

		// Miss
		group.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
		group.generalShader = static_cast<u32>(RaytracerShaderStageIndices::Miss);
		m_ShaderGroups.push_back(group);

		// Closest hit
		group.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = static_cast<u32>(RaytracerShaderStageIndices::ClosestHit);
		m_ShaderGroups.push_back(group);

		// Push const (We're not using this atm)
		// vk::PushConstantRange pushConst = {};
		// pushConst.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR;
		// pushConst.offset = 0;
		// pushConst.size = sizeof(RaytracerPushConstants);

		// Automatic shader reflection has issues with RT shaders, so we'll just use our custom one here.
		const std::vector descriptorLayouts = { m_DescLayout };
		const std::vector<vk::PushConstantRange> pushConstants = {};

		PipelineBuilder builder(m_pRenderCtx);
		builder.SetShader(m_pShader.get());
		builder.SetDescriptorSetLayout(descriptorLayouts, pushConstants);
		builder.SetMaxRayRecursionDepth(2 < m_pRenderCtx->rtProperties.maxRayRecursionDepth ? 2 : m_pRenderCtx->rtProperties.maxRayRecursionDepth);
		builder.SetRayTracingShaderGroups(m_ShaderGroups);
		m_RtPipeline = std::make_unique<VulkanPipeline>(builder.BuildRaytracing());

		HPR_CORE_LOG_INFO("Created ray tracing pipeline!");
	}

	void VulkanRaytracer::CreateShaderBindingTable()
	{
		constexpr u32 missCount = 1;
		constexpr u32 hitCount = 1;
		constexpr u32 handleCount = 1 + missCount + hitCount;
		const u32 handleSize = m_pRenderCtx->rtProperties.shaderGroupHandleSize;

		// The SBT (buffer) needs to have starting groups to be aligned and handles in the group to be aligned.
		const u32 handleSizeAligned = AlignUp(handleSize, m_pRenderCtx->rtProperties.shaderGroupHandleAlignment);

		const u32 groupBaseAlignment = m_pRenderCtx->rtProperties.shaderGroupBaseAlignment;
		m_RGenRegion.stride = AlignUp(handleSizeAligned, groupBaseAlignment);
		m_RGenRegion.size = m_RGenRegion.stride;
		m_MissRegion.stride = handleSizeAligned;
		m_MissRegion.size = AlignUp(missCount * handleSizeAligned, groupBaseAlignment);
		m_HitRegion.stride = handleSizeAligned;
		m_HitRegion.size = AlignUp(hitCount * handleSizeAligned, groupBaseAlignment);

		// Get the shader group handles
		u32 dataSize = handleCount * handleSize;
		std::vector<u8> handles(dataSize);
		VulkanUtils::VkCheck(m_pRenderCtx->device.getRayTracingShaderGroupHandlesKHR(m_RtPipeline->GetPipeline(), 0, handleCount, dataSize, handles.data()));

		// Allocate a buffer for storing the SBT
		vk::DeviceSize sbtSize = m_RGenRegion.size + m_MissRegion.size + m_HitRegion.size + m_CallRegion.size;

		m_SbtBuffer = std::make_unique<VulkanBuffer>(
			m_pRenderCtx, sbtSize,
			vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			"SBT"
			);

		// Find the SBT addresses of each group
		vk::DeviceAddress sbtAddress = m_SbtBuffer->GetDeviceAddress();
		m_RGenRegion.deviceAddress = sbtAddress;
		m_MissRegion.deviceAddress = sbtAddress + m_RGenRegion.size;
		m_HitRegion.deviceAddress = sbtAddress + m_RGenRegion.size + m_MissRegion.size;

		// Helper to retrieve the handle data
		auto getHandle = [&](u32 i) { return handles.data() + i * handleSize; };

		// Map the SBT buffer and write in the handles.
		u8* pSBTBuffer = reinterpret_cast<u8*>(m_SbtBuffer->Map());
		u8* pData = nullptr;
		u32 handleIdx = 0;
		// Raygen
		pData = pSBTBuffer;
		memcpy(pData, getHandle(handleIdx++), handleSize);
		// Miss
		pData = pSBTBuffer + m_RGenRegion.size;
		for (u32 c = 0; c < missCount; c++)
		{
			memcpy(pData, getHandle(handleIdx++), handleSize);
			pData += m_MissRegion.stride;
		}
		// Hit
		pData = pSBTBuffer + m_RGenRegion.size + m_MissRegion.size;
		for (uint32_t c = 0; c < hitCount; c++)
		{
			memcpy(pData, getHandle(handleIdx++), handleSize);
			pData += m_HitRegion.stride;
		}

		m_SbtBuffer->Unmap();

		HPR_CORE_LOG_INFO("Created ray tracing SBT!");
	}

	void VulkanRaytracer::UpdateDescriptors(u32 frameIdx)
	{
		// Write descriptor set
		vk::WriteDescriptorSetAccelerationStructureKHR descASInfo{};
		descASInfo.setAccelerationStructures(m_Tlas);

		vk::DescriptorImageInfo imageInfo = {};
		imageInfo.sampler = vk::Sampler{};
		imageInfo.imageView = m_pOutputImage->GetColorImage()->GetImageView();
		imageInfo.imageLayout = vk::ImageLayout::eGeneral; // ImageLayout NEEDS to be VK_IMAGE_LAYOUT_GENERAL when StorageImage

		RTFrameData& frameData = m_FrameDatas[frameIdx];
		{
			// hacky workaround - TODO: make this not hacky :)
			void* mapped = frameData.rtCameraUniform.Map();
			memcpy(mapped, &m_CameraData, sizeof(RTCameraData));
			frameData.rtCameraUniform.Unmap();
		}

		vk::DescriptorBufferInfo cameraInfo = {};
		cameraInfo.buffer = frameData.rtCameraUniform.GetBuffer();
		cameraInfo.offset = 0;
		cameraInfo.range = sizeof(RTCameraData);

		DescriptorWriter writer(m_pRenderCtx->device, frameData.descriptorSet);
		writer.WriteAccelStructure(&descASInfo, static_cast<u32>(RaytracerBindings::Acceleration));
		writer.WriteImage(imageInfo, static_cast<u32>(RaytracerBindings::OutputImage), vk::DescriptorType::eStorageImage);
		writer.WriteBuffer(cameraInfo, static_cast<u32>(RaytracerBindings::CameraBuffer), vk::DescriptorType::eUniformBuffer);
		writer.Write();
	}
}
