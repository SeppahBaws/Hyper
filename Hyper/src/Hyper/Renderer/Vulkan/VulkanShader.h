#pragma once
#include <vulkan/vulkan.hpp>

#include "Hyper/Renderer/RenderContext.h"

namespace Hyper
{
	enum class ShaderStageType : VkShaderStageFlags
	{
		Vertex = VK_SHADER_STAGE_VERTEX_BIT,
		Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
	};

	class ShaderModule
	{
	public:
		ShaderModule(RenderContext* pRenderCtx, ShaderStageType type, const std::filesystem::path& filePath);
		~ShaderModule();

		// Disallow copying
		ShaderModule(const ShaderModule& other) = delete;
		ShaderModule& operator=(const ShaderModule& other) = delete;
		// Custom 
		ShaderModule(ShaderModule&& other);
		ShaderModule& operator=(ShaderModule&& other);

		[[nodiscard]] vk::ShaderModule GetShaderModule() const
		{ return m_Module; }

		[[nodiscard]] vk::PipelineShaderStageCreateInfo GetShaderInfo() const
		{ return m_ShaderInfo; }

	private:
		RenderContext* m_pRenderCtx;

		ShaderStageType m_Type{};
		std::filesystem::path m_FilePath;

		vk::ShaderModule m_Module{};
		vk::PipelineShaderStageCreateInfo m_ShaderInfo{};
	};

	class VulkanShader
	{
	public:
		VulkanShader(RenderContext* pRenderCtx);

		void AddStage(ShaderStageType type, const std::filesystem::path& filePath);

		[[nodiscard]] std::vector<vk::PipelineShaderStageCreateInfo> GetAllShaderStages() const;
	
	private:
		RenderContext* m_pRenderCtx;

		std::unordered_map<ShaderStageType, ShaderModule> m_ShaderModules;
	};
}
