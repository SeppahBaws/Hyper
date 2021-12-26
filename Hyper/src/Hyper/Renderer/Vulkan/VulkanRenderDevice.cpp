#include "HyperPCH.h"
#include "VulkanRenderDevice.h"

#include "VulkanDebug.h"
#include "VulkanUtility.h"

namespace Hyper
{
	VulkanRenderDevice::VulkanRenderDevice()
	{
		if (HYPER_VALIDATE && !CheckValidationLayerSupport())
		{
			HPR_CORE_LOG_ERROR("Vulkan Validation layers were requested, but none are available!");
			return;
		}

		CreateInstance();

		VkDebug::Setup(m_Instance);

		PickDevice();
	}

	VulkanRenderDevice::~VulkanRenderDevice()
	{
		VkDebug::FreeDebugCallback(m_Instance);
		m_Instance.destroy();
	}

	void VulkanRenderDevice::CreateInstance()
	{
		vk::ApplicationInfo appInfo{
			"Hyper Application", VK_MAKE_VERSION(1, 0, 0),
			"Hyper Engine", VK_MAKE_VERSION(1, 0, 0),
			VK_API_VERSION_1_2
		};

		constexpr const char* VK_KHR_WIN32_SURFACE_EXTENSION_NAME = "VK_KHR_win32_surface";

		std::vector extensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };

		if (HYPER_VALIDATE)
		{
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		vk::InstanceCreateInfo createInfo{ {}, &appInfo, {}, extensions };

		if (HYPER_VALIDATE)
		{
			createInfo.setPEnabledLayerNames(m_ValidationLayers);
		}

		try
		{
			m_Instance = vk::createInstance(createInfo);
			HPR_CORE_LOG_INFO("Successfully created Vulkan Instance!");
		}
		catch (vk::SystemError& e)
		{
			throw std::runtime_error("Failed to create Vulkan Instance:"s + e.what());
		}
	}

	bool VulkanRenderDevice::CheckValidationLayerSupport() const
	{
		std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

		for (const char* layerName : m_ValidationLayers)
		{
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}
			if (!layerFound)
				return false;
		}

		return true;
	}

	void VulkanRenderDevice::PickDevice()
	{
	}
}
