#pragma once
#include "Vulkan/VulkanShader.h"

namespace Hyper
{
	class VulkanGraphicsPipeline;
	class VulkanRayTracingPipeline;
	struct RenderContext;
	class VulkanShader;

	struct ShaderDependencies
	{
		std::vector<VulkanGraphicsPipeline*> graphicsPipelines;
		std::vector<VulkanRayTracingPipeline*> rayTracingPipelines;
	};

	class ShaderLibrary
	{
	public:
		ShaderLibrary(RenderContext* pRenderCtx);
		~ShaderLibrary();

		VulkanShader* GetShader(const std::string& shaderName) const;
		void RegisterShaderDependency(const UUID& shaderId, VulkanGraphicsPipeline* graphicsPipeline);
		void RegisterShaderDependency(const UUID& shaderId, VulkanRayTracingPipeline* rtPipeline);

		void UnRegisterShaderDependency(const UUID& shaderId, VulkanGraphicsPipeline* graphicsPipeline);
		void UnRegisterShaderDependency(const UUID& shaderId, VulkanRayTracingPipeline* rtPipeline);

		void DrawImGui();

	private:
		void LoadShader(const std::string& name, const std::unordered_map<ShaderStageType, std::filesystem::path>& stages);

	private:
		RenderContext* m_pRenderCtx;

		std::unordered_map<std::string, std::unique_ptr<VulkanShader>> m_LoadedShaders;
		std::unordered_map<UUID, ShaderDependencies> m_ShaderDependencies;
	};
}
