#include "HyperPCH.h"
#include "VulkanShader.h"
#include "Hyper/IO/FileUtils.h"

#include <shaderc/shaderc.hpp>

namespace Hyper
{
	std::string ToString(const shaderc_compilation_status& status)
	{
		switch (status)
		{
		case shaderc_compilation_status_success: return "success";
		case shaderc_compilation_status_invalid_stage: return "invalid_stage";
		case shaderc_compilation_status_compilation_error: return "compilation_error";
		case shaderc_compilation_status_internal_error: return "internal_error";
		case shaderc_compilation_status_null_result_object: return "null_result_object";
		case shaderc_compilation_status_invalid_assembly: return "invalid_assembly";
		case shaderc_compilation_status_validation_error: return "validation_error";
		case shaderc_compilation_status_transformation_error: return "transformation_error";
		case shaderc_compilation_status_configuration_error: return "configuration_error";
		}

		throw std::runtime_error("Unsupported shaderc_compilation_status value!");
	}

	shaderc_shader_kind GetShadercShaderKind(const ShaderStageType& type)
	{
		switch (type)
		{
		case ShaderStageType::Vertex: return shaderc_shader_kind::shaderc_vertex_shader;
		case ShaderStageType::Fragment: return shaderc_shader_kind::shaderc_fragment_shader;
		}

		throw std::runtime_error("Unsupported shader type!");
	}

	ShaderModule::ShaderModule(RenderContext* pRenderCtx, ShaderStageType type,
		const std::filesystem::path& filePath)
		: m_pRenderCtx(pRenderCtx), m_Type(type), m_FilePath(filePath)
	{
		std::string shaderCode;
		if (!IO::ReadFileSync(filePath, shaderCode))
		{
			throw std::runtime_error("Failed to read shader file: "s + m_FilePath.string());
		}

		// TODO: compiled shader caching system.

		// Compile GLSL code to SPIR-V
		std::vector<u32> shaderBinary{};
		{
			shaderc::Compiler compiler{};
			shaderc::CompileOptions options;
			options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
			options.SetWarningsAsErrors();
			options.SetGenerateDebugInfo();

			if (!compiler.IsValid())
			{
				throw std::runtime_error("Shader compiler is invalid!");
			}

			const auto result = compiler.CompileGlslToSpv(shaderCode, GetShadercShaderKind(type), filePath.string().c_str(), options);

			const shaderc_compilation_status status = result.GetCompilationStatus();
			std::string shaderId = filePath.string();
			size_t numWarnings = result.GetNumWarnings();
			size_t numErrors = result.GetNumErrors();

			if (result.GetCompilationStatus() == shaderc_compilation_status_success)
			{
				HPR_VKLOG_INFO("Successfully compiled shader {} with {} warning(s) and {} error(s)", shaderId, numWarnings, numErrors);

				shaderBinary = { result.cbegin(), result.cend() };
			}
			else
			{
				HPR_VKLOG_ERROR("Failed to compile shader {} : {}, {} warning(s) and {} error(s):", shaderId, ToString(status), numWarnings, numErrors);
				HPR_VKLOG_ERROR("Errors:\n{}", result.GetErrorMessage());
			}
		}

		// TODO: create `VkShaderModule`s
		{
			vk::ShaderModuleCreateInfo createInfo = {};
			createInfo.setCode(shaderBinary);

			try
			{
				m_Module = pRenderCtx->device.createShaderModule(createInfo);
			}
			catch (vk::SystemError& e)
			{
				throw std::runtime_error("Failed to create shader module: "s + e.what());
			}

			m_ShaderInfo = vk::PipelineShaderStageCreateInfo{};
			m_ShaderInfo.stage = static_cast<vk::ShaderStageFlagBits>(m_Type);
			m_ShaderInfo.module = m_Module;
			m_ShaderInfo.pName = "main";
		}
	}

	ShaderModule::~ShaderModule()
	{
		if (m_Module)
		{
			m_pRenderCtx->device.destroyShaderModule(m_Module);
		}
	}

	ShaderModule::ShaderModule(ShaderModule&& other)
		: m_pRenderCtx(other.m_pRenderCtx)
		, m_Type(other.m_Type)
		, m_FilePath(other.m_FilePath)
		, m_Module(other.m_Module)
		, m_ShaderInfo(other.m_ShaderInfo)
	{
		// Invalidate the other's handle so that we don't try to destroy it when moving.
		other.m_Module = nullptr;
	}

	ShaderModule& ShaderModule::operator=(ShaderModule&& other)
	{
		m_pRenderCtx = other.m_pRenderCtx;
		m_Type = other.m_Type;
		m_FilePath = other.m_FilePath;
		m_Module = other.m_Module;
		m_ShaderInfo = other.m_ShaderInfo;

		// Invalidate the other's handle so that we don't try to destroy it when moving.
		other.m_Module = nullptr;

		return *this;
	}

	VulkanShader::VulkanShader(RenderContext* pRenderCtx)
		: m_pRenderCtx(pRenderCtx)
	{
	}

	void VulkanShader::AddStage(ShaderStageType type, const std::filesystem::path& filePath)
	{
		m_ShaderModules.insert_or_assign(type, ShaderModule(m_pRenderCtx, type, filePath));
	}

	std::vector<vk::PipelineShaderStageCreateInfo> VulkanShader::GetAllShaderStages() const
	{
		std::vector<vk::PipelineShaderStageCreateInfo> infos;

		for (const auto& shaderModule : m_ShaderModules | std::views::values)
		{
			infos.push_back(shaderModule.GetShaderInfo());
		}

		return infos;
	}
}
