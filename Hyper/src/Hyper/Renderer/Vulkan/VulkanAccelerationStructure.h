#pragma once
#include "VulkanBuffer.h"

namespace Hyper
{
	class Mesh;

	class VulkanAccelerationStructure
	{
		struct Accel
		{
			vk::AccelerationStructureKHR handle;
			u64 deviceAddress = 0;
			std::unique_ptr<VulkanBuffer> pBuffer;
		};

		struct RayTracingScratchBuffer
		{
			u64 deviceAddress = 0;
			std::unique_ptr<VulkanBuffer> pBuffer;
		};

		struct VulkanRTBuffer
		{
			u32 size = 0;
			void* localData = nullptr;
			std::unique_ptr<VulkanBuffer> buffer;
		};

	public:
		VulkanAccelerationStructure(RenderContext* pRenderCtx);
		~VulkanAccelerationStructure();

		void AddMesh(const Mesh* mesh);
		void Build();

		[[nodiscard]] const Accel& GetTLAS() const { return m_Tlas; }

	private:
		void CreateBlas(const Mesh* pMesh);
		void CreateTlas();

	private:
		RenderContext* m_pRenderCtx;

		std::vector<Accel> m_BLASes{};
		Accel m_Tlas{};

		std::vector<const Mesh*> m_StagedMeshes;
	};
}
