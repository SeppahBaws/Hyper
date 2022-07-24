#include "HyperPCH.h"
#include "VulkanAccelerationStructure.h"

#include "VulkanDebug.h"
#include "VulkanUtility.h"
#include "Hyper/Renderer/FlyCamera.h"
#include "Hyper/Renderer/Mesh.h"
#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	VulkanAccelerationStructure::VulkanAccelerationStructure(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
	}

	VulkanAccelerationStructure::~VulkanAccelerationStructure()
	{
		m_pRenderCtx->device.destroyAccelerationStructureKHR(m_Tlas.handle);
		m_Tlas.pBuffer.reset();

		for (auto& blas : m_BLASes)
		{
			m_pRenderCtx->device.destroyAccelerationStructureKHR(blas.handle);
			blas.pBuffer.reset();
		}
	}

	void VulkanAccelerationStructure::AddMesh(const Mesh* mesh, const glm::mat4& transform, const std::string& name)
	{
		m_StagedMeshes.push_back(std::tuple(mesh, transform, name));
	}

	void VulkanAccelerationStructure::Build()
	{
		for (auto&[mesh, transform, name] : m_StagedMeshes)
		{
			CreateBlas(mesh, transform, name);
		}
		HPR_CORE_LOG_INFO("Created all BLASes!");

		CreateTlas();
		HPR_CORE_LOG_INFO("Created TLAS!");
	}

	void VulkanAccelerationStructure::CreateBlas(const Mesh* pMesh, const glm::mat4& transform, const std::string& name)
	{
		Accel bottomLevelAS;
		bottomLevelAS.transform = transform;

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
		bottomLevelAS.handle = VulkanUtils::Check(m_pRenderCtx->device.createAccelerationStructureKHR(accelerationStructureCreateInfo));

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

		VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::eAccelerationStructureKHR, bottomLevelAS.handle, fmt::format("{}", name));

		m_BLASes.push_back(std::move(bottomLevelAS));
	}

	void VulkanAccelerationStructure::CreateTlas()
	{
		std::vector<vk::AccelerationStructureInstanceKHR> tlas;
		tlas.reserve(m_BLASes.size());

		for (auto& blas : m_BLASes)
		{
			// GLM is column-major, but VkTransformMatrixKHR is row-major, so we need to convert.
			vk::TransformMatrixKHR transformMatrix = std::array{
				std::array{blas.transform[0][0], blas.transform[1][0], blas.transform[2][0], blas.transform[3][0]},
				std::array{blas.transform[0][1], blas.transform[1][1], blas.transform[2][1], blas.transform[3][1]},
				std::array{blas.transform[0][2], blas.transform[1][2], blas.transform[2][2], blas.transform[3][2]},
			};
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
		m_Tlas.handle = VulkanUtils::Check(m_pRenderCtx->device.createAccelerationStructureKHR(createInfo));

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
}
