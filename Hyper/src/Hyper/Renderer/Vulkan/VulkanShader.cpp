#include "HyperPCH.h"
#include "VulkanShader.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_reflect.hpp>

#include "VulkanDescriptors.h"
#include "VulkanUtility.h"
#include "Hyper/Renderer/RenderContext.h"
#include "Hyper/Renderer/ShaderCompiler.h"

namespace Hyper
{
	VulkanShader::VulkanShader(RenderContext* pRenderCtx, std::unordered_map<ShaderStageType, std::filesystem::path> shaders)
		: m_pRenderCtx(pRenderCtx), m_Id(UUID{})
	{
		for (const auto& [stage, path] : shaders)
		{
			m_ShaderModules[stage] = ShaderModule {
				.shaderPath = path,
				.module = VK_NULL_HANDLE
			};
		}

		Create();
	}

	VulkanShader::~VulkanShader()
	{
		Destroy();
	}

	void VulkanShader::Reload()
	{
		VulkanUtils::CheckResult(m_pRenderCtx->device.waitIdle());

		// TODO: handle shader compilation errors.
		Destroy();
		Create();
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

	void VulkanShader::Create()
	{
		for (const auto& [stage, module] : m_ShaderModules)
		{
			// Compile the shader into SPIR-V bytes
			std::vector<u32> spirvBytes = ShaderCompiler::Compile(module.shaderPath, stage);

			// Create shader module
			vk::ShaderModuleCreateInfo createInfo = {};
			createInfo.setCode(spirvBytes);

			vk::ShaderModule shaderModule = VulkanUtils::Check(m_pRenderCtx->device.createShaderModule(createInfo));

			// Store compiled shader and module
			m_ShaderModules.insert_or_assign(stage, ShaderModule{
				module.shaderPath,
				shaderModule
				});

			bool doReflection = true;
			switch (stage)
			{
				// We can't do reflection on RT shaders because of invalid validation errors about overlapping descriptors.
			case ShaderStageType::RayGen:
			case ShaderStageType::Miss:
			case ShaderStageType::ClosestHit:
				doReflection = false;
				break;

			default:
				doReflection = true;
				break;
			}
			if (doReflection)
			{
				Reflect(stage, spirvBytes, module.shaderPath.string());
			}
		}

		// Create descriptors if 
		{
			for (const auto& descriptorSet : m_DescriptorSetBindings)
			{
				auto builder = DescriptorSetLayoutBuilder(m_pRenderCtx->device);
				for (const auto& [binding, descriptor] : descriptorSet)
				{
					builder = builder.AddBinding(descriptor.descType, binding, 1, descriptor.stageFlags);
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
		}

		HPR_VKLOG_INFO("Successfully created shaders");
	}

	void VulkanShader::Destroy()
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

		m_DescriptorSetBindings.clear();
		m_PushConstants.clear();
		m_DescriptorLayouts.clear();
		m_PushConstRanges.clear();
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

			if (descriptorSet >= m_DescriptorSetBindings.size())
				m_DescriptorSetBindings.resize(descriptorSet + 1);

			if (m_DescriptorSetBindings[descriptorSet].contains(binding))
			{
				HPR_VKLOG_ERROR("Shader validation error: descriptor in set {} binding {} already exists!", descriptorSet, binding);
				break;
			}

			m_DescriptorSetBindings[descriptorSet][binding] = {
				.descType = vk::DescriptorType::eUniformBuffer,
				.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage)
			};

			HPR_VKLOG_INFO("Uniform buffer: '{}'", name);
			HPR_VKLOG_INFO("  binding: {}", binding);
			HPR_VKLOG_INFO("  set: {}", descriptorSet);
		}

		// Sampled images
		for (const auto& resource : resources.sampled_images)
		{
			const auto& name = resource.name;
			u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			u32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

			if (descriptorSet >= m_DescriptorSetBindings.size())
				m_DescriptorSetBindings.resize(descriptorSet + 1);

			if (m_DescriptorSetBindings[descriptorSet].contains(binding))
			{
				HPR_VKLOG_ERROR("Shader validation error: descriptor in set {} binding {} already exists!", descriptorSet, binding);
				break;
			}

			m_DescriptorSetBindings[descriptorSet][binding] = {
				.descType = vk::DescriptorType::eCombinedImageSampler,
				.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage)
			};

			HPR_VKLOG_INFO("Combined Image Sampler: '{}'", name);
			HPR_VKLOG_INFO("  binding: {}", binding);
			HPR_VKLOG_INFO("  set: {}", descriptorSet);
		}

		// Separate images
		for (const auto& resource : resources.separate_images)
		{
			const auto& name = resource.name;
			u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			u32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

			if (descriptorSet >= m_DescriptorSetBindings.size())
				m_DescriptorSetBindings.resize(descriptorSet + 1);

			if (m_DescriptorSetBindings[descriptorSet].contains(binding))
			{
				if (m_DescriptorSetBindings[descriptorSet][binding].descType == vk::DescriptorType::eSampler)
				{
					// If the existing descriptor is a separate sampler, we can combine it into a CombinedImageSampler
					m_DescriptorSetBindings[descriptorSet][binding].descType = vk::DescriptorType::eCombinedImageSampler;
				}
				else
				{
					// Else, we have overlapping and mismatching descriptors, we can't have that!
					HPR_VKLOG_ERROR("Shader validation error: descriptor in set {} binding {} already exists and can't be combined into a CombinedImageSampler!", descriptorSet, binding);
					break;
				}
			}
			else
			{
				m_DescriptorSetBindings[descriptorSet][binding] = {
					.descType = vk::DescriptorType::eSampledImage,
					.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage)
				};
			}

			HPR_VKLOG_INFO("Separate Image: '{}'", name);
			HPR_VKLOG_INFO("  binding: {}", binding);
			HPR_VKLOG_INFO("  set: {}", descriptorSet);
		}

		// Separate samplers
		for (const auto& resource : resources.separate_samplers)
		{
			const auto& name = resource.name;
			u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			u32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

			if (descriptorSet >= m_DescriptorSetBindings.size())
				m_DescriptorSetBindings.resize(descriptorSet + 1);

			if (m_DescriptorSetBindings[descriptorSet].contains(binding))
			{
				if (m_DescriptorSetBindings[descriptorSet][binding].descType == vk::DescriptorType::eSampledImage)
				{
					// If the existing descriptor is a separate sampler, we can combine it into a CombinedImageSampler
					m_DescriptorSetBindings[descriptorSet][binding].descType = vk::DescriptorType::eCombinedImageSampler;
				}
				else
				{
					// Else, we have overlapping and mismatching descriptors, we can't have that!
					HPR_VKLOG_ERROR("Shader validation error: descriptor in set {} binding {} already exists and can't be combined into a CombinedImageSampler!", descriptorSet, binding);
					break;
				}
			}
			else
			{
				m_DescriptorSetBindings[descriptorSet][binding] = {
					.descType = vk::DescriptorType::eSampler,
					.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage)
				};
			}

			HPR_VKLOG_INFO("Separate sampler: '{}'", name);
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

			if (m_DescriptorSetBindings[descriptorSet].contains(binding))
			{
				HPR_VKLOG_ERROR("Shader validation error: descriptor in set {} binding {} already exists!", descriptorSet, binding);
				break;
			}

			m_DescriptorSetBindings[descriptorSet][binding] = {
				.descType = vk::DescriptorType::eStorageImage,
				.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage)
			};

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

			if (m_DescriptorSetBindings[descriptorSet].contains(binding))
			{
				HPR_VKLOG_ERROR("Shader validation error: descriptor in set {} binding {} already exists!", descriptorSet, binding);
				break;
			}

			m_DescriptorSetBindings[descriptorSet][binding] = {
				.descType = vk::DescriptorType::eStorageBuffer,
				.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage)
			};

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

			if (m_DescriptorSetBindings[descriptorSet].contains(binding))
			{
				HPR_VKLOG_ERROR("Shader validation error: descriptor in set {} binding {} already exists!", descriptorSet, binding);
				break;
			}

			m_DescriptorSetBindings[descriptorSet][binding] = {
				.descType = vk::DescriptorType::eAccelerationStructureKHR,
				.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage)
			};

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
			pushConst.size = bufferSize;
			pushConst.offset = bufferOffset;
			pushConst.stageFlags = static_cast<vk::ShaderStageFlagBits>(stage);

			HPR_VKLOG_INFO("Push constant: '{}'", name);
			HPR_VKLOG_INFO("  size: {}", pushConst.size);
			HPR_VKLOG_INFO("  offset: {}", pushConst.offset);
		}
	}
}
