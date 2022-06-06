#pragma once
#include "RenderContext.h"
#include "Vulkan/VulkanDescriptors.h"

namespace Hyper
{
	class Texture;

	enum class MaterialTextureType
	{
		Albedo,
		Normal
	};

	class Material
	{
	public:
		Material(RenderContext* pRenderCtx, const std::string& name);
		~Material();
		Material(Material&& other) noexcept;
		Material& operator=(Material&& other) noexcept;
		// Disable copying
		Material(const Material& other) = delete;
		Material& operator=(const Material& other) = delete;

		void LoadTexture(MaterialTextureType type, const std::filesystem::path& fileName);
		void PostLoadInititalize();

		void Bind(const vk::CommandBuffer& cmd, const vk::PipelineLayout& layout) const;

	private:
		RenderContext* m_pRenderCtx;
		
		std::string m_Name;
		std::unordered_map<MaterialTextureType, std::unique_ptr<Texture>> m_Textures;

		std::unique_ptr<vk::DescriptorSetLayout> m_pLayout;
		std::unique_ptr<DescriptorPool> m_DescriptorPool;
		vk::DescriptorSet m_DescriptorSet;
	};
}
