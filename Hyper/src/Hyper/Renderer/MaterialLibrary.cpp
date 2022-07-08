#include "HyperPCH.h"
#include "MaterialLibrary.h"

namespace Hyper
{
	MaterialLibrary::MaterialLibrary(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
	}

	MaterialLibrary::~MaterialLibrary()
	{
		m_Materials.clear();
	}

	MaterialLibrary::MaterialLibrary(MaterialLibrary&& other)
		: m_pRenderCtx(other.m_pRenderCtx)
		, m_Materials(std::move(other.m_Materials))
	{
		HPR_VKLOG_WARN("MaterialLibrary moved!");
	}

	MaterialLibrary& MaterialLibrary::operator=(MaterialLibrary&& other)
	{
		m_pRenderCtx = other.m_pRenderCtx;
		m_Materials = std::move(other.m_Materials);

		HPR_VKLOG_WARN("MaterialLibrary move-assigned!");

		return *this;
	}

	Material& MaterialLibrary::CreateMaterial(const std::string& name)
	{
		Material material = Material{ m_pRenderCtx, name };
		UUID id = material.GetId();
		m_Materials.emplace(id, std::move(material));

		return m_Materials.at(id);
	}

	const Material& MaterialLibrary::GetMaterial(UUID id) const
	{
		if (m_Materials.contains(id))
		{
			return m_Materials.at(id);
		}

		HPR_CORE_LOG_ERROR("Failed to get material with id '{}': no such material was found in the material library", id);
		throw std::runtime_error(fmt::format("Failed to get material with id '{}': no such material was found in the material library", id));
	}
}
