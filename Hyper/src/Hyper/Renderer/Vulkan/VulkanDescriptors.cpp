#include "HyperPCH.h"
#include "VulkanDescriptors.h"

#include "VulkanUtility.h"

namespace Hyper
{
	DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder(vk::Device device)
		: m_Device(device)
	{
	}

	DescriptorSetLayoutBuilder DescriptorSetLayoutBuilder::AddBinding(vk::DescriptorType descriptorType, u32 binding, u32 count, vk::ShaderStageFlags stageFlags)
	{
		m_Bindings.emplace_back(vk::DescriptorSetLayoutBinding{
			binding,
			descriptorType,
			count,
			stageFlags
		});
		return *this;
	}

	vk::DescriptorSetLayout DescriptorSetLayoutBuilder::Build()
	{
		vk::DescriptorSetLayoutCreateInfo info = {};
		info.setBindings(m_Bindings);

		const vk::DescriptorSetLayout layout = VulkanUtils::Check(m_Device.createDescriptorSetLayout(info));

		return layout;
	}

	DescriptorPool::Builder::Builder(vk::Device device)
		: m_Device(device)
	{
	}

	DescriptorPool::Builder DescriptorPool::Builder::AddSize(vk::DescriptorType type, u32 count)
	{
		m_Sizes.emplace_back(vk::DescriptorPoolSize{ type, count });
		return *this;
	}

	DescriptorPool::Builder DescriptorPool::Builder::SetMaxSets(u32 maxSets)
	{
		m_MaxSets = maxSets;
		return *this;
	}

	DescriptorPool::Builder DescriptorPool::Builder::SetFlags(vk::DescriptorPoolCreateFlags flags)
	{
		m_Flags = flags;
		return *this;
	}

	DescriptorPool DescriptorPool::Builder::Build()
	{
		auto temp = DescriptorPool{ m_Device, m_Sizes, m_MaxSets, m_Flags };
		return temp;
	}

	DescriptorPool::DescriptorPool(vk::Device device, const std::vector<vk::DescriptorPoolSize>& sizes, u32 maxSets, vk::DescriptorPoolCreateFlags flags)
		: m_Device(device)
	{
		vk::DescriptorPoolCreateInfo info = {};
		info.maxSets = maxSets;
		info.setPoolSizes(sizes);
		info.flags = flags;

		m_Pool = VulkanUtils::Check(device.createDescriptorPool(info));
	}

	DescriptorPool::~DescriptorPool()
	{
		if (m_Device && m_Pool)
		{
			m_Device.resetDescriptorPool(m_Pool); // Make sure to release all the descriptor sets
			m_Device.destroyDescriptorPool(m_Pool);
		}
	}

	DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
		: m_Device(other.m_Device), m_Pool(other.m_Pool)
	{
		other.m_Device = nullptr;
		other.m_Pool = nullptr;
	}

	DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept
	{
		m_Device = other.m_Device;
		m_Pool = other.m_Pool;
		other.m_Device = nullptr;
		other.m_Pool = nullptr;

		return *this;
	}

	std::vector<vk::DescriptorSet> DescriptorPool::Allocate(const std::vector<vk::DescriptorSetLayout>& layouts) const
	{
		vk::DescriptorSetAllocateInfo info = {};
		info.descriptorPool = m_Pool;
		info.setSetLayouts(layouts);

		const std::vector<vk::DescriptorSet> sets = VulkanUtils::Check(m_Device.allocateDescriptorSets(info));

		return sets;
	}

	DescriptorWriter::DescriptorWriter(vk::Device device, vk::DescriptorSet descriptorSet)
		: m_Device(device), m_DescriptorSet(descriptorSet)
	{
	}

	void DescriptorWriter::WriteBuffer(const vk::DescriptorBufferInfo& bufferInfo, u32 dstBinding, vk::DescriptorType type)
	{
		vk::WriteDescriptorSet setWrite = {};
		setWrite.dstBinding = dstBinding;
		setWrite.dstSet = m_DescriptorSet;
		setWrite.descriptorCount = 1;
		setWrite.descriptorType = type;
		setWrite.setBufferInfo(bufferInfo);

		m_DescriptorWrites.push_back(setWrite);
	}
	
	void DescriptorWriter::WriteImage(const vk::DescriptorImageInfo& imageInfo, u32 dstBinding, vk::DescriptorType type)
	{
		vk::WriteDescriptorSet setWrite = {};
		setWrite.dstBinding = dstBinding;
		setWrite.dstSet = m_DescriptorSet;
		setWrite.descriptorCount = 1;
		setWrite.descriptorType = type;
		setWrite.setImageInfo(imageInfo);

		m_DescriptorWrites.push_back(setWrite);
	}

	void DescriptorWriter::WriteAccelStructure(const vk::WriteDescriptorSetAccelerationStructureKHR* accelInfo, u32 dstBinding)
	{
		vk::WriteDescriptorSet setWrite = {};
		setWrite.pNext = accelInfo;

		setWrite.dstBinding = dstBinding;
		setWrite.dstSet = m_DescriptorSet;
		setWrite.descriptorCount = 1;
		setWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;

		m_DescriptorWrites.push_back(setWrite);
	}

	void DescriptorWriter::Write()
	{
		m_Device.updateDescriptorSets(m_DescriptorWrites, {});
	}
}
