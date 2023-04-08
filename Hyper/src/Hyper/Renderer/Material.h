#pragma once
#include <glm/vec4.hpp>

namespace Hyper
{
	struct RenderContext;
	struct TextureHandle;
	struct MaterialPushConst;
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
		void PostLoadInitialize();

		glm::uvec4 GetTextureIndices() const;

	private:
		RenderContext* m_pRenderCtx;

		UUID m_Id;
		std::string m_Name;
		std::unordered_map<MaterialTextureType, TextureHandle> m_Textures;
	};
}
