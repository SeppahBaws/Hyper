#include "HyperPCH.h"
#include "VulkanDevice.h"

#include "VulkanDebug.h"
#include "VulkanUtility.h"

namespace Hyper
{
	VulkanDevice::VulkanDevice(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
		if (HYPER_VALIDATE)
		{
			m_RequiredInstanceLayerNames.push_back("VK_LAYER_KHRONOS_validation");
		}

		if (!CheckExtensionsAndLayersSupport())
		{
			return;
		}
		if (HYPER_VALIDATE && !CheckExtensionsAndLayersSupport())
		{
			HPR_VKLOG_ERROR("Vulkan Validation layers were requested, but none are available!");
			return;
		}

		// Create instance
		{
			const vk::ApplicationInfo appInfo{
				"Hyper Application", VK_MAKE_VERSION(1, 0, 0),
				"Hyper Engine", VK_MAKE_VERSION(1, 0, 0),
				VK_API_VERSION_1_3
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
				createInfo.setPEnabledLayerNames(m_RequiredInstanceLayerNames);
			}

			try
			{
				pRenderCtx->instance = vk::createInstance(createInfo);
				HPR_VKLOG_INFO("Successfully created Vulkan Instance!");
			}
			catch (vk::SystemError& e)
			{
				throw std::runtime_error("Failed to create Vulkan Instance:"s + e.what());
			}

			HPR_VKLOG_INFO("Created Vulkan Instance");
		}

		// Setup Vulkan debug utility
		VkDebug::Setup(pRenderCtx);

		// Create device
		{
			// Find physical device
			auto availableDevices = pRenderCtx->instance.enumeratePhysicalDevices();
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

			pRenderCtx->physicalDevice = *foundDevice;

			HPR_VKLOG_INFO("Found a physical device");

			std::vector<vk::QueueFamilyProperties> queueFamilyProperties = pRenderCtx->physicalDevice.getQueueFamilyProperties();

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

			vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
			dynamicRenderingFeatures.dynamicRendering = true;

			vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo{
				{}, deviceQueueCreateInfo, m_RequiredDeviceLayerNames, m_RequiredDeviceExtensionNames, {}
			};
			deviceCreateInfo.pNext = &dynamicRenderingFeatures;
			pRenderCtx->device = pRenderCtx->physicalDevice.createDevice(deviceCreateInfo);

			HPR_VKLOG_INFO("Created the logical device");

			m_GraphicsQueue = pRenderCtx->device.getQueue(m_GraphicsQueueFamilyIndex, 0);
		}

		// Init VMA
		{
			VmaAllocatorCreateInfo createInfo{};
			createInfo.vulkanApiVersion = VK_API_VERSION_1_3;
			createInfo.physicalDevice = pRenderCtx->physicalDevice;
			createInfo.device = pRenderCtx->device;
			createInfo.instance = pRenderCtx->instance;
			
			VulkanUtils::VkCheck(vmaCreateAllocator(&createInfo, &m_Allocator));
		}

		// Create command pool
		{
			vk::CommandPoolCreateInfo info{};
			info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
			info.queueFamilyIndex = m_GraphicsQueueFamilyIndex;

			try
			{
				m_CommandPool = pRenderCtx->device.createCommandPool(info);
			}
			catch (vk::SystemError& e)
			{
				throw std::runtime_error("Failed to create command pool:"s + e.what());
			}

			HPR_VKLOG_INFO("Created the command pool");
		}
	}

	VulkanDevice::~VulkanDevice()
	{
		// TODO: keep track of allocated command buffers and destroy them all.
		m_pRenderCtx->device.destroy(m_CommandPool);

		vmaDestroyAllocator(m_Allocator);

		m_pRenderCtx->device.destroy();
		VkDebug::FreeDebugCallback(m_pRenderCtx->instance);
		m_pRenderCtx->instance.destroy();
	}

	std::vector<vk::CommandBuffer> VulkanDevice::GetCommandBuffers(u32 count)
	{
		vk::CommandBufferAllocateInfo allocInfo{};
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandBufferCount = count;

		std::vector<vk::CommandBuffer> buffers;
		try
		{
			buffers = m_pRenderCtx->device.allocateCommandBuffers(allocInfo);
		}
		catch (vk::SystemError& e)
		{
			throw std::runtime_error("Failed to allocate command buffer: "s + e.what());
		}

		return buffers;
	}

	bool VulkanDevice::CheckExtensionsAndLayersSupport() const
	{
		std::vector availableExtensions = vk::enumerateInstanceExtensionProperties();
		std::vector availableLayers = vk::enumerateInstanceLayerProperties();

		// Check if the required extensions are available
		const bool hasRequiredExtensions = std::ranges::all_of(m_RequiredInstanceExtensionNames,
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
		const bool hasRequiredLayers = std::ranges::all_of(m_RequiredInstanceLayerNames,
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
}
