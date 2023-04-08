#include "HyperPCH.h"
#include "Material.h"

#include "RenderContext.h"
#include "Renderer.h"
#include "Texture.h"
#include "Hyper/Asset/TextureManager.h"
#include "Vulkan/VulkanDescriptors.h"

namespace Hyper
{
	Material::Material(RenderContext* pRenderCtx, const std::string& name)
		: m_pRenderCtx(pRenderCtx)
		, m_Name(name)
	{
	}

	Material::~Material()
	{
		for (auto& texture : m_Textures | std::views::values)
		{
			m_pRenderCtx->pTextureManager->UnloadTexture(texture);
		}
	}

	Material::Material(Material&& other) noexcept: m_pRenderCtx(other.m_pRenderCtx),
		m_Id(other.m_Id),
		m_Name(std::move(other.m_Name)),
		m_Textures(std::move(other.m_Textures))
	{
	}

	Material& Material::operator=(Material&& other) noexcept
	{
		if (this == &other)
			return *this;
		m_pRenderCtx = other.m_pRenderCtx;
		m_Id = other.m_Id;
		m_Name = std::move(other.m_Name);
		m_Textures = std::move(other.m_Textures);

		return *this;
	}

	void Material::LoadTexture(MaterialTextureType type, const std::filesystem::path& fileName, bool srgb)
	{
		if (!m_Textures.contains(type))
		{
			m_Textures[type] = m_pRenderCtx->pTextureManager->LoadTexture(fileName, srgb);
		}
		else
		{
			HPR_CORE_LOG_ERROR("Cannot load {} texture '{}' - it already exists on material '{}'", type == MaterialTextureType::Albedo ? "Albedo" : "Normal",
				fileName.string(), m_Name);
		}
	}

	void Material::PostLoadInitialize()
	{
		const TextureHandle albedoTexture = m_Textures.at(MaterialTextureType::Albedo);
		const auto albedoImageInfo = m_pRenderCtx->pTextureManager->GetTexture(albedoTexture)->GetDescriptorImageInfo();

		const TextureHandle normalTexture = m_Textures.at(MaterialTextureType::Normal);
		const auto normalImageInfo = m_pRenderCtx->pTextureManager->GetTexture(normalTexture)->GetDescriptorImageInfo();

		m_pRenderCtx->bindlessDescriptorWriter->WriteImageBindless(albedoImageInfo, RenderContext::bindlessTextureBinding, albedoTexture.idx,
			vk::DescriptorType::eCombinedImageSampler);
		m_pRenderCtx->bindlessDescriptorWriter->WriteImageBindless(normalImageInfo, RenderContext::bindlessTextureBinding, normalTexture.idx,
			vk::DescriptorType::eCombinedImageSampler);
	}

	glm::uvec4 Material::GetTextureIndices() const
	{
		glm::uvec4 vec{};
		vec.x = m_Textures.at(MaterialTextureType::Albedo).idx;
		vec.y = m_Textures.at(MaterialTextureType::Normal).idx;

		return vec;
	}
}
