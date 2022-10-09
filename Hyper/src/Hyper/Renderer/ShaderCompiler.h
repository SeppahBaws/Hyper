#pragma once

namespace Hyper
{
	enum class ShaderStageType : VkShaderStageFlags;
}

namespace Hyper
{
	class ShaderCompiler
	{
	public:
		static std::vector<u32> Compile(const std::filesystem::path& filePath, ShaderStageType stage);

	private:
		static std::vector<u32> CompileGLSL(const std::string& shaderCode, ShaderStageType stage, const std::string& shaderName);
		static std::string PreProcessGLSL(const std::string& shaderCode, ShaderStageType stage, const std::string& shaderName);

		static std::vector<u32> CompileHLSL(const std::filesystem::path& filePath, ShaderStageType stage);
	};
}
