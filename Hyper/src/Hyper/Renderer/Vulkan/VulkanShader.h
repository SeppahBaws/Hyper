#pragma once
#include <vulkan/vulkan.hpp>

#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	enum class ShaderStageType : VkShaderStageFlags
	{
		Vertex = vk::ShaderStageFlagBits::eVertex,
		Fragment = vk::ShaderStageFlagBits::eFragment,
	};

	struct ShaderModule
	{
		std::filesystem::path shaderPath;
		vk::ShaderModule module;
	};

	class VulkanShader
	{
	public:
		VulkanShader(RenderContext* pRenderCtx, std::unordered_map<ShaderStageType, std::filesystem::path> shaders);
		~VulkanShader();

		[[nodiscard]] std::vector<vk::PipelineShaderStageCreateInfo> GetAllShaderStages() const;

	private:
		vk::ShaderModule CompileStage(ShaderStageType stage, const std::filesystem::path& filePath);

	private:
		RenderContext* m_pRenderCtx;

		std::unordered_map<ShaderStageType, ShaderModule> m_ShaderModules;
	};
}
