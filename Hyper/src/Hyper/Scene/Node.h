#pragma once
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace Hyper
{
	class Mesh;

	class Node
	{
	public:
		Node(const std::string& name);
		
		void Draw(const vk::CommandBuffer& cmd, const vk::PipelineLayout& pipelineLayout);
		void Update(float dt);

		void DrawImGui();

	private:
		void CalculateTransforms(bool calculateChildren = false);

	private:
		friend class Scene;

		std::string m_Name;
		u64 m_Id{};
		
		Node* m_pParent{ nullptr };
		std::vector<std::unique_ptr<Node>> m_pChildren;

		glm::vec3 m_Position{};
		glm::vec3 m_Rotation{};
		glm::vec3 m_Scale{};
		glm::mat4 m_LocalTransform{ 1.0f };
		glm::mat4 m_WorldTransform{ 1.0f };

		std::vector<std::shared_ptr<Mesh>> m_Meshes;

		bool m_TransformDirty = true;
	};
}
