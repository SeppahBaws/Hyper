#include <iostream>
#include <fstream>
#include <sstream>
#include <shaderc/shaderc.hpp>

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

	return "";
}

int main()
{
	const std::string filePath = "res/test.glsl";
	std::ifstream file(filePath);
	std::stringstream buf;
	buf << file.rdbuf();
	std::string shaderContent = buf.str();

	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
	options.SetWarningsAsErrors();
	options.SetGenerateDebugInfo();

	if (!compiler.IsValid())
	{
		std::cout << "Compiler is not valid!" << std::endl;
	}

	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(shaderContent, shaderc_shader_kind::shaderc_fragment_shader, filePath.c_str(), options);

	std::cout << "Compilation status: " << ToString(result.GetCompilationStatus()) << std::endl;
	std::cout << "Shader compiled with " << result.GetNumWarnings() << " warning(s) and " << result.GetNumErrors() << " error(s)" << std::endl;
	std::cout << "Errors: " << result.GetErrorMessage() << std::endl;

	// std::cout << "---------------------[ Shader code ]---------------------" << std::endl;
	//
	// std::ofstream output("res/test.spirv");
	// for (unsigned byte : result)
	// {
	// 	std::cout << byte;
	// }
}
