#include "HyperPCH.h"
#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Hyper/Debug/Profiler.h"
#include "Vulkan/VulkanDebug.h"
#include "Vulkan/VulkanIndexBuffer.h"

namespace Hyper
{
	Mesh::Mesh(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
		Import();

		// Vertex buffer
		m_pVertexBuffer = std::make_unique<VulkanVertexBuffer>(m_pRenderCtx, "test vertex buffer");
		m_pVertexBuffer->CreateFrom(m_Vertices);

		// Index buffer
		m_pIndexBuffer = std::make_unique<VulkanIndexBuffer>(m_pRenderCtx, "test index buffer");
		m_pIndexBuffer->CreateFrom(m_Indices);
	}

	Mesh::~Mesh()
	{
		m_pVertexBuffer.reset();
	}

	void Mesh::Bind(const vk::CommandBuffer& cmd) const
	{
		m_pVertexBuffer->Bind(cmd);
		m_pIndexBuffer->Bind(cmd);
	}

	void Mesh::Draw(const vk::CommandBuffer& cmd) const
	{
		HPR_PROFILE_SCOPE();

		Bind(cmd);

		cmd.drawIndexed(static_cast<u32>(m_Indices.size()), 1, 0, 0, 0);
	}

	void Mesh::Import()
	{
		m_Vertices = {};
		m_Indices = {};
		Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile("res/models/Suzanne.fbx", aiProcess_Triangulate);
		if (scene == nullptr)
		{
			HPR_CORE_LOG_ERROR("Failed to import model file!");
			return;
		}

		for (u32 m = 0; m < scene->mNumMeshes; m++)
		{
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

				m_Vertices.emplace_back(VertexPosNormTex{ pos, norm, tex });
			}

			for (u32 f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace face = mesh->mFaces[f];
				for (u32 i = 0; i < face.mNumIndices; i++)
				{
					m_Indices.emplace_back(face.mIndices[i]);
				}
			}
		}
	}
}
