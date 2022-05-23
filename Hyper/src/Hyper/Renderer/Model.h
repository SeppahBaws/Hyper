#pragma once
#include <glm/vec3.hpp>

#include "RenderContext.h"

namespace Hyper
{
	class Mesh;

	class Model
	{
	public:
		explicit Model(RenderContext* pRenderCtx, const std::filesystem::path& filePath);

		void Draw(const vk::CommandBuffer& cmd, const vk::PipelineLayout& layout) const;

	private:
		void Import(const std::filesystem::path& filePath);

	private:
		RenderContext* m_pRenderCtx{};

		glm::vec3 m_Position{};
		glm::vec3 m_Rotation{ 90.0f, 0.0f, 0.0f };
		glm::vec3 m_Scale{ 0.01f };

		std::vector<std::unique_ptr<Mesh>> m_Meshes{};
	};
}
