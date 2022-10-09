#include "HyperPCH.h"
#include "ShaderLibrary.h"
#include "Vulkan/VulkanPipeline.h"

#include <imgui.h>

#include "RenderContext.h"

namespace Hyper
{
	ShaderLibrary::ShaderLibrary(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
		LoadShader("StaticGeometry", std::unordered_map<ShaderStageType, std::filesystem::path>{
			{ ShaderStageType::Vertex, "res/shaders/StaticGeometry.vert.hlsl" },
			{ ShaderStageType::Fragment, "res/shaders/StaticGeometry.frag.hlsl" }
		});

		LoadShader("Composite", std::unordered_map<ShaderStageType, std::filesystem::path>{
			{ ShaderStageType::Vertex, "res/shaders/Composite.vert.hlsl" },
			{ ShaderStageType::Fragment, "res/shaders/Composite.frag.hlsl" },
		});

		LoadShader("RTShadows", std::unordered_map<ShaderStageType, std::filesystem::path>{
			{ ShaderStageType::RayGen, "res/shaders/RTShadows.rgen.hlsl" },
			{ ShaderStageType::Miss, "res/shaders/RTShadows.rmiss.hlsl" },
			{ ShaderStageType::ClosestHit, "res/shaders/RTShadows.rchit.hlsl" },
		});
	}

	ShaderLibrary::~ShaderLibrary()
	{
		m_LoadedShaders.clear();
	}

	VulkanShader* ShaderLibrary::GetShader(const std::string& shaderName) const
	{
		if (!m_LoadedShaders.contains(shaderName))
		{
			throw std::runtime_error(fmt::format("Failed to get shader '{}' - it isn't loaded.", shaderName));
		}

		return m_LoadedShaders.at(shaderName).get();
	}

	void ShaderLibrary::RegisterShaderDependency(const UUID& shaderId, VulkanGraphicsPipeline* graphicsPipeline)
	{
		m_ShaderDependencies[shaderId].graphicsPipelines.push_back(graphicsPipeline);
	}

	void ShaderLibrary::RegisterShaderDependency(const UUID& shaderId, VulkanRayTracingPipeline* rtPipeline)
	{
		m_ShaderDependencies[shaderId].rayTracingPipelines.push_back(rtPipeline);
	}

	void ShaderLibrary::UnRegisterShaderDependency(const UUID& shaderId, VulkanGraphicsPipeline* graphicsPipeline)
	{
		std::erase(m_ShaderDependencies[shaderId].graphicsPipelines, graphicsPipeline);
	}

	void ShaderLibrary::UnRegisterShaderDependency(const UUID& shaderId, VulkanRayTracingPipeline* rtPipeline)
	{
		std::erase(m_ShaderDependencies[shaderId].rayTracingPipelines, rtPipeline);
	}

	void ShaderLibrary::DrawImGui()
	{
		if (m_pRenderCtx->drawImGui)
		{
			if (ImGui::Begin("Shader Library"))
			{
				if (ImGui::BeginTable("Loaded shaders", 2))
				{
					for (auto& [name, pShader] : m_LoadedShaders)
					{
						ImGui::TableNextRow();

						ImGui::TableSetColumnIndex(0);
						ImGui::Text(name.c_str());

						ImGui::TableSetColumnIndex(1);
						if (ImGui::Button(fmt::format("Reload##{}", name).c_str()))
						{
							pShader->Reload();
							for (VulkanGraphicsPipeline* graphicsPipeline : m_ShaderDependencies[pShader->GetId()].graphicsPipelines)
							{
								graphicsPipeline->Recreate();
							}
							for (VulkanRayTracingPipeline* rtPipeline : m_ShaderDependencies[pShader->GetId()].rayTracingPipelines)
							{
								rtPipeline->Recreate();
							}
						}
					}
				}
				ImGui::EndTable();
			}
			ImGui::End();
		}
	}

	void ShaderLibrary::LoadShader(const std::string& name, const std::unordered_map<ShaderStageType, std::filesystem::path>& stages)
	{
		m_LoadedShaders[name] = std::make_unique<VulkanShader>(
			m_pRenderCtx,
			stages);
	}
}
