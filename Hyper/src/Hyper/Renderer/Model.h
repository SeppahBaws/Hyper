#pragma once
#include <glm/vec3.hpp>

#include "Material.h"
#include "Mesh.h"
#include "RenderContext.h"

namespace Hyper
{
	class Model
	{
	public:
		explicit Model(RenderContext* pRenderCtx, const std::filesystem::path& filePath);
		Model(const Model& other) = delete;
		Model& operator=(const Model& other) = delete;
		Model(Model&& other) noexcept;
		Model& operator=(Model&& other) noexcept;

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

		const std::vector<std::unique_ptr<Mesh>>& GetMeshes() const
		{
			return m_Meshes;
		}

		glm::mat4 GetTransformMatrix() const;

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
