#include "HyperPCH.h"
#include "Scene.h"

#include "Hyper/Renderer/Model.h"
#include "Hyper/Renderer/Vulkan/VulkanAccelerationStructure.h"

namespace Hyper
{
	Scene::Scene(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
	}

	Scene::~Scene()
	{
		m_pAcceleration.reset();
		m_Models.clear();
	}

	void Scene::AddModel(const std::filesystem::path& filePath, const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale)
	{
		Model model{m_pRenderCtx, filePath};
		model.SetPosition(pos);
		model.SetRotation(rot);
		model.SetScale(scale);

		m_Models.push_back(std::move(model));
	}

	void Scene::BuildAccelerationStructure()
	{
		m_pAcceleration = std::make_unique<VulkanAccelerationStructure>(m_pRenderCtx);
		for (const auto& model : m_Models)
		{
			for (auto& mesh : model.GetMeshes())
			{
				m_pAcceleration->AddMesh(mesh.get(), model.GetTransformMatrix());
			}
		}

		m_pAcceleration->Build();
	}

	void Scene::Draw(const vk::CommandBuffer& cmd, const vk::PipelineLayout& pipelineLayout)
	{
		for (const auto& model : m_Models)
		{
			model.Draw(cmd, pipelineLayout);
		}
	}
}
