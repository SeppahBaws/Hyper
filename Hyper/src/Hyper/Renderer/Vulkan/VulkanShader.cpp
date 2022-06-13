#include "HyperPCH.h"
#include "VulkanShader.h"
#include "Hyper/IO/FileUtils.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_reflect.hpp>

#include "VulkanDescriptors.h"

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

	VulkanShader::VulkanShader(RenderContext* pRenderCtx, std::unordered_map<ShaderStageType, std::filesystem::path> shaders)
		: m_pRenderCtx(pRenderCtx)
	{
		for (const auto& [stage, path] : shaders)
		{
			std::string shaderCode{};
			if (!IO::ReadFileSync(path, shaderCode))
			{
				throw std::runtime_error("Failed to read shader file: "s + path.string());
			}

			shaderCode = PreProcessStage(stage, shaderCode, path.string());
			auto [shaderModule, spirvBytes] = CompileStage(stage, shaderCode, path.string());
			m_ShaderModules.insert_or_assign(stage, ShaderModule{
				path,
				shaderModule
			});

			// Write shader spirv to file (mainly for debugging purposes)
			const std::filesystem::path compiledPath = path.parent_path() / "compiled";
			// Create the compiled directory if it doesn't exist yet
			if (!std::filesystem::exists(compiledPath))
			{
				std::filesystem::create_directory(compiledPath);
			}
			std::filesystem::path spvPath = compiledPath / path.filename();
			spvPath = fmt::format("{}.spv", spvPath.string());
			if (!IO::WriteFileSync(spvPath, spirvBytes))
			{
				HPR_VKLOG_ERROR("Failed to write spirv shader to '{}'", spvPath.string());
			}

			Reflect(stage, spirvBytes, path.string());
		}

		for (const auto& descriptorSet : m_DescriptorSetBindings)
		{
			auto builder = DescriptorSetLayoutBuilder(m_pRenderCtx->device);
			for (const auto& [descType, binding, stageFlags] : descriptorSet)
			{
				builder = builder.AddBinding(descType, binding, 1, stageFlags);
			}

			m_DescriptorLayouts.push_back(builder.Build());
		}

		for (const auto& [offset, size, stageFlags] : m_PushConstants)
		{
			auto& range = m_PushConstRanges.emplace_back();
			range.offset = offset;
			range.size = size;
			range.stageFlags = stageFlags;
		}

		HPR_VKLOG_INFO("Successfully created shaders");
	}

	VulkanShader::~VulkanShader()
	{
		for (auto& layout : m_DescriptorLayouts)
		{
			m_pRenderCtx->device.destroyDescriptorSetLayout(layout);
			layout = nullptr;
		}

		for (auto& [_, module] : m_ShaderModules | std::views::values)
		{
			m_pRenderCtx->device.destroyShaderModule(module);
			module = nullptr;
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

	std::string VulkanShader::PreProcessStage(ShaderStageType stage, const std::string& shaderCode, const std::string& shaderName)
	{
		std::string outputShaderCode{};

		shaderc::Compiler compiler{};
		shaderc::CompileOptions options{};

		const auto result = compiler.PreprocessGlsl(shaderCode, GetShadercShaderKind(stage), shaderName.c_str(), options);

		const shaderc_compilation_status status = result.GetCompilationStatus();
		std::string shaderId = shaderName;
		size_t numWarnings = result.GetNumWarnings();
		size_t numErrors = result.GetNumErrors();

		if (result.GetCompilationStatus() == shaderc_compilation_status_success)
		{
			outputShaderCode = { result.begin(), result.end() };
		}
		else
		{
			HPR_VKLOG_ERROR("Failed to preprocess shader {} : {}, {} warning(s) and {} error(s):", shaderId, ToString(status), numWarnings, numErrors);
			HPR_VKLOG_ERROR("Errors:\n{}", result.GetErrorMessage());
		}

		return outputShaderCode;
	}

	std::pair<vk::ShaderModule, std::vector<u32>> VulkanShader::CompileStage(ShaderStageType stage, const std::string& shaderCode, const std::string& shaderName)
	{
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
			HPR_VKLOG_INFO("Successfully compiled shader {} with {} warning(s) and {} error(s)", shaderId, numWarnings, numErrors);

			// Storing this for now, in case we want to do some sort of shader reflection later.
			shaderBinary = { result.cbegin(), result.cend() };
		}
		else
		{
			HPR_VKLOG_ERROR("Failed to compile shader {} : {}, {} warning(s) and {} error(s):", shaderId, ToString(status), numWarnings, numErrors);
			HPR_VKLOG_ERROR("Errors:\n{}", result.GetErrorMessage());
			throw std::runtime_error("Failed to compile shader");
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

		return { module, shaderBinary };
	}

	void VulkanShader::Reflect(ShaderStageType stage, const std::vector<u32>& spirvBytes, const std::string& moduleName)
	{
		spirv_cross::Compiler compiler(spirvBytes);
		auto resources = compiler.get_shader_resources();

		HPR_VKLOG_INFO("===[ {} reflection ]===", moduleName);
		HPR_VKLOG_TRACE("  uniform_buffers: {}", resources.uniform_buffers.size());
		HPR_VKLOG_TRACE("  storage_buffers: {}", resources.storage_buffers.size());
		HPR_VKLOG_TRACE("  stage_inputs: {}", resources.stage_inputs.size());
		HPR_VKLOG_TRACE("  stage_outputs: {}", resources.stage_outputs.size());
		HPR_VKLOG_TRACE("  subpass_inputs: {}", resources.subpass_inputs.size());
		HPR_VKLOG_TRACE("  storage_images: {}", resources.storage_images.size());
		HPR_VKLOG_TRACE("  sampled_images: {}", resources.sampled_images.size());
		HPR_VKLOG_TRACE("  atomic_counters: {}", resources.atomic_counters.size());
		HPR_VKLOG_TRACE("  acceleration_structures: {}", resources.acceleration_structures.size());
		HPR_VKLOG_TRACE("  push_constant_buffers: {}", resources.push_constant_buffers.size());
		HPR_VKLOG_TRACE("  separate_images: {}", resources.separate_images.size());
		HPR_VKLOG_TRACE("  separate_samplers: {}", resources.separate_samplers.size());
		HPR_VKLOG_TRACE("  builtin_inputs: {}", resources.builtin_inputs.size());
		HPR_VKLOG_TRACE("  builtin_outputs: {}", resources.builtin_outputs.size());

		// Uniform buffers
		for (const auto& resource : resources.uniform_buffers)
		{
			// uniformBuffer.
			const auto& name = resource.name;
			auto& bufferType = compiler.get_type(resource.base_type_id);
			u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			u32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

			HPR_VKLOG_INFO("Uniform buffer: '{}'", name);
			HPR_VKLOG_INFO("  binding: {}", binding);
			HPR_VKLOG_INFO("  set: {}", descriptorSet);

			if (descriptorSet >= m_DescriptorSetBindings.size())
				m_DescriptorSetBindings.resize(descriptorSet + 1);

			auto& set = m_DescriptorSetBindings[descriptorSet].emplace_back();
			set.binding = binding;
			set.descType = vk::DescriptorType::eUniformBuffer;
			set.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage);
		}

		// Samplers
		for (const auto& resource : resources.sampled_images)
		{
			const auto& name = resource.name;
			u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			u32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

			if (descriptorSet >= m_DescriptorSetBindings.size())
				m_DescriptorSetBindings.resize(descriptorSet + 1);

			auto& set = m_DescriptorSetBindings[descriptorSet].emplace_back();
			set.binding = binding;
			set.descType = vk::DescriptorType::eCombinedImageSampler;
			set.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage);

			HPR_VKLOG_INFO("Sampler: '{}'", name);
			HPR_VKLOG_INFO("  binding: {}", binding);
			HPR_VKLOG_INFO("  set: {}", descriptorSet);
		}

		// Storage images
		for (const auto& resource : resources.storage_images)
		{
			const auto& name = resource.name;
			u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			u32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

			if (descriptorSet >= m_DescriptorSetBindings.size())
				m_DescriptorSetBindings.resize(descriptorSet + 1);

			auto& set = m_DescriptorSetBindings[descriptorSet].emplace_back();
			set.binding = binding;
			set.descType = vk::DescriptorType::eStorageImage;
			set.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage);

			HPR_VKLOG_INFO("Storage image: '{}'", name);
			HPR_VKLOG_INFO("  binding: {}", binding);
			HPR_VKLOG_INFO("  set: {}", descriptorSet);
		}

		// Storage buffers
		for (const auto& resource : resources.storage_buffers)
		{
			const auto& name = resource.name;
			u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			u32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

			if (descriptorSet >= m_DescriptorSetBindings.size())
				m_DescriptorSetBindings.resize(descriptorSet + 1);

			auto& set = m_DescriptorSetBindings[descriptorSet].emplace_back();
			set.binding = binding;
			set.descType = vk::DescriptorType::eStorageBuffer;
			set.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage);

			HPR_VKLOG_INFO("Storage buffer: '{}'", name);
			HPR_VKLOG_INFO("  binding: {}", binding);
			HPR_VKLOG_INFO("  set: {}", descriptorSet);
		}

		// Acceleration structures
		for (const auto& resource : resources.acceleration_structures)
		{
			const auto& name = resource.name;
			u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			u32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

			if (descriptorSet >= m_DescriptorSetBindings.size())
				m_DescriptorSetBindings.resize(descriptorSet + 1);

			auto& set = m_DescriptorSetBindings[descriptorSet].emplace_back();
			set.binding = binding;
			set.descType = vk::DescriptorType::eAccelerationStructureKHR;
			set.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage);

			HPR_VKLOG_INFO("Acceleration structure: '{}'", name);
			HPR_VKLOG_INFO("  binding: {}", binding);
			HPR_VKLOG_INFO("  set: {}", descriptorSet);
		}

		// Push constants
		for (const auto& resource : resources.push_constant_buffers)
		{
			const std::string& name = resource.name;
			auto& bufferType = compiler.get_type(resource.base_type_id);
			const u32 bufferSize = static_cast<u32>(compiler.get_declared_struct_size(bufferType));
			u32 bufferOffset = 0;
			if (!m_PushConstants.empty())
				bufferOffset = m_PushConstants.back().offset + m_PushConstants.back().size;

			auto& pushConst = m_PushConstants.emplace_back();
			pushConst.size = bufferSize - bufferOffset;
			pushConst.offset = bufferOffset;
			pushConst.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage);

			HPR_VKLOG_INFO("Push constant: '{}'", name);
			HPR_VKLOG_INFO("  size: {}", pushConst.size);
			HPR_VKLOG_INFO("  offset: {}", pushConst.offset);
		}
	}
}
