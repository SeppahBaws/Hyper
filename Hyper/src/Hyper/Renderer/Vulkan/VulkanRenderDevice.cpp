#include "HyperPCH.h"
#include "VulkanRenderDevice.h"

#include "VulkanDebug.h"
#include "VulkanUtility.h"

namespace Hyper
{
	VulkanRenderDevice::VulkanRenderDevice()
	{
		if (HYPER_VALIDATE)
		{
			m_RequiredLayerNames.push_back("VK_LAYER_KHRONOS_validation");
		}

		if (!CheckExtensionsAndLayersSupport())
		{
			return;
		}
		if (HYPER_VALIDATE && !CheckExtensionsAndLayersSupport())
		{
			HPR_CORE_LOG_ERROR("Vulkan Validation layers were requested, but none are available!");
			return;
		}

		CreateInstance();

		VkDebug::Setup(m_Instance);

		CreateDevice();
		CreateCommandPool();
	}

	VulkanRenderDevice::~VulkanRenderDevice()
	{
		m_Device.destroyCommandPool(m_CommandPool);
		m_Device.destroy();

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
			createInfo.setPEnabledLayerNames(m_RequiredLayerNames);
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

	bool VulkanRenderDevice::CheckExtensionsAndLayersSupport() const
	{
		std::vector availableExtensions = vk::enumerateInstanceExtensionProperties();
		std::vector availableLayers = vk::enumerateInstanceLayerProperties();

		// Check if the required extensions are available
		const bool hasRequiredExtensions = std::ranges::all_of(m_RequiredExtensionNames, 
			[&availableExtensions](const char* name)
			{
				return std::ranges::find_if(availableExtensions, [&name](const vk::ExtensionProperties& property)
				{
					return strcmp(property.extensionName, name) == 0;
				}) != availableExtensions.end();
			});
		if (!hasRequiredExtensions)
		{
			HPR_VKLOG_ERROR("Cannot find one or more of the required extensions.");
		}

		// Check if the required layers are available
		const bool hasRequiredLayers = std::ranges::all_of(m_RequiredLayerNames,
			[&availableLayers](const char* name)
			{
				return std::ranges::find_if(availableLayers, [&name](const vk::LayerProperties& property)
				{
					return strcmp(property.layerName, name) == 0;
				}) != availableLayers.end();
			});

		if (!hasRequiredLayers)
		{
			HPR_VKLOG_ERROR("Cannot find one or more of the required layers.");
		}

		return hasRequiredExtensions && hasRequiredLayers;
	}

	void VulkanRenderDevice::CreateDevice()
	{
		auto availableDevices = m_Instance.enumeratePhysicalDevices();
		const auto foundDevice = std::ranges::find_if(availableDevices, [](const vk::PhysicalDevice& device)
		{
			const vk::PhysicalDeviceProperties properties = device.getProperties();
			return properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
		});

		if (foundDevice == availableDevices.end())
		{
			HPR_VKLOG_ERROR("Could not find a discrete pysical device!");
			return;
		}

		m_PhysicalDevice = *foundDevice;

		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();

		// Get the first index which supports graphics
		const auto propertyIterator = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
			[](const vk::QueueFamilyProperties& qfp)
			{
				return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
			});

		m_GraphicsQueueFamilyIndex = static_cast<u32>(std::distance(queueFamilyProperties.begin(), propertyIterator));
		assert(m_GraphicsQueueFamilyIndex < queueFamilyProperties.size());

		// Create a device
		constexpr f32 queuePriority = 0.0f;
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
			{}, m_GraphicsQueueFamilyIndex, 1, &queuePriority
		};

		vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo{
			{}, deviceQueueCreateInfo, m_RequiredLayerNames, m_RequiredExtensionNames, {}
		};
		m_Device = m_PhysicalDevice.createDevice(deviceCreateInfo);

		m_GraphicsQueue = m_Device.getQueue(m_GraphicsQueueFamilyIndex, 0);
	}

	void VulkanRenderDevice::CreateCommandPool()
	{
		vk::CommandPoolCreateInfo info{
			{}, m_GraphicsQueueFamilyIndex
		};

		try
		{
			m_CommandPool = m_Device.createCommandPool(info);
		}
		catch (vk::SystemError& e)
		{
			throw std::runtime_error("Failed to create command pool:"s + e.what());
		}
	}
}
