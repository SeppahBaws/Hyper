#pragma once
#include <vulkan/vulkan.hpp>

#define WIN32_LEAN_AND_MEAN // VMA includes windows.h, so make sure that this is not enabled.
#include <vk_mem_alloc.h>

#include "VulkanQueue.h"
#include "NsightAftermath/NsightAftermathGpuCrashTracker.h"

namespace Hyper
{
	struct RenderContext;

	class VulkanDevice final
	{
	public:
		VulkanDevice(RenderContext* pRenderCtx);
		~VulkanDevice();

	private:
		[[nodiscard]] bool CheckExtensionsAndLayersSupport() const;

	private:
#ifdef HYPER_DISTRIBUTE
		const bool HYPER_VALIDATE = false;
#else
		const bool HYPER_VALIDATE = true;
#endif

		std::vector<const char*> m_RequiredInstanceLayerNames = {};
		std::vector<const char*> m_RequiredInstanceExtensionNames = {};
		std::vector<const char*> m_RequiredDeviceLayerNames = {};
		std::vector<const char*> m_RequiredDeviceExtensionNames = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,

			// Ray tracing extensions
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,

			// Required by VK_KHR_acceleration_structure
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,

			// Required by VK_KHR_ray_tracing_pipeline
			VK_KHR_SPIRV_1_4_EXTENSION_NAME,

			// Required by VK_KHR_spirv_1_4
			VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
		};

		RenderContext* m_pRenderCtx;
		VulkanQueue m_GraphicsQueue;
		VulkanQueue m_ComputeQueue;
		vk::CommandPool m_CommandPool;
		VmaAllocator m_Allocator;

#ifdef HYPER_USE_AFTERMATH
		GpuCrashTracker::MarkerMap m_MarkerMap;
		GpuCrashTracker m_GpuCrashTracker;
#endif
	};
}
