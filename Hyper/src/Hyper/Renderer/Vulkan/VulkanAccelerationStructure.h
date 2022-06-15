#pragma once
#include "VulkanBuffer.h"
#include "VulkanDescriptors.h"
#include "VulkanImage.h"
#include "VulkanPipeline.h"
#include "Hyper/Renderer/RenderTarget.h"

namespace Hyper
{
	class Mesh;

	enum class RaytracerBindings : u32
	{
		Acceleration = 0,
		OutputImage = 1,
	};

	enum class RaytracerShaderStageIndices : u32
	{
		Raygen = 0,
		Miss,
		ClosestHit,

		ShaderStageCount,
	};

	struct RaytracerPushConstants
	{
		
	};

	// Temp helper functions (yes, they're from the nvpro samples for now)
	template <class integral>
	constexpr bool IsAligned(integral x, size_t a) noexcept
	{
		return (x & (integral(a) - 1)) == 0;
	}

	template <class integral>
	constexpr integral AlignUp(integral x, size_t a) noexcept
	{
		return integral((x + (integral(a) - 1)) & ~integral(a - 1));
	}

	template <class integral>
	constexpr integral AlignDown(integral x, size_t a) noexcept
	{
		return integral(x & ~integral(a - 1));
	}

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

		void RayTrace(vk::CommandBuffer cmd);

		[[nodiscard]] RenderTarget* GetOutputImage() const { return m_pOutputImage.get(); }

	private:
		void CreateBlas(const Mesh* pMesh);
		void CreateTlas();
		void CreateDescriptorSet();
		void CreatePipeline();
		void CreateShaderBindingTable();

	private:
		RenderContext* m_pRenderCtx;

		std::vector<Accel> m_BLASes{};
		Accel m_Tlas{};

		std::vector<const Mesh*> m_StagedMeshes;

		vk::DescriptorSetLayout m_DescLayout;
		std::unique_ptr<DescriptorPool> m_pPool;
		vk::DescriptorSet m_Desc;

		const u32 m_OutputWidth = 1920;
		const u32 m_OutputHeight = 1080;
		std::unique_ptr<RenderTarget> m_pOutputImage;
		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> m_ShaderGroups;
		std::unique_ptr<VulkanShader> m_pShader;
		std::unique_ptr<VulkanPipeline> m_RtPipeline;

		std::unique_ptr<VulkanBuffer> m_SbtBuffer;
		vk::StridedDeviceAddressRegionKHR m_RGenRegion{};
		vk::StridedDeviceAddressRegionKHR m_MissRegion{};
		vk::StridedDeviceAddressRegionKHR m_HitRegion{};
		vk::StridedDeviceAddressRegionKHR m_CallRegion{};
	};
}
