#include "HyperPCH.h"
#include "VulkanAccelerationStructure.h"

#include <d3d12sdklayers.h>

#include "VulkanDebug.h"
#include "VulkanDescriptors.h"
#include "VulkanPipeline.h"
#include "VulkanUtility.h"
#include "Hyper/Renderer/Mesh.h"

namespace Hyper
{
	VulkanAccelerationStructure::VulkanAccelerationStructure(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
		m_pOutputImage = std::make_unique<RenderTarget>(m_pRenderCtx, "Raytracing output image", m_OutputWidth, m_OutputHeight);
	}

	VulkanAccelerationStructure::~VulkanAccelerationStructure()
	{
		m_SbtBuffer.reset();
		m_RtPipeline.reset();
		m_pShader.reset();
		m_pOutputImage.reset();
		m_pPool.reset();
		m_pRenderCtx->device.destroyDescriptorSetLayout(m_DescLayout);

		m_pRenderCtx->device.destroyAccelerationStructureKHR(m_Tlas.handle);
		m_Tlas.pBuffer.reset();

		for (auto& blas : m_BLASes)
		{
			m_pRenderCtx->device.destroyAccelerationStructureKHR(blas.handle);
			blas.pBuffer.reset();
		}

		m_pOutputImage.reset();
	}

	void VulkanAccelerationStructure::AddMesh(const Mesh* mesh)
	{
		m_StagedMeshes.push_back(mesh);
	}

	void VulkanAccelerationStructure::Build()
	{
		for (const Mesh* mesh : m_StagedMeshes)
		{
			CreateBlas(mesh);
		}
		HPR_CORE_LOG_INFO("Created all BLASes!");

		CreateTlas();
		HPR_CORE_LOG_INFO("Created TLAS!");

		// For testing...
		CreateDescriptorSet();
		CreatePipeline();
		CreateShaderBindingTable();
	}

	void VulkanAccelerationStructure::RayTrace(vk::CommandBuffer cmd)
	{
		m_pOutputImage->GetColorImage()->TransitionLayout(cmd, vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, vk::ImageLayout::eGeneral,
			vk::PipelineStageFlagBits::eRayTracingShaderKHR);

		cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_RtPipeline->GetPipeline());
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, m_RtPipeline->GetLayout(), 0, { m_Desc }, {});
		cmd.traceRaysKHR(m_RGenRegion, m_MissRegion, m_HitRegion, m_CallRegion, m_OutputWidth, m_OutputHeight, 1);

		VkDebug::EndRegion(cmd);
	}

	void VulkanAccelerationStructure::CreateBlas(const Mesh* pMesh)
	{
		Accel bottomLevelAS;

		vk::DeviceAddress vertexBufferDeviceAddress{};
		vk::DeviceAddress indexBufferDeviceAddress{};

		vertexBufferDeviceAddress = pMesh->GetVertexBuffer()->GetBuffer()->GetDeviceAddress();
		indexBufferDeviceAddress = pMesh->GetIndexBuffer()->GetBuffer()->GetDeviceAddress();

		// Build geometries
		vk::AccelerationStructureGeometryKHR accelerationStructureGeometry = {};
		accelerationStructureGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
		accelerationStructureGeometry.geometryType = vk::GeometryTypeKHR::eTriangles;
		accelerationStructureGeometry.geometry.triangles.vertexFormat = vk::Format::eR32G32B32A32Sfloat;
		accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress = vertexBufferDeviceAddress;
		accelerationStructureGeometry.geometry.triangles.maxVertex = pMesh->GetVertexCount();
		accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(VertexPosNormTex);
		accelerationStructureGeometry.geometry.triangles.indexType = vk::IndexType::eUint32;
		accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress = indexBufferDeviceAddress;

		// Build size info
		vk::AccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
		accelerationStructureBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
		accelerationStructureBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
		accelerationStructureBuildGeometryInfo.setGeometries(accelerationStructureGeometry);

		const u32 numTriangles = pMesh->GetTriCount();
		vk::AccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = m_pRenderCtx->device.getAccelerationStructureBuildSizesKHR(
			vk::AccelerationStructureBuildTypeKHR::eDevice,
			accelerationStructureBuildGeometryInfo,
			{ numTriangles });

		bottomLevelAS.pBuffer = std::make_unique<VulkanBuffer>(
			m_pRenderCtx,
			accelerationStructureBuildSizesInfo.accelerationStructureSize,
			vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			"BLAS test"
		);
		bottomLevelAS.deviceAddress = bottomLevelAS.pBuffer->GetDeviceAddress();

		vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
		accelerationStructureCreateInfo.buffer = bottomLevelAS.pBuffer->GetBuffer();
		accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
		bottomLevelAS.handle = m_pRenderCtx->device.createAccelerationStructureKHR(accelerationStructureCreateInfo);

		// Create a scratch buffer that will be used during the build of the BLAS.
		RayTracingScratchBuffer scratchBuffer{};
		scratchBuffer.pBuffer = std::make_unique<VulkanBuffer>(m_pRenderCtx,
			accelerationStructureBuildSizesInfo.buildScratchSize,
			vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
			VMA_MEMORY_USAGE_GPU_ONLY,
			"BLAS Scratch buffer");
		scratchBuffer.deviceAddress = scratchBuffer.pBuffer->GetDeviceAddress();

		vk::AccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = {};
		accelerationBuildGeometryInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
		accelerationBuildGeometryInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
		accelerationBuildGeometryInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
		accelerationBuildGeometryInfo.dstAccelerationStructure = bottomLevelAS.handle;
		accelerationBuildGeometryInfo.setGeometries(accelerationStructureGeometry);
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

		vk::AccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo = {};
		accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;
		std::vector accelerationStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		vk::CommandBuffer cmd = m_pRenderCtx->commandPool->GetCommandBuffer();
		VulkanCommandBuffer::Begin(cmd, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		cmd.buildAccelerationStructuresKHR(accelerationBuildGeometryInfo, accelerationStructureRangeInfos);
		VulkanCommandBuffer::End(cmd);
		m_pRenderCtx->graphicsQueue.Submit({}, {}, {}, cmd, {});
		m_pRenderCtx->graphicsQueue.WaitIdle();
		m_pRenderCtx->commandPool->FreeCommandBuffer(cmd);

		scratchBuffer.pBuffer.reset();

		VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::eAccelerationStructureKHR, bottomLevelAS.handle, fmt::format("BLAS {}", m_BLASes.size()));

		m_BLASes.push_back(std::move(bottomLevelAS));
	}

	void VulkanAccelerationStructure::CreateTlas()
	{
		vk::TransformMatrixKHR transformMatrix = std::array{
			std::array{ 1.0f, 0.0f, 0.0f, 0.0f },
			std::array{ 0.0f, 1.0f, 0.0f, 0.0f },
			std::array{ 0.0f, 0.0f, 1.0f, 0.0f }
		};

		std::vector<vk::AccelerationStructureInstanceKHR> tlas;
		tlas.reserve(m_BLASes.size());

		for (auto& blas : m_BLASes)
		{
			auto& instance = tlas.emplace_back(vk::AccelerationStructureInstanceKHR{});
			instance.transform = transformMatrix;
			instance.instanceCustomIndex = 0;
			instance.mask = 0xFF;
			instance.instanceShaderBindingTableRecordOffset = 0;
			instance.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
			// VkHPP type doesn't work here: https://bytemeta.vip/repo/KhronosGroup/Vulkan-Hpp/issues/1002
			instance.accelerationStructureReference = blas.deviceAddress;
		}

		// Build the TLAS

		u32 countInstance = static_cast<u32>(tlas.size());

		vk::CommandBuffer cmd = m_pRenderCtx->commandPool->GetCommandBuffer();
		VulkanCommandBuffer::Begin(cmd, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		VulkanRTBuffer instancesBuffer;
		instancesBuffer.buffer = std::make_unique<VulkanBuffer>(
			m_pRenderCtx,
			tlas.data(),
			tlas.size() * sizeof(vk::AccelerationStructureInstanceKHR),
			vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			""
		);

		vk::DeviceAddress instBufferAddr = instancesBuffer.buffer->GetDeviceAddress();

		// Make sure the copy of the instance buffer is copied before triggering the acceleration structure build
		vk::MemoryBarrier barrier{};
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR;
		cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
			{},
			barrier,
			{},
			{}
		);

		// Creating the TLAS

		vk::AccelerationStructureGeometryInstancesDataKHR instancesVk = {};
		instancesVk.data.deviceAddress = instBufferAddr;

		vk::AccelerationStructureGeometryKHR topASGeometry = {};
		topASGeometry.geometryType = vk::GeometryTypeKHR::eInstances;
		topASGeometry.geometry.instances = instancesVk;

		// Find sizes
		vk::AccelerationStructureBuildGeometryInfoKHR buildInfo = {};
		buildInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
		buildInfo.setGeometries(topASGeometry);
		buildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
		buildInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
		buildInfo.srcAccelerationStructure = nullptr;

		vk::AccelerationStructureBuildSizesInfoKHR sizeInfo = m_pRenderCtx->device.getAccelerationStructureBuildSizesKHR(
			vk::AccelerationStructureBuildTypeKHR::eDevice,
			buildInfo,
			countInstance
		);

		// Create TLAS
		vk::AccelerationStructureCreateInfoKHR createInfo = {};
		createInfo.size = sizeInfo.accelerationStructureSize;

		m_Tlas.pBuffer = std::make_unique<VulkanBuffer>(
			m_pRenderCtx,
			sizeInfo.accelerationStructureSize,
			vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst,
			VMA_MEMORY_USAGE_CPU_TO_GPU,
			"TLAS test"
		);
		m_Tlas.deviceAddress = m_Tlas.pBuffer->GetDeviceAddress();
		createInfo.buffer = m_Tlas.pBuffer->GetBuffer();
		m_Tlas.handle = m_pRenderCtx->device.createAccelerationStructureKHR(createInfo);

		// Allocate scratch memory
		RayTracingScratchBuffer scratchBuffer = {};
		scratchBuffer.pBuffer = std::make_unique<VulkanBuffer>(m_pRenderCtx,
			sizeInfo.buildScratchSize,
			vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
			VMA_MEMORY_USAGE_GPU_ONLY,
			"TLAS Scratch buffer");
		scratchBuffer.deviceAddress = scratchBuffer.pBuffer->GetDeviceAddress();

		// Update build inforation
		buildInfo.srcAccelerationStructure = nullptr;
		buildInfo.dstAccelerationStructure = m_Tlas.handle;
		buildInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

		// Build offsets info: n instances
		vk::AccelerationStructureBuildRangeInfoKHR buildOffsetInfo = {};
		buildOffsetInfo.primitiveCount = countInstance;
		buildOffsetInfo.primitiveOffset = 0;
		buildOffsetInfo.firstVertex = 0;
		buildOffsetInfo.transformOffset = 0;
		const vk::AccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

		cmd.buildAccelerationStructuresKHR(buildInfo, pBuildOffsetInfo);

		VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::eAccelerationStructureKHR, m_Tlas.handle, "Top Level Acceleration Structure");

		VulkanCommandBuffer::End(cmd);
		m_pRenderCtx->graphicsQueue.Submit({}, {}, {}, cmd, {});
		m_pRenderCtx->graphicsQueue.WaitIdle();
		m_pRenderCtx->commandPool->FreeCommandBuffer(cmd);

		scratchBuffer.pBuffer.reset();
	}

	void VulkanAccelerationStructure::CreateDescriptorSet()
	{
		// Create descriptor set layout
		m_DescLayout = DescriptorSetLayoutBuilder(m_pRenderCtx->device)
			.AddBinding(vk::DescriptorType::eAccelerationStructureKHR, static_cast<u32>(RaytracerBindings::Acceleration), 1, vk::ShaderStageFlagBits::eRaygenKHR)
			.AddBinding(vk::DescriptorType::eStorageImage, static_cast<u32>(RaytracerBindings::OutputImage), 1, vk::ShaderStageFlagBits::eRaygenKHR)
			.Build();

		constexpr u32 maxSets = 1;

		// Create descriptor pool
		m_pPool = std::make_unique<DescriptorPool>(DescriptorPool::Builder(m_pRenderCtx->device)
			.AddSize(vk::DescriptorType::eAccelerationStructureKHR, 1 * maxSets)
			.AddSize(vk::DescriptorType::eStorageImage, 1 * maxSets)
			.SetMaxSets(maxSets)
			.SetFlags({})
			.Build());

		// Allocate descriptor set
		m_Desc = m_pPool->Allocate({ m_DescLayout })[0];

		// Write descriptor set
		vk::WriteDescriptorSetAccelerationStructureKHR descASInfo{};
		descASInfo.setAccelerationStructures(m_Tlas.handle);

		vk::DescriptorImageInfo imageInfo = {};
		imageInfo.sampler = vk::Sampler{};
		imageInfo.imageView = m_pOutputImage->GetColorImage()->GetImageView();
		imageInfo.imageLayout = vk::ImageLayout::eGeneral; // ImageLayout NEEDS to be VK_IMAGE_LAYOUT_GENERAL when StorageImage

		DescriptorWriter writer(m_pRenderCtx->device, m_Desc);
		writer.WriteAccelStructure(&descASInfo, static_cast<u32>(RaytracerBindings::Acceleration));
		writer.WriteImage(imageInfo, static_cast<u32>(RaytracerBindings::OutputImage), vk::DescriptorType::eStorageImage);
		writer.Write();

		HPR_CORE_LOG_INFO("Created ray tracing descriptor set!");
	}

	void VulkanAccelerationStructure::CreatePipeline()
	{
		m_pShader = std::make_unique<VulkanShader>(m_pRenderCtx, std::unordered_map<ShaderStageType, std::filesystem::path>{
			{ ShaderStageType::RayGen, "res/shaders/RTAO.rgen" },
			{ ShaderStageType::Miss, "res/shaders/RTAO.rmiss" },
			{ ShaderStageType::ClosestHit, "res/shaders/RTAO.rchit" },
		});

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

		const std::vector descriptorLayouts = m_pShader->GetAllDescriptorSetLayouts();
		const std::vector pushConstants = m_pShader->GetAllPushConstantRanges();

		PipelineBuilder builder(m_pRenderCtx);
		builder.SetShader(m_pShader.get());
		builder.SetDescriptorSetLayout(descriptorLayouts, pushConstants);
		builder.SetMaxRayRecursionDepth(2 < m_pRenderCtx->rtProperties.maxRayRecursionDepth ? 2 : m_pRenderCtx->rtProperties.maxRayRecursionDepth);
		builder.SetRayTracingShaderGroups(m_ShaderGroups);
		m_RtPipeline = std::make_unique<VulkanPipeline>(builder.BuildRaytracing());

		HPR_CORE_LOG_INFO("Created ray tracing pipeline!");
	}

	void VulkanAccelerationStructure::CreateShaderBindingTable()
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
}
