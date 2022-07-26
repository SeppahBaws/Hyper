#include "HyperPCH.h"
#include "Material.h"

#include "RenderContext.h"
#include "Renderer.h"
#include "Texture.h"
#include "Vulkan/VulkanDebug.h"
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
			texture.reset();
		}

		m_DescriptorPool.reset();
		if (m_pLayout != nullptr)
		{
			m_pRenderCtx->device.destroyDescriptorSetLayout(*m_pLayout);
			m_pLayout = nullptr;
		}
	}

	Material::Material(Material&& other) noexcept: m_pRenderCtx(other.m_pRenderCtx),
		m_Id(other.m_Id),
		m_Name(std::move(other.m_Name)),
		m_Textures(std::move(other.m_Textures)),
		m_pLayout(std::move(other.m_pLayout)),
		m_DescriptorPool(std::move(other.m_DescriptorPool)),
		m_DescriptorSet(std::move(other.m_DescriptorSet))
	{
		other.m_pLayout = nullptr;
	}

	Material& Material::operator=(Material&& other) noexcept
	{
		if (this == &other)
			return *this;
		m_pRenderCtx = other.m_pRenderCtx;
		m_Id = other.m_Id;
		m_Name = std::move(other.m_Name);
		m_Textures = std::move(other.m_Textures);
		m_pLayout = std::move(other.m_pLayout);
		m_DescriptorSet = std::move(other.m_DescriptorSet);
		m_DescriptorPool = std::move(other.m_DescriptorPool);

		other.m_pLayout = nullptr;
		return *this;
	}

	void Material::LoadTexture(MaterialTextureType type, const std::filesystem::path& fileName, bool srgb)
	{
		if (!m_Textures.contains(type))
		{
			m_Textures[type] = std::make_unique<Texture>(m_pRenderCtx, fileName, srgb);
		}
		else
		{
			HPR_CORE_LOG_ERROR("Cannot load {} texture '{}' - it already exists on material '{}'", type == MaterialTextureType::Albedo ? "Albedo" : "Normal",
				fileName.string(), m_Name);
		}
	}

	void Material::PostLoadInititalize()
	{
		// 1. Create descriptors etc
		m_DescriptorPool = std::make_unique<DescriptorPool>(
			DescriptorPool::Builder(m_pRenderCtx->device)
			.AddSize(vk::DescriptorType::eUniformBuffer, 10)
			.AddSize(vk::DescriptorType::eCombinedImageSampler, 10)
			.SetMaxSets(10)
			.SetFlags({})
			.Build());
		VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::eDescriptorPool, m_DescriptorPool->GetPool(), fmt::format("'{}' Descriptor Pool", m_Name));

		m_pLayout = std::make_unique<vk::DescriptorSetLayout>(
			DescriptorSetLayoutBuilder(m_pRenderCtx->device)
			.AddBinding(vk::DescriptorType::eCombinedImageSampler, 0, 1, vk::ShaderStageFlagBits::eFragment)
			.AddBinding(vk::DescriptorType::eCombinedImageSampler, 1, 1, vk::ShaderStageFlagBits::eFragment)
			.Build());
		VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::eDescriptorSetLayout, *m_pLayout, fmt::format("'{}' Descriptor Set Layout", m_Name));

		m_DescriptorSet = m_DescriptorPool->Allocate({ *m_pLayout })[0];
		VkDebug::SetObjectName(m_pRenderCtx->device, vk::ObjectType::eDescriptorSet, m_DescriptorSet, fmt::format("'{}' Descriptor Set", m_Name));

		// 2. Write descriptors
		DescriptorWriter writer{ m_pRenderCtx->device, m_DescriptorSet };
		const auto& albedoTexture = m_Textures.at(MaterialTextureType::Albedo);
		const auto albedoImageInfo = albedoTexture->GetDescriptorImageInfo();

		const auto& normalTexture = m_Textures.at(MaterialTextureType::Normal);
		const auto normalImageInfo = normalTexture->GetDescriptorImageInfo();
		writer.WriteImage(albedoImageInfo, 0, vk::DescriptorType::eCombinedImageSampler);
		writer.WriteImage(normalImageInfo, 1, vk::DescriptorType::eCombinedImageSampler);
		writer.Write();
	}

	void Material::Bind(const vk::CommandBuffer& cmd, const vk::PipelineLayout& layout) const
	{
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 1, m_DescriptorSet, {});
	}
}
