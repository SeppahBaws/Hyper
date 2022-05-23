#include "HyperPCH.h"
#include "Model.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

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

	void Model::Draw(const vk::CommandBuffer& cmd, const vk::PipelineLayout& layout) const
	{
		HPR_PROFILE_SCOPE();

		// Upload push const
		ModelMatrixPushConst pushConst{};
		pushConst.modelMatrix = glm::translate(glm::mat4(1.0f), m_Position)
			* glm::toMat4(glm::quat(glm::radians(m_Rotation)))
			* glm::scale(glm::mat4(1.0f), m_Scale);
		cmd.pushConstants<ModelMatrixPushConst>(layout, vk::ShaderStageFlagBits::eVertex, 0, pushConst);

		for (const std::unique_ptr<Mesh>& pMesh : m_Meshes)
		{
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

		for (u32 m = 0; m < scene->mNumMeshes; m++)
		{
			std::vector<VertexPosNormTex> vertices;
			std::vector<u32> indices;

			const aiMesh* mesh = scene->mMeshes[m];
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

			for (u32 f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace face = mesh->mFaces[f];
				for (u32 i = 0; i < face.mNumIndices; i++)
				{
					indices.emplace_back(face.mIndices[i]);
				}
			}

			m_Meshes.emplace_back(std::make_unique<Mesh>(m_pRenderCtx, vertices, indices));
		}

		HPR_CORE_LOG_INFO("Successfully imported model '{}'!", path.filename().string());
	}
}
