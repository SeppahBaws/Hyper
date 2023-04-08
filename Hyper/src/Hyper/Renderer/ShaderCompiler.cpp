#include "HyperPCH.h"
#include "ShaderCompiler.h"

#include <shaderc/shaderc.hpp>
#ifdef HYPER_WINDOWS
#include <atlbase.h> // windows-only function.
#include <Unknwnbase.h>
#endif
#include <dxc/dxcapi.h>

#include "Hyper/IO/FileUtils.h"
#include "Vulkan/VulkanShader.h"

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
		case ShaderStageType::RayGen: return shaderc_shader_kind::shaderc_raygen_shader;
		case ShaderStageType::Miss: return shaderc_shader_kind::shaderc_miss_shader;
		case ShaderStageType::ClosestHit: return shaderc_shader_kind::shaderc_closesthit_shader;
		}

		throw std::runtime_error("Unsupported shader type!");
	}

	std::vector<u32> ShaderCompiler::Compile(const std::filesystem::path& filePath, ShaderStageType stage)
	{
		// Shortcut for directly loading spv files
		if (filePath.extension().string().substr(1) == "spv")
		{
			std::vector<u32> spirvBytes;
			if (!IO::ReadFileSync(filePath, spirvBytes))
			{
				HPR_CORE_LOG_ERROR("Failed to load spir-v file: {}", filePath.string());
				throw std::runtime_error("Failed to load spir-v file: "s + filePath.string());
			}
			return spirvBytes;
		}

		const std::filesystem::path compiledPath = filePath.parent_path() / "compiled";
		// Create the compiled directory if it doesn't exist yet
		if (!std::filesystem::exists(compiledPath))
		{
			std::filesystem::create_directory(compiledPath);
		}

		// Combine last write time with filename to construct the cached binary filename
		auto lastWrite = std::chrono::duration_cast<std::chrono::seconds>(std::filesystem::last_write_time(filePath).time_since_epoch()).count();
		std::filesystem::path cachedPath = compiledPath / filePath.filename();
		cachedPath = fmt::format("{}_{}.spv", cachedPath.string(), lastWrite);

		std::vector<u32> spirvBytes;

		if (std::filesystem::exists(cachedPath))
		{
			HPR_CORE_LOG_INFO("Shader '{}' was already compiled, loading shader binary from '{}'...", filePath.filename().string(), cachedPath.string());

			// Matching shader name and write date were found, load this.
			if (!IO::ReadFileSync(cachedPath, spirvBytes))
			{
				throw std::runtime_error("Failed to read cached shader!");
			}
		}
		else
		{
			HPR_CORE_LOG_INFO("Shader '{}' was either never compiled or has been changed since last compilation. Compiling and caching to '{}'...", filePath.filename().string(), cachedPath.string());

			// No matching shader name and write date were found, but maybe a different date exists.
			for (const auto& file : std::filesystem::directory_iterator(cachedPath.parent_path()))
			{
				// If an outdated cached shader exists, delete it so that we don't gobble up disk space.
				if (file.path().filename().string().starts_with(filePath.filename().string()))
				{
					std::filesystem::remove(file.path());
				}
			}

			// Compile shader
			const std::string extension = filePath.extension().string().substr(1);
			if (extension == "glsl")
			{
				// Load shader source code
				std::string shaderCode{};
				if (!IO::ReadFileSync(filePath, shaderCode))
				{
					throw std::runtime_error("Failed to read shader file: "s + filePath.string());
				}

				spirvBytes = CompileGLSL(shaderCode, stage, filePath.filename().string());
			}
			else if (extension == "hlsl")
			{
				spirvBytes = CompileHLSL(filePath, stage);
			}
			else
			{
				throw std::runtime_error("Unsupported shader file extension: "s + extension);
			}

			// Cache compiled output
			if (!IO::WriteFileSync(cachedPath, spirvBytes))
			{
				HPR_CORE_LOG_ERROR("Failed to write SPIR-V shader to '{}'", cachedPath.string());
			}
		}

		return spirvBytes;
	}

	std::vector<u32> ShaderCompiler::CompileGLSL(const std::string& shaderCode, ShaderStageType stage, const std::string& shaderName)
	{
		std::string processedCode = PreProcessGLSL(shaderCode, stage, shaderName);

		// Compile GLSL code to SPIR-V
		std::vector<u32> shaderBinary{};

		shaderc::Compiler compiler{};
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		options.SetTargetSpirv(shaderc_spirv_version_1_6);
		options.SetForcedVersionProfile(460, shaderc_profile_core); // Somehow it doesn't want to infer these from the actual shader itself (??)
		options.SetWarningsAsErrors();
		options.SetGenerateDebugInfo();

		if (!compiler.IsValid())
		{
			throw std::runtime_error("Shader compiler is invalid!");
		}

		const auto result = compiler.CompileGlslToSpv(shaderCode, GetShadercShaderKind(stage), shaderName.c_str(), options);

		const shaderc_compilation_status status = result.GetCompilationStatus();
		std::string shaderId = shaderName;
		size_t numWarnings = result.GetNumWarnings();
		size_t numErrors = result.GetNumErrors();

		// Check compilation status.
		if (result.GetCompilationStatus() == shaderc_compilation_status_success)
		{
			HPR_CORE_LOG_INFO("Successfully compiled shader {} with {} warning(s) and {} error(s)", shaderId, numWarnings, numErrors);

			// Storing this for now, in case we want to do some sort of shader reflection later.
			shaderBinary = { result.cbegin(), result.cend() };
		}
		else
		{
			HPR_CORE_LOG_ERROR("Failed to compile shader {} : {}, {} warning(s) and {} error(s):", shaderId, ToString(status), numWarnings, numErrors);
			HPR_CORE_LOG_ERROR("Errors:\n{}", result.GetErrorMessage());
			throw std::runtime_error("Failed to compile shader");
		}

		return shaderBinary;
	}

	std::string ShaderCompiler::PreProcessGLSL(const std::string& shaderCode, ShaderStageType stage, const std::string& shaderName)
	{
		std::string processedCode{};

		shaderc::Compiler compiler{};
		shaderc::CompileOptions options{};

		const auto result = compiler.PreprocessGlsl(shaderCode, GetShadercShaderKind(stage), shaderName.c_str(), options);

		const shaderc_compilation_status status = result.GetCompilationStatus();
		std::string shaderId = shaderName;
		size_t numWarnings = result.GetNumWarnings();
		size_t numErrors = result.GetNumErrors();

		if (result.GetCompilationStatus() == shaderc_compilation_status_success)
		{
			processedCode = { result.begin(), result.end() };
		}
		else
		{
			HPR_CORE_LOG_ERROR("Failed to preprocess shader {} : {}, {} warning(s) and {} error(s):", shaderId, ToString(status), numWarnings, numErrors);
			HPR_CORE_LOG_ERROR("Errors:\n{}", result.GetErrorMessage());
		}

		return processedCode;
	}

	std::vector<u32> ShaderCompiler::CompileHLSL(const std::filesystem::path& filePath, ShaderStageType stage)
	{
		std::vector<u32> shaderBinary{};

#ifdef HYPER_WINDOWS
		HRESULT hres;

		// Initialize DXC library
		CComPtr<IDxcLibrary> library;
		hres = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
		if (FAILED(hres))
		{
			throw std::runtime_error("Could not init DXC Library");
		}

		// Initialize DXC compiler
		CComPtr<IDxcCompiler3> compiler;
		hres = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		if (FAILED(hres))
		{
			throw std::runtime_error("Could not init DXC Compiler");
		}

		// Initialize DXC utility
		CComPtr<IDxcUtils> utils;
		hres = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		if (FAILED(hres))
		{
			throw std::runtime_error("Could not init DXC Utiliy");
		}

		// Load the HLSL text shader from disk
		uint32_t codePage = DXC_CP_ACP;
		CComPtr<IDxcBlobEncoding> sourceBlob;
		hres = utils->LoadFile(filePath.c_str(), &codePage, &sourceBlob);
		if (FAILED(hres))
		{
			throw std::runtime_error("Could not load shader file");
		}

		// Select target profile based on shader file extension
		LPCWSTR targetProfile{};
		switch (stage)
		{
		case ShaderStageType::Vertex:
			targetProfile = L"vs_6_1";
			break;
		case ShaderStageType::Fragment:
			targetProfile = L"ps_6_1";
			break;
		case ShaderStageType::RayGen:
		case ShaderStageType::ClosestHit:
		case ShaderStageType::Miss:
			targetProfile = L"lib_6_3"; // requires shader model 6.3 or later...
			break;
		default:
			HPR_CORE_LOG_ERROR("Unsupported shader stage");
			throw std::runtime_error("Unsupported shader stage");
		}

		// Configure the compiler arguments for compiling the HLSL shader to SPIR-V
		std::vector<LPCWSTR> arguments = {
			// (Optional) name of the shader file to be displayed e.g. in an error message
			filePath.filename().c_str(),
			// Shader main entry point
			L"-E", L"main",
			// Shader target profile
			L"-T", targetProfile,
			// Compile to SPIRV
			L"-spirv", L"-fspv-target-env=vulkan1.3",
			L"-fspv-extension=SPV_KHR_ray_query",
			L"-fspv-extension=SPV_KHR_ray_tracing",
			// Enable Descriptor indexing extension for bindless rendering
			L"-fspv-extension=SPV_EXT_descriptor_indexing",
			// Don't optimize (atm optimizing would break reflection)
			L"-O0",
			// Set the HLSL version to HLSL 2021
			L"-HV", L"2021"
		};

		// Compile shader
		DxcBuffer buffer{};
		buffer.Encoding = DXC_CP_ACP;
		buffer.Ptr = sourceBlob->GetBufferPointer();
		buffer.Size = sourceBlob->GetBufferSize();

		CComPtr<IDxcResult> result{ nullptr };
		hres = compiler->Compile(
			&buffer,
			arguments.data(),
			static_cast<u32>(arguments.size()),
			nullptr,
			IID_PPV_ARGS(&result));

		if (SUCCEEDED(hres))
		{
			result->GetStatus(&hres);
		}

		// Output error if compilation failed
		if (FAILED(hres) && (result))
		{
			CComPtr<IDxcBlobEncoding> errorBlob;
			hres = result->GetErrorBuffer(&errorBlob);
			if (SUCCEEDED(hres) && errorBlob)
			{
				HPR_CORE_LOG_ERROR("Shader compilation failed: {}", (const char*)errorBlob->GetBufferPointer());
				throw std::runtime_error("Compilation failed");
			}
		}

		// Get compilation result
		CComPtr<IDxcBlob> code;
		result->GetResult(&code);

		const u32* ptr = static_cast<u32*>(code->GetBufferPointer());
		const size_t size = code->GetBufferSize() / sizeof(u32);
		shaderBinary.assign(ptr, ptr + size);

#endif

		return shaderBinary;
	}
}
