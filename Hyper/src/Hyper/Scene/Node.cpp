#include "HyperPCH.h"
#include "Node.h"

#include "Hyper/Debug/Profiler.h"
#include "Hyper/Renderer/Mesh.h"
#include "Hyper/Renderer/Renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "imgui.h"

namespace Hyper
{
	Node::Node(const std::string& name)
		: m_Name(name)
	{
	}

	void Node::Draw(const vk::CommandBuffer& cmd, const vk::PipelineLayout& pipelineLayout)
	{
		HPR_PROFILE_SCOPE();

		ModelMatrixPushConst pushConst{};
		pushConst.modelMatrix = m_WorldTransform;
		cmd.pushConstants<ModelMatrixPushConst>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, pushConst);

		for (const auto& mesh : m_Meshes)
		{
			mesh->Draw(cmd);
		}

		for (const auto& child : m_pChildren)
		{
			child->Draw(cmd, pipelineLayout);
		}
	}

	void Node::Update(float /*dt*/)
	{
		if (m_TransformDirty)
		{
			CalculateTransforms();
		}
	}

	void Node::DrawImGui()
	{
		bool isEdited = false;

		ImGui::LabelText("Name", m_Name.c_str());
		if (ImGui::InputFloat3("Position", (float*)&m_Position))
			isEdited = true;
		if (ImGui::InputFloat3("Rotation", (float*)&m_Rotation))
			isEdited = true;
		if (ImGui::InputFloat3("Scale", (float*)&m_Scale))
			isEdited = true;


		if (isEdited)
			m_TransformDirty = true;
	}

	void Node::CalculateTransforms(bool calculateChildren)
	{
		m_LocalTransform = glm::translate(glm::mat4(1.0f), m_Position)
			* glm::toMat4(glm::quat(glm::radians(m_Rotation)))
			* glm::scale(glm::mat4(1.0f), m_Scale);

		m_WorldTransform = m_LocalTransform;
		if (m_pParent)
		{
			m_WorldTransform = m_pParent->m_WorldTransform * m_LocalTransform;
		}

		// Mark all children as dirty as well
		for (const auto& child : m_pChildren)
		{
			if (calculateChildren)
			{
				child->CalculateTransforms(true);
			}
			else
			{
				child->m_TransformDirty = true;
			}
		}

		m_TransformDirty = false;
	}
}
