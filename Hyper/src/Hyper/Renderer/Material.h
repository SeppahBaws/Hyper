#pragma once
#include "Vulkan/VulkanDescriptors.h"

namespace Hyper
{
	struct RenderContext;

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

		UUID GetId() const { return m_Id; }

		void LoadTexture(MaterialTextureType type, const std::filesystem::path& fileName, bool srgb = true);
		void PostLoadInititalize();

		void Bind(const vk::CommandBuffer& cmd, const vk::PipelineLayout& layout) const;

	private:
		RenderContext* m_pRenderCtx;

		UUID m_Id;
		std::string m_Name;
		std::unordered_map<MaterialTextureType, std::unique_ptr<Texture>> m_Textures;

		std::unique_ptr<vk::DescriptorSetLayout> m_pLayout;
		std::unique_ptr<DescriptorPool> m_DescriptorPool;
		vk::DescriptorSet m_DescriptorSet;
	};
}
