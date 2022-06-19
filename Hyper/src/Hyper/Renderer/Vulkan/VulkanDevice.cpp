#include "HyperPCH.h"
#include "VulkanDevice.h"

#include "VulkanDebug.h"
#include "VulkanExtensions.h"
#include "VulkanUtility.h"

namespace Hyper
{
	VulkanDevice::VulkanDevice(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
#ifdef HYPER_USE_AFTERMATH
		, m_MarkerMap{}
		, m_GpuCrashTracker{m_MarkerMap}
#endif
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

			pRenderCtx->instance = VulkanUtils::Check(vk::createInstance(createInfo));

			HPR_VKLOG_INFO("Created Vulkan Instance");
		}

		if (HYPER_VALIDATE)
		{
			// Setup Vulkan debug utility
			VkDebug::Setup(pRenderCtx);
		}

#ifdef HYPER_USE_AFTERMATH
		// Initialize the Aftermath GPU crash tracker
		m_GpuCrashTracker.Initialize();
#endif

		// Create device
		{
			// Find physical device
			auto availableDevices = VulkanUtils::Check(pRenderCtx->instance.enumeratePhysicalDevices());
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

			vk::PhysicalDeviceProperties2 properties{};
			properties.pNext = &pRenderCtx->rtProperties;
			pRenderCtx->physicalDevice.getProperties2(&properties);

			HPR_VKLOG_INFO("Found a physical device");

			std::vector<vk::QueueFamilyProperties> queueFamilyProperties = pRenderCtx->physicalDevice.getQueueFamilyProperties();

			// Get the first QueueFamily which supports graphics and compute
			const auto graphicsPropertyIterator = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
				[](const vk::QueueFamilyProperties& qfp)
				{
					return qfp.queueFlags & (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute);
				});

			u32 graphicsQueueFamilyIndex = static_cast<u32>(std::distance(queueFamilyProperties.begin(), graphicsPropertyIterator));
			assert(graphicsQueueFamilyIndex < queueFamilyProperties.size());

			// Create a device and retrieve the queues
			constexpr f32 queuePriority = 0.0f;
			std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
			// Graphics queue
			queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo{{}, graphicsQueueFamilyIndex, 1, &queuePriority});

			auto deviceCreateInfoChain = vk::StructureChain<
				vk::DeviceCreateInfo,
				// Dynamic rendering
				vk::PhysicalDeviceDynamicRenderingFeatures,
				// Ray-tracing features
				vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
				vk::PhysicalDeviceBufferDeviceAddressFeatures,
				vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
				// Device diagnostics for Nvidia Aftermath
				// TODO: make this an optional feature
				vk::DeviceDiagnosticsConfigCreateInfoNV
			>();

			// Enable dynamic rendering
			deviceCreateInfoChain.get<vk::PhysicalDeviceDynamicRenderingFeatures>().dynamicRendering = true;

			// Enable ray tracing features
			deviceCreateInfoChain.get<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>().rayTracingPipeline = true;
			deviceCreateInfoChain.get<vk::PhysicalDeviceBufferDeviceAddressFeatures>().bufferDeviceAddress = true;
			deviceCreateInfoChain.get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>().accelerationStructure = true;

			// Enable device diagnostics
			const vk::DeviceDiagnosticsConfigFlagsNV aftermathFlags =
				vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableResourceTracking |
				vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableAutomaticCheckpoints |
				vk::DeviceDiagnosticsConfigFlagBitsNV::eEnableShaderDebugInfo;
			deviceCreateInfoChain.get<vk::DeviceDiagnosticsConfigCreateInfoNV>().flags = aftermathFlags;

			auto& deviceCreateInfo = deviceCreateInfoChain.get<vk::DeviceCreateInfo>()
				.setQueueCreateInfos(queueCreateInfos)
				.setPEnabledLayerNames(m_RequiredDeviceLayerNames)
				.setPEnabledExtensionNames(m_RequiredDeviceExtensionNames);
			pRenderCtx->device = VulkanUtils::Check(pRenderCtx->physicalDevice.createDevice(deviceCreateInfo));

			HPR_VKLOG_INFO("Created the logical device");


			m_GraphicsQueue = VulkanQueue{
				.queue = pRenderCtx->device.getQueue(graphicsQueueFamilyIndex, 0),
				.familyIndex = graphicsQueueFamilyIndex,
				.flags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute
			};
			pRenderCtx->graphicsQueue = m_GraphicsQueue;

			HPR_VKLOG_INFO("Got the graphics queue");
		}

		// Init VMA
		{
			VmaAllocatorCreateInfo createInfo{};
			createInfo.vulkanApiVersion = VK_API_VERSION_1_3;
			createInfo.physicalDevice = pRenderCtx->physicalDevice;
			createInfo.device = pRenderCtx->device;
			createInfo.instance = pRenderCtx->instance;
			createInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
			
			VulkanUtils::Check(vmaCreateAllocator(&createInfo, &m_Allocator));
			m_pRenderCtx->allocator = m_Allocator;
		}

		// Load extensions
		LoadVkExtensions(m_pRenderCtx->instance, vkGetInstanceProcAddr, m_pRenderCtx->device, vkGetDeviceProcAddr);
	}

	VulkanDevice::~VulkanDevice()
	{
		vmaDestroyAllocator(m_Allocator);

		m_pRenderCtx->device.destroy();

		// We only have debug callbacks if we have validation layers enabled.
		if (HYPER_VALIDATE)
		{
			VkDebug::FreeDebugCallback(m_pRenderCtx->instance);
		}

		m_pRenderCtx->instance.destroy();
	}

	bool VulkanDevice::CheckExtensionsAndLayersSupport() const
	{
		std::vector availableExtensions = VulkanUtils::Check(vk::enumerateInstanceExtensionProperties());
		std::vector availableLayers = VulkanUtils::Check(vk::enumerateInstanceLayerProperties());

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
