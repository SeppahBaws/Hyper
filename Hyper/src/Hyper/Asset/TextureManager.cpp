#include "HyperPCH.h"
#include "TextureManager.h"

#include "Hyper/Renderer/RenderContext.h"
#include "Hyper/Renderer/Texture.h"

namespace Hyper
{
	static u32 Version = 0;

	TextureManager::TextureManager(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
		m_pTextures.reserve(20);
	}

	TextureManager::~TextureManager()
	{
		for (u32 i = 0; i < m_pTextures.size(); ++i)
		{
			delete m_pTextures[i].ptr;
			m_pTextures[i].version = 0;
		}

		m_pTextures.clear();
	}

	TextureHandle TextureManager::LoadTexture(const std::filesystem::path& fileName, bool srgb)
	{
		u32 version = ++Version;
		m_pTextures.emplace_back(new Texture(m_pRenderCtx, fileName, srgb), version);
		
		return TextureHandle {
			.idx = static_cast<u32>(m_pTextures.size()) - 1,
			.version = version
		};
	}

	void TextureManager::UnloadTexture(TextureHandle handle)
	{
		delete m_pTextures[handle.idx].ptr;
		m_pTextures[handle.idx].version = 0;
	}

	Texture* TextureManager::GetTexture(TextureHandle handle) const
	{
		// IsValid check:
		if (handle.idx >= m_pTextures.size() || handle.version != m_pTextures[handle.idx].version)
		{
			HPR_CORE_LOG_WARN("Texture at index {} is invalid. Expected version {} but got {}", handle.idx, handle.version, m_pTextures[handle.idx].version);
			return nullptr;
		}

		return m_pTextures[handle.idx].ptr;
	}
}
