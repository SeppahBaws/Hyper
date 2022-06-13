#pragma once
#include <glm/vec3.hpp>
#include "Hyper/Renderer/RenderContext.h"
#include "Hyper/Renderer/Vulkan/VulkanBuffer.h"

namespace Hyper
{
	class VulkanAccelerationStructure;
	class Model;
	
	class Scene
	{
	public:
		Scene(RenderContext* pRenderCtx);
		~Scene();

		void AddModel(const std::filesystem::path& filePath, const glm::vec3& pos = glm::vec3{ 0.0f }, const glm::vec3& rot = glm::vec3{ 0.0f },
			const glm::vec3& scale = glm::vec3{ 1.0f });

		void BuildAccelerationStructure();

		void Draw(const vk::CommandBuffer& cmd, const vk::PipelineLayout& pipelineLayout);
		void RayTrace(const vk::CommandBuffer& cmd);

	private:
		RenderContext* m_pRenderCtx;
		std::vector<Model> m_Models;

		std::unique_ptr<VulkanAccelerationStructure> m_pAcceleration;
	};
}
