#pragma once
#include <glm/mat4x4.hpp>

#include "VulkanBuffer.h"

namespace Hyper
{
	struct LightingSettings;
	class FlyCamera;
	class VulkanAccelerationStructure;
	class DescriptorPool;
	class RenderTarget;
	class VulkanShader;
	class VulkanPipeline;
	struct RenderContext;

	struct RTCameraData
	{
		glm::mat4 viewInv{ 1.0f };
		glm::mat4 projInv{ 1.0f };
	};

	struct RTPushConstants
	{
		glm::vec3 sunDirection;
	};

	struct RTFrameData
	{
		VulkanBuffer rtCameraUniform;
		vk::DescriptorSet descriptorSet;
	};

	enum class RaytracerBindings : u32
	{
		Acceleration = 0,
		OutputImage = 1,
		CameraBuffer = 2,
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
		VulkanRaytracer(RenderContext* pRenderCtx, vk::AccelerationStructureKHR tlas, u32 numFrames, u32 outputWidth, u32 outputHeight);
		~VulkanRaytracer();

		void RayTrace(vk::CommandBuffer cmd, FlyCamera* pCamera, u32 frameIdx, const LightingSettings& lightingSettings);
		void Resize(u32 width, u32 height);

		[[nodiscard]] const RenderTarget* GetOutputImage() const { return m_pOutputImage.get(); }

	private:
		void CreateDescriptorSet();
		void CreatePipeline();
		void CreateShaderBindingTable();

		void UpdateDescriptors(u32 frameIdx);

	private:
		RenderContext* m_pRenderCtx;
		const vk::AccelerationStructureKHR m_Tlas;
		const u32 m_NumFrames;

		vk::DescriptorSetLayout m_DescLayout;
		std::unique_ptr<DescriptorPool> m_pPool;
		RTCameraData m_CameraData;
		std::vector<RTFrameData> m_FrameDatas;

		RTPushConstants m_RtPushConstants{};

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
