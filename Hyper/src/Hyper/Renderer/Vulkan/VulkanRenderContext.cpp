#include "HyperPCH.h"
#include "VulkanRenderContext.h"

namespace Hyper
{
	VulkanRenderContext::VulkanRenderContext()
	{
		const vk::ApplicationInfo appInfo = vk::ApplicationInfo()
			.setPApplicationName("Hyper Application")
			.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
			.setPEngineName("Hyper Engine")
			.setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
			.setApiVersion(VK_API_VERSION_1_2);

		constexpr const char* VK_KHR_WIN32_SURFACE_EXTENSION_NAME = "VK_KHR_win32_surface";

		std::vector<const char*> extensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };

		if (HYPER_VALIDATE)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
			.setPApplicationInfo(&appInfo)
			.setPEnabledLayerNames({})
			.setPEnabledExtensionNames(extensions);

		if (HYPER_VALIDATE)
		{
			createInfo.enabledLayerCount = static_cast<u32>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
		}

		try
		{
			m_Instance = vk::createInstance(createInfo);
			HPR_CORE_LOG_INFO("Successfully created Vulkan Instance!");
		}
		catch (vk::SystemError& e)
		{
			throw std::runtime_error("Failed to create Instance: {}"s + e.what());
		}
	}
}
