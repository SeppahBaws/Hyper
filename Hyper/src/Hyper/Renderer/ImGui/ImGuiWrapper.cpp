#include "HyperPCH.h"
#include "ImGuiWrapper.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include "imgui_impl_vulkan_dynamic_rendering.h"
#include "Hyper/Renderer/RenderContext.h"
#include "Hyper/Renderer/Vulkan/VulkanDebug.h"
#include "Hyper/Renderer/Vulkan/VulkanDescriptors.h"
#include "Hyper/Renderer/Vulkan/VulkanUtility.h"

namespace Hyper
{
	ImGuiWrapper::ImGuiWrapper(RenderContext* pRenderCtx, GLFWwindow* window, vk::Format colorFormat, vk::Format depthFormat)
		: m_pRenderCtx(pRenderCtx)
	{
		m_pDescriptorPool = std::make_unique<DescriptorPool>(DescriptorPool::Builder(m_pRenderCtx->device)
			.AddSize(vk::DescriptorType::eSampler, 1000)
			.AddSize(vk::DescriptorType::eCombinedImageSampler, 1000)
			.AddSize(vk::DescriptorType::eSampledImage, 1000)
			.AddSize(vk::DescriptorType::eStorageImage, 1000)
			.AddSize(vk::DescriptorType::eUniformTexelBuffer, 1000)
			.AddSize(vk::DescriptorType::eStorageTexelBuffer, 1000)
			.AddSize(vk::DescriptorType::eUniformBuffer, 1000)
			.AddSize(vk::DescriptorType::eStorageBuffer, 1000)
			.AddSize(vk::DescriptorType::eUniformBufferDynamic, 1000)
			.AddSize(vk::DescriptorType::eStorageBufferDynamic, 1000)
			.AddSize(vk::DescriptorType::eInputAttachment, 1000)
			.SetMaxSets(1000)
			.Build());

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		io.FontDefault = io.Fonts->AddFontFromFileTTF("res/fonts/Roboto/Roboto-Regular.ttf", 15.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());

		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForVulkan(window, true);
		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance = m_pRenderCtx->instance;
		initInfo.PhysicalDevice = m_pRenderCtx->physicalDevice;
		initInfo.Device = m_pRenderCtx->device;
		initInfo.QueueFamily = m_pRenderCtx->graphicsQueue.familyIndex;
		initInfo.Queue = m_pRenderCtx->graphicsQueue.queue;
		initInfo.PipelineCache = VK_NULL_HANDLE;
		initInfo.DescriptorPool = m_pDescriptorPool->GetPool();
		initInfo.Subpass = 0;
		initInfo.MinImageCount = m_pRenderCtx->imagesInFlight;
		initInfo.ImageCount = m_pRenderCtx->imagesInFlight;
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.Allocator = nullptr;
		initInfo.CheckVkResultFn = [](VkResult result) { VulkanUtils::Check(result); };
		initInfo.UseDynamicRendering = true;
		initInfo.ColorAttachmentFormat = static_cast<VkFormat>(colorFormat);
		initInfo.DepthAttachmentFormat = static_cast<VkFormat>(depthFormat);
		ImGui_ImplVulkan_Init(&initInfo, nullptr);

		// Upload font
		vk::CommandBuffer cmd = m_pRenderCtx->commandPool->GetCommandBuffer();
		VulkanCommandBuffer::Begin(cmd, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		ImGui_ImplVulkan_CreateFontsTexture(cmd);

		VulkanCommandBuffer::End(cmd);
		m_pRenderCtx->graphicsQueue.Submit({}, {}, {}, cmd, {});
		m_pRenderCtx->graphicsQueue.WaitIdle();
		m_pRenderCtx->commandPool->FreeCommandBuffer(cmd);

		ImGui_ImplVulkan_DestroyFontUploadObjects();

		HPR_CORE_LOG_INFO("Created ImGui wrapper");
	}

	ImGuiWrapper::~ImGuiWrapper()
	{
		m_pDescriptorPool.reset();
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiWrapper::NewFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiWrapper::Draw(vk::CommandBuffer cmd)
	{
		VkDebug::BeginRegion(cmd, "Dear ImGui Render", glm::vec4(0.2f, 0.2f, 0.8f, 1.0f));

		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		VkDebug::EndRegion(cmd);

		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}
