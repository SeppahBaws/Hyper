#pragma once

namespace Hyper
{
	struct RenderContext;
	class Texture;
	
	struct TextureHandle
	{
		u32 idx;
		u32 version;
	};

	class TextureManager
	{
	public:
		explicit TextureManager(RenderContext* pRenderCtx);
		~TextureManager();

		TextureHandle LoadTexture(const std::filesystem::path& fileName, bool srgb);
		void UnloadTexture(TextureHandle handle);
		Texture* GetTexture(TextureHandle handle) const;

	private:
		RenderContext* m_pRenderCtx;

		struct TextureStorage
		{
			Texture* ptr;
			u32 version;
		};
		std::vector<TextureStorage> m_pTextures;
	};
}
