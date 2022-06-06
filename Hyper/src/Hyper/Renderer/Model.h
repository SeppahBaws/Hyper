#pragma once
#include <glm/vec3.hpp>

#include "Material.h"
#include "RenderContext.h"

namespace Hyper
{
	class Mesh;

	class Model
	{
	public:
		explicit Model(RenderContext* pRenderCtx, const std::filesystem::path& filePath);

		void Draw(const vk::CommandBuffer& cmd, const vk::PipelineLayout& pipelineLayout) const;

		void SetPosition(const glm::vec3& position)
		{
			m_Position = position;
		}

		void SetRotation(const glm::vec3& rotation)
		{
			m_Rotation = rotation;
		}

		void SetScale(const glm::vec3& scale)
		{
			m_Scale = scale;
		}

	private:
		void Import(const std::filesystem::path& filePath);

	private:
		RenderContext* m_pRenderCtx{};

		glm::vec3 m_Position{};
		glm::vec3 m_Rotation{};
		glm::vec3 m_Scale{ 1.0f };

		std::vector<std::unique_ptr<Mesh>> m_Meshes{};
		std::vector<Material> m_Materials{};
	};
}
