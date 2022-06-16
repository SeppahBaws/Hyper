#pragma once
#include <glm/mat4x4.hpp>

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
			glm::mat4 transform;
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

		void AddMesh(const Mesh* mesh, const glm::mat4& transform);
		void Build();

		[[nodiscard]] const Accel& GetTLAS() const { return m_Tlas; }

	private:
		void CreateBlas(const Mesh* pMesh, const glm::mat4& transform);
		void CreateTlas();

	private:
		RenderContext* m_pRenderCtx;

		std::vector<Accel> m_BLASes{};
		Accel m_Tlas{};

		std::vector<std::pair<const Mesh*, glm::mat4>> m_StagedMeshes;
	};
}
