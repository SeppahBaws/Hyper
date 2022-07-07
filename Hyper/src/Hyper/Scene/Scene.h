#pragma once
#include <glm/vec3.hpp>

#include "Node.h"
#include "Hyper/Renderer/Vulkan/VulkanBuffer.h"

struct aiScene;
struct aiNode;

namespace Hyper
{
	class VulkanAccelerationStructure;
	class Model;

	class Scene
	{
	public:
		Scene(RenderContext* pRenderCtx);
		~Scene();

		void ImportModel(const std::filesystem::path& filePath, const glm::vec3& pos = glm::vec3{ 0.0f }, const glm::vec3& rot = glm::vec3{ 0.0f }, const glm::vec3& scale = glm::vec3{ 1.0f });

		void BuildAccelerationStructure();

		void Draw(const vk::CommandBuffer& cmd, const vk::PipelineLayout& pipelineLayout);
		void Update(float dt);

		[[nodiscard]] VulkanAccelerationStructure* GetAccelerationStructure() const { return m_pAcceleration.get(); }

	private:
		std::unique_ptr<Node> LoadNode(const aiScene* pScene, const aiNode* pNode, const std::string& overwriteName = "");
		void LoadMaterials(const aiScene* pScene, const std::filesystem::path& filePath);

		// void AddNodeToAccel(Node* pNode);

	private:
		RenderContext* m_pRenderCtx;

		std::vector<std::unique_ptr<Node>> m_RootNodes;
		std::unique_ptr<VulkanAccelerationStructure> m_pAcceleration;

		std::vector<UUID> m_TempMaterialMappings;
	};
}
