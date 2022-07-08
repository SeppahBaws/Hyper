#include "HyperPCH.h"
#include "Scene.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include "imgui.h"
#include "Hyper/Core/Context.h"
#include "Hyper/Renderer/MaterialLibrary.h"
#include "Hyper/Renderer/Mesh.h"
#include "Hyper/Renderer/RenderContext.h"
#include "Hyper/Renderer/Renderer.h"
#include "Hyper/Renderer/Vulkan/Vertex.h"
#include "Hyper/Renderer/Vulkan/VulkanAccelerationStructure.h"

namespace Hyper
{
	Scene::Scene(Context* pContext)
		: Subsystem(pContext)
		, m_pRenderCtx(nullptr)
	{
	}

	Scene::~Scene()
	{
	}

	void Scene::ImportModel(const std::filesystem::path& filePath, const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale)
	{
		Assimp::Importer importer;

		HPR_CORE_LOG_INFO("Loading file '{}'", filePath.string());
		const aiScene* scene = importer.ReadFile(filePath.string(), aiProcess_Triangulate | aiProcess_OptimizeMeshes | aiProcess_ImproveCacheLocality);
		if (!scene)
		{
			HPR_CORE_LOG_ERROR("Failed to import model '{}' : {}", filePath.string(), importer.GetErrorString());
			return;
		}
		HPR_CORE_LOG_INFO("File loaded!");

		static const auto toString = [](aiMetadataType type)
		{
			switch (type)
			{
			case AI_BOOL: return "AI_BOOL";
			case AI_INT32: return "AI_INT32";
			case AI_UINT64: return "AI_UINT64";
			case AI_FLOAT: return "AI_FLOAT";
			case AI_DOUBLE: return "AI_DOUBLE";
			case AI_AISTRING: return "AI_AISTRING";
			case AI_AIVECTOR3D: return "AI_AIVECTOR3D";
			case AI_AIMETADATA: return "AI_AIMETADATA";
			case AI_META_MAX: return "AI_META_MAX";
			case FORCE_32BIT: return "FORCE_32BIT";
			}
		};

		HPR_CORE_LOG_INFO("Assimp Scene Meta Data:");
		for (u32 m = 0; m < scene->mMetaData->mNumProperties; m++)
		{
			const auto key = scene->mMetaData->mKeys[m];
			const auto type = toString(scene->mMetaData->mValues[m].mType);
			HPR_CORE_LOG_TRACE("  {} ({})", key.C_Str(), type);
		}

		m_TempMaterialMappings.clear();
		LoadMaterials(scene, filePath);

		m_RootNodes.push_back(LoadNode(scene, scene->mRootNode, filePath.filename().string()));
		m_RootNodes.back()->m_Position = pos;
		m_RootNodes.back()->m_Rotation = rot;
		m_RootNodes.back()->m_Scale = scale;
		m_RootNodes.back()->CalculateTransforms(true);
	}

	void Scene::BuildAccelerationStructure()
	{
		m_pAcceleration = std::make_unique<VulkanAccelerationStructure>(m_pRenderCtx);

		std::function<void(Node*)> addNodeToAccel = [&](Node* pNode)
		{
			// Add node's meshes
			u32 meshIdx = 0;
			for (const auto& mesh : pNode->m_Meshes)
			{
				glm::mat4 transform = pNode->m_LocalTransform;
				if (pNode->m_pParent)
				{
					transform = pNode->m_pParent->m_WorldTransform * transform;
				}

				std::string debugName = pNode->m_Name;
				if (pNode->m_Meshes.size() > 1)
				{
					debugName = fmt::format("{} ({})", debugName, meshIdx);
				}
				m_pAcceleration->AddMesh(mesh.get(), transform, debugName);

				meshIdx++;
			}

			// Add children
			for (const auto& child : pNode->m_pChildren)
			{
				addNodeToAccel(child.get());
			}
		};

		for (const auto& node : m_RootNodes)
		{
			addNodeToAccel(node.get());
		}

		m_pAcceleration->Build();
	}

	static Node* selectedNode = nullptr;
	void Scene::Draw(const vk::CommandBuffer& cmd, const vk::PipelineLayout& pipelineLayout) const
	{
		for (const auto& node : m_RootNodes)
		{
			node->Draw(m_pRenderCtx, cmd, pipelineLayout);
		}

		if (ImGui::Begin("Scene hierarchy"))
		{
			static std::function<void(Node*)> drawNode = [&](Node* pNode)
			{
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
				if (selectedNode == pNode)
					flags |= ImGuiTreeNodeFlags_Selected;

				if (pNode->m_pChildren.empty())
				{
					flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

					ImGui::TreeNodeEx(pNode->m_Name.c_str(), flags);
					if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
						selectedNode = pNode;
				}
				else if (ImGui::TreeNodeEx(pNode->m_Name.c_str(), flags))
				{
					if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
						selectedNode = pNode;

					for (const auto& child : pNode->m_pChildren)
					{
						drawNode(child.get());
					}

					ImGui::TreePop();
				}
			};

			for (const auto& node : m_RootNodes)
			{
				drawNode(node.get());
			}
			ImGui::End();
		}

		if (ImGui::Begin("Node inspector"))
		{
			if (selectedNode)
			{
				selectedNode->DrawImGui();
			}
			else
			{
				ImGui::Text("Select a node first");
			}
			ImGui::End();
		}
	}

	bool Scene::OnInitialize()
	{
		const Renderer* pRenderer = m_pContext->GetSubsystem<Renderer>();

		m_pRenderCtx = pRenderer->GetRenderContext();

		// ImportModel("res/NewSponza/Main/NewSponza_Main_Blender_glTF.gltf", glm::vec3{ 0.0f }, glm::vec3{ 90.0f, 0.0f, 0.0f });
		// ImportModel("res/NewSponza/PKG_A_Curtains/NewSponza_Curtains_glTF.gltf", glm::vec3{ 0.0f }, glm::vec3{ 90.0f, 0.0f, 0.0f });
		ImportModel("res/models/Sponza/Sponza.gltf", glm::vec3{ 0.0f }, glm::vec3{ 90.0f, 0.0f, 0.0f }, glm::vec3{ 0.01f });

		BuildAccelerationStructure();

		return true;
	}

	void Scene::OnShutdown()
	{
		// Wait till the renderer is done processing all render commands.
		m_pContext->GetSubsystem<Renderer>()->WaitIdle();

		m_pAcceleration.reset();
		m_RootNodes.clear();
	}

	void Scene::OnTick(f32 dt)
	{
		std::function<void(Node*)> updateNode = [&](Node* pNode)
		{
			pNode->Update(dt);

			for (const auto& pChild : pNode->m_pChildren)
			{
				updateNode(pChild.get());
			}
		};

		for (const auto& node : m_RootNodes)
		{
			updateNode(node.get());
		}
	}

	static u64 nodeId = 0;
	std::unique_ptr<Node> Scene::LoadNode(const aiScene* pScene, const aiNode* pNode, const std::string& overwriteName)
	{
		std::string nodeName = pNode->mName.C_Str();
		if (!overwriteName.empty())
		{
			nodeName = overwriteName;
		}
		std::unique_ptr<Node> node = std::make_unique<Node>(nodeName);
		node->m_Id = ++nodeId;

		auto t = pNode->mTransformation;
		glm::mat4 transform = glm::mat4{
			{ t.a1, t.b1, t.c1, t.d1 },
			{ t.a2, t.b2, t.c2, t.d2 },
			{ t.a3, t.b3, t.c3, t.d3 },
			{ t.a4, t.b4, t.c4, t.d4 },
		};
		node->m_TransformDirty = true;

		glm::vec3 translation;
		glm::quat orientation;
		glm::vec3 scale;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transform, scale, orientation, translation, skew, perspective);

		// Transform vertices from assimp's system to ours.
		// Assimp uses right-handed y up, z out of screen: https://assimp-docs.readthedocs.io/en/master/usage/use_the_lib.html#introduction
		node->m_Position = translation;
		node->m_Rotation = glm::degrees(glm::eulerAngles(orientation));
		node->m_Scale = scale;

		for (u32 m = 0; m < pNode->mNumMeshes; m++)
		{
			std::vector<VertexPosNormTex> vertices{};
			std::vector<u32> indices{};

			const u32 meshId = pNode->mMeshes[m];
			const auto aiMesh = pScene->mMeshes[meshId];
			const UUID materialId = m_TempMaterialMappings[aiMesh->mMaterialIndex];

			// Load vertices
			for (u32 v = 0; v < aiMesh->mNumVertices; v++)
			{
				// Positions are guaranteed, the rest is uncertain
				const glm::vec3 pos{ aiMesh->mVertices[v].x, aiMesh->mVertices[v].y, aiMesh->mVertices[v].z };
				glm::vec3 norm{ 0.0f, 0.0f, 1.0f };
				glm::vec2 tex{ 0.0f, 0.0f };

				if (aiMesh->HasNormals())
				{
					norm = { aiMesh->mNormals[v].x, aiMesh->mNormals[v].y, aiMesh->mNormals[v].z };
				}
				if (aiMesh->HasTextureCoords(0))
				{
					tex = { aiMesh->mTextureCoords[0][v].x, aiMesh->mTextureCoords[0][v].y };
				}

				vertices.emplace_back(VertexPosNormTex{ pos, norm, tex });
			}

			// Load indices
			for (u32 f = 0; f < aiMesh->mNumFaces; f++)
			{
				const aiFace face = aiMesh->mFaces[f];
				for (u32 i = 0; i < face.mNumIndices; i++)
				{
					indices.emplace_back(face.mIndices[i]);
				}
			}

			node->m_Meshes.push_back(std::make_shared<Mesh>(m_pRenderCtx, materialId, vertices, indices, aiMesh->mNumFaces));
		}

		for (u32 c = 0; c < pNode->mNumChildren; c++)
		{
			node->m_pChildren.push_back(LoadNode(pScene, pNode->mChildren[c]));
			node->m_pChildren[node->m_pChildren.size() - 1]->m_pParent = node.get();
		}


		return node;
	}

	void Scene::LoadMaterials(const aiScene* pScene, const std::filesystem::path& filePath)
	{
		MaterialLibrary* materialLibrary = m_pRenderCtx->pMaterialLibrary;

		m_TempMaterialMappings.resize(pScene->mNumMaterials);

		for (u32 m = 0; m < pScene->mNumMaterials; m++)
		{
			aiString texturePath;

			const aiMaterial* pMat = pScene->mMaterials[m];
			const std::string materialName = pMat->GetName().C_Str();

			HPR_CORE_LOG_DEBUG("Material '{}'", pMat->GetName().C_Str());

			auto getTextureType = [](u32 number)
			{
				switch (number)
				{
				case aiTextureType_NONE: return "aiTextureType_NONE";
				case aiTextureType_DIFFUSE: return "aiTextureType_DIFFUSE";
				case aiTextureType_SPECULAR: return "aiTextureType_SPECULAR";
				case aiTextureType_AMBIENT: return "aiTextureType_AMBIENT";
				case aiTextureType_EMISSIVE: return "aiTextureType_EMISSIVE";
				case aiTextureType_HEIGHT: return "aiTextureType_HEIGHT";
				case aiTextureType_NORMALS: return "aiTextureType_NORMALS";
				case aiTextureType_SHININESS: return "aiTextureType_SHININESS";
				case aiTextureType_OPACITY: return "aiTextureType_OPACITY";
				case aiTextureType_DISPLACEMENT: return "aiTextureType_DISPLACEMENT";
				case aiTextureType_LIGHTMAP: return "aiTextureType_LIGHTMAP";
				case aiTextureType_REFLECTION: return "aiTextureType_REFLECTION";
				case aiTextureType_BASE_COLOR: return "aiTextureType_BASE_COLOR";
				case aiTextureType_NORMAL_CAMERA: return "aiTextureType_NORMAL_CAMERA";
				case aiTextureType_EMISSION_COLOR: return "aiTextureType_EMISSION_COLOR";
				case aiTextureType_METALNESS: return "aiTextureType_METALNESS";
				case aiTextureType_DIFFUSE_ROUGHNESS: return "aiTextureType_DIFFUSE_ROUGHNESS";
				case aiTextureType_AMBIENT_OCCLUSION: return "aiTextureType_AMBIENT_OCCLUSION";
				case aiTextureType_SHEEN: return "aiTextureType_SHEEN";
				case aiTextureType_CLEARCOAT: return "aiTextureType_CLEARCOAT";
				case aiTextureType_TRANSMISSION: return "aiTextureType_TRANSMISSION";
				case aiTextureType_UNKNOWN: return "aiTextureType_UNKNOWN";
				}

				return "unknown value";
			};

			aiString tempPath;
			for (u32 t = 0; t < aiTextureType_UNKNOWN; t++)
			{
				auto textureType = getTextureType(t);
				if (AI_SUCCESS == pMat->GetTexture(static_cast<aiTextureType>(t), 0, &tempPath))
				{
					HPR_CORE_LOG_DEBUG("  {} - {}", textureType, std::string(tempPath.C_Str()));
				}
			}

			Material& material = materialLibrary->CreateMaterial(materialName);
			m_TempMaterialMappings[m] = material.GetId();
			HPR_CORE_LOG_INFO("Created material with id '{}'", material.GetId());

			if (AI_SUCCESS == pMat->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath))
			{
				const auto absoluteTexturePath = filePath.parent_path() / std::filesystem::path(texturePath.C_Str());
				material.LoadTexture(MaterialTextureType::Albedo, absoluteTexturePath);
			}
			else
			{
				material.LoadTexture(MaterialTextureType::Albedo, "res/textures/default-white.png");
			}

			if (AI_SUCCESS == pMat->GetTexture(aiTextureType_NORMALS, 0, &texturePath))
			{
				const auto absoluteTexturePath = filePath.parent_path().append(texturePath.C_Str());
				material.LoadTexture(MaterialTextureType::Normal, absoluteTexturePath);
			}
			else
			{
				material.LoadTexture(MaterialTextureType::Normal, "res/textures/default-normal.png");
			}

			material.PostLoadInititalize();
		}
	}
}
