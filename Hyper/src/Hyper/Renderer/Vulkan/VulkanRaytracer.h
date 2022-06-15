#pragma once

namespace Hyper
{
	class VulkanAccelerationStructure;
	class DescriptorPool;
	class RenderTarget;
	class VulkanShader;
	class VulkanPipeline;
	class VulkanBuffer;
	struct RenderContext;

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

	class VulkanRaytracer
	{
	public:
		VulkanRaytracer(RenderContext* pRenderCtx, vk::AccelerationStructureKHR tlas);
		~VulkanRaytracer();

		void RayTrace(vk::CommandBuffer cmd);
		void Resize(u32 width, u32 height);

		[[nodiscard]] const RenderTarget* GetOutputImage() const { return m_pOutputImage.get(); }

	private:
		void CreateDescriptorSet();
		void CreatePipeline();
		void CreateShaderBindingTable();

	private:
		RenderContext* m_pRenderCtx;

		vk::AccelerationStructureKHR m_Tlas;

		vk::DescriptorSetLayout m_DescLayout;
		std::unique_ptr<DescriptorPool> m_pPool;
		vk::DescriptorSet m_Desc;

		u32 m_OutputWidth = 1920;
		u32 m_OutputHeight = 1080;
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
