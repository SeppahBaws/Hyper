#pragma once
#include "Material.h"

namespace Hyper
{
	class MaterialLibrary
	{
	public:
		MaterialLibrary(RenderContext* pRenderCtx);
		~MaterialLibrary();
		MaterialLibrary(MaterialLibrary&& other);
		MaterialLibrary& operator=(MaterialLibrary&& other);
		MaterialLibrary(const MaterialLibrary& other) = delete;
		MaterialLibrary& operator=(const MaterialLibrary& other) = delete;

		Material& CreateMaterial(const std::string& name);
		[[nodiscard]] const Material& GetMaterial(UUID id) const;

	private:
		RenderContext* m_pRenderCtx;

		std::unordered_map<UUID, Material> m_Materials{};
	};
}
