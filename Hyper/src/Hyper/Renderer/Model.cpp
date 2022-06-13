#include "HyperPCH.h"
#include "Model.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Material.h"
#include "Mesh.h"
#include "Renderer.h"
#include "Hyper/Debug/Profiler.h"

namespace Hyper
{
	Model::Model(RenderContext* pRenderCtx, const std::filesystem::path& path)
		: m_pRenderCtx(pRenderCtx)
	{
		if (!std::filesystem::exists(path))
		{
			HPR_CORE_LOG_ERROR("Cannot load model '{}' - it does not exist!", path.string());
			return;
		}

		Import(path);
	}

	Model::Model(Model&& other) noexcept: m_pRenderCtx(other.m_pRenderCtx),
		m_Position(std::move(other.m_Position)),
		m_Rotation(std::move(other.m_Rotation)),
		m_Scale(std::move(other.m_Scale)),
		m_Meshes(std::move(other.m_Meshes)),
		m_Materials(std::move(other.m_Materials))
	{
	}

	Model& Model::operator=(Model&& other) noexcept
	{
		if (this == &other)
			return *this;
		m_pRenderCtx = other.m_pRenderCtx;
		m_Position = std::move(other.m_Position);
		m_Rotation = std::move(other.m_Rotation);
		m_Scale = std::move(other.m_Scale);
		m_Meshes = std::move(other.m_Meshes);
		m_Materials = std::move(other.m_Materials);
		return *this;
	}

	void Model::Draw(const vk::CommandBuffer& cmd, const vk::PipelineLayout& pipelineLayout) const
	{
		HPR_PROFILE_SCOPE();

		// Upload push const
		ModelMatrixPushConst pushConst{};
		pushConst.modelMatrix = glm::translate(glm::mat4(1.0f), m_Position)
			* glm::toMat4(glm::quat(glm::radians(m_Rotation)))
			* glm::scale(glm::mat4(1.0f), m_Scale);
		cmd.pushConstants<ModelMatrixPushConst>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, pushConst);

		u32 currentMaterial = 0;
		m_Materials[currentMaterial].Bind(cmd, pipelineLayout);

		for (const std::unique_ptr<Mesh>& pMesh : m_Meshes)
		{
			const u32 meshMat = pMesh->GetMaterialIdx();
			if (meshMat != currentMaterial)
			{
				currentMaterial = meshMat;
				m_Materials[meshMat].Bind(cmd, pipelineLayout);
			}

			pMesh->Draw(cmd);
		}
	}

	void Model::Import(const std::filesystem::path& path)
	{
		HPR_CORE_LOG_INFO("Importing model '{}'...", path.filename().string());

		Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_OptimizeMeshes | aiProcess_ImproveCacheLocality);
		if (!scene)
		{
			HPR_CORE_LOG_ERROR("Failed to import model '{}' : {}", path.string(), importer.GetErrorString());
			return;
		}

		// Load materials
		for (u32 m = 0; m < scene->mNumMaterials; m++)
		{
			aiString texturePath;

			const aiMaterial* pMat = scene->mMaterials[m];
			const std::string materialName = pMat->GetName().C_Str();

			HPR_CORE_LOG_DEBUG("Material '{}'", pMat->GetName().C_Str());
			// for (u32 p = 0; p < pMat->mNumProperties; p++)
			// {
			// 	const aiMaterialProperty* property = pMat->mProperties[p];
			// 	// property->mKey
			// 	// pMat->Get(aimatkey)
			// 	HPR_CORE_LOG_INFO("  Material property: {}", property->mKey.C_Str());
			// }

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

			// Material material{ m_pRenderCtx, materialName };
			Material& material = m_Materials.emplace_back(m_pRenderCtx, materialName);

			if (AI_SUCCESS == pMat->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath))
			{
				const auto absoluteTexturePath = path.parent_path() / std::filesystem::path(texturePath.C_Str());
				material.LoadTexture(MaterialTextureType::Albedo, absoluteTexturePath);
			}
			else
			{
				material.LoadTexture(MaterialTextureType::Albedo, "res/textures/default-white.png");
			}

			if (AI_SUCCESS == pMat->GetTexture(aiTextureType_NORMALS, 0, &texturePath))
			{
				const auto absoluteTexturePath = path.parent_path().append(texturePath.C_Str());
				material.LoadTexture(MaterialTextureType::Normal, absoluteTexturePath);
			}
			else
			{
				material.LoadTexture(MaterialTextureType::Normal, "res/textures/default-normal.png");
			}

			material.PostLoadInititalize();
			// m_Materials.push_back(std::move(material));
		}

		// Load meshes
		for (u32 m = 0; m < scene->mNumMeshes; m++)
		{
			std::vector<VertexPosNormTex> vertices;
			std::vector<u32> indices;

			const aiMesh* mesh = scene->mMeshes[m];

			// Load vertices
			for (u32 v = 0; v < mesh->mNumVertices; v++)
			{
				// Positions are guaranteed, the rest is uncertain
				const glm::vec3 pos{ mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };
				glm::vec3 norm{ 0.0f, 0.0f, 1.0f };
				glm::vec2 tex{ 0.0f, 0.0f };

				if (mesh->HasNormals())
				{
					norm = { mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z };
				}
				if (mesh->HasTextureCoords(0))
				{
					tex = { mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y };
				}

				vertices.emplace_back(VertexPosNormTex{ pos, norm, tex });
			}

			// Load indices
			for (u32 f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace face = mesh->mFaces[f];
				for (u32 i = 0; i < face.mNumIndices; i++)
				{
					indices.emplace_back(face.mIndices[i]);
				}
			}

			m_Meshes.emplace_back(std::make_unique<Mesh>(m_pRenderCtx, mesh->mMaterialIndex, vertices, indices, mesh->mNumFaces));
		}

		// Sort meshes based on their material idx
		std::ranges::sort(m_Meshes, [](const std::unique_ptr<Mesh>& left, const std::unique_ptr<Mesh>& right)
		{
			return left->GetMaterialIdx() < right->GetMaterialIdx();
		});

		HPR_CORE_LOG_INFO("Successfully imported model '{}'!", path.filename().string());
	}
}
