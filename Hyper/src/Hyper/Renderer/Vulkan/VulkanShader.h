#pragma once
#include <vulkan/vulkan.hpp>

#include "VulkanDescriptors.h"
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

	struct ShaderDescriptorBinding
	{
		vk::DescriptorType descType;
		u32 binding;
		vk::ShaderStageFlags stageFlags;
	};

	struct ShaderPushConstant
	{
		u32 offset;
		u32 size;
		vk::ShaderStageFlags stageFlags;
	};

	class VulkanShader
	{
	public:
		VulkanShader(RenderContext* pRenderCtx, std::unordered_map<ShaderStageType, std::filesystem::path> shaders);
		~VulkanShader();

		[[nodiscard]] std::vector<vk::PipelineShaderStageCreateInfo> GetAllShaderStages() const;

		[[nodiscard]] const std::vector<vk::DescriptorSetLayout>& GetAllDescriptorSetLayouts() const { return m_DescriptorLayouts; }
		[[nodiscard]] const std::vector<vk::PushConstantRange>& GetAllPushConstantRanges() const { return m_PushConstRanges; }

	private:
		std::pair<vk::ShaderModule, std::vector<u32>> CompileStage(ShaderStageType stage, const std::filesystem::path& filePath);
		void Reflect(ShaderStageType stage, const std::vector<u32>& spirvBytes);

	private:
		RenderContext* m_pRenderCtx;

		std::unordered_map<ShaderStageType, ShaderModule> m_ShaderModules;

		std::vector<std::vector<ShaderDescriptorBinding>> m_DescriptorSetBindings;
		std::vector<ShaderPushConstant> m_PushConstants;

		std::vector<vk::DescriptorSetLayout> m_DescriptorLayouts;
		std::vector<vk::PushConstantRange> m_PushConstRanges;
	};
}
