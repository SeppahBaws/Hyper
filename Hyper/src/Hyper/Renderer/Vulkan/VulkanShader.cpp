#include "HyperPCH.h"
#include "VulkanShader.h"
#include "Hyper/IO/FileUtils.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_reflect.hpp>

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

	VulkanShader::VulkanShader(RenderContext* pRenderCtx, std::unordered_map<ShaderStageType, std::filesystem::path> shaders)
		: m_pRenderCtx(pRenderCtx)
	{
		for (const auto& [stage, path] : shaders)
		{
			const vk::ShaderModule vulkanModule = CompileStage(stage, path);
			m_ShaderModules.insert_or_assign(stage, ShaderModule{
				path,
				vulkanModule
			});
		}

		HPR_VKLOG_INFO("Successfully created shaders");
	}

	VulkanShader::~VulkanShader()
	{
		for (const auto& [_, module] : m_ShaderModules | std::views::values)
		{
			m_pRenderCtx->device.destroyShaderModule(module);
		}
	}

	std::vector<vk::PipelineShaderStageCreateInfo> VulkanShader::GetAllShaderStages() const
	{
		std::vector<vk::PipelineShaderStageCreateInfo> infos;

		for (const auto& [stage, module] : m_ShaderModules)
		{
			auto& info = infos.emplace_back();
			info.module = module.module;
			info.stage = static_cast<vk::ShaderStageFlagBits>(stage);
			info.pName = "main";
		}

		return infos;
	}

	vk::ShaderModule VulkanShader::CompileStage(ShaderStageType stage, const std::filesystem::path& filePath)
	{
		std::string shaderCode;
		if (!IO::ReadFileSync(filePath, shaderCode))
		{
			throw std::runtime_error("Failed to read shader file: "s + filePath.string());
		}

		// TODO: compiled shader caching system.

		// Compile GLSL code to SPIR-V
		std::vector<u32> shaderBinary{};

		shaderc::Compiler compiler{};
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		options.SetWarningsAsErrors();
		options.SetGenerateDebugInfo();

		if (!compiler.IsValid())
		{
			throw std::runtime_error("Shader compiler is invalid!");
		}

		const auto result = compiler.CompileGlslToSpv(shaderCode, GetShadercShaderKind(stage), filePath.string().c_str(), options);

		const shaderc_compilation_status status = result.GetCompilationStatus();
		std::string shaderId = filePath.string();
		size_t numWarnings = result.GetNumWarnings();
		size_t numErrors = result.GetNumErrors();

		// Check compilation status.
		if (result.GetCompilationStatus() == shaderc_compilation_status_success)
		{
			HPR_VKLOG_INFO("Successfully compiled shader {} with {} warning(s) and {} error(s)", shaderId, numWarnings, numErrors);

			// Storing this for now, in case we want to do some sort of shader reflection later.
			shaderBinary = { result.cbegin(), result.cend() };
		}
		else
		{
			HPR_VKLOG_ERROR("Failed to compile shader {} : {}, {} warning(s) and {} error(s):", shaderId, ToString(status), numWarnings, numErrors);
			HPR_VKLOG_ERROR("Errors:\n{}", result.GetErrorMessage());
		}

		// Create shader module
		vk::ShaderModule module;
		{
			vk::ShaderModuleCreateInfo createInfo = {};
			createInfo.setCode(shaderBinary);

			try
			{
				module = m_pRenderCtx->device.createShaderModule(createInfo);
			}
			catch (vk::SystemError& e)
			{
				throw std::runtime_error("Failed to create shader module: "s + e.what());
			}
		}

		return module;
	}
}
