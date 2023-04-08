#pragma once

// These helpers are based on Brendan Galea's implementation in his littelVulkanEngine tutorial series:
// https://github.com/blurrypiano/littleVulkanEngine/blob/master/littleVulkanEngine/tutorial20/lve_descriptors.hpp

namespace Hyper
{
	class DescriptorSetLayoutBuilder
	{
	public:
		DescriptorSetLayoutBuilder(vk::Device device);

		DescriptorSetLayoutBuilder& AddBinding(vk::DescriptorType descriptorType, u32 binding, u32 count, vk::ShaderStageFlags stageFlags);
		DescriptorSetLayoutBuilder& SetFlags(vk::DescriptorSetLayoutCreateFlags flags);
		vk::DescriptorSetLayout Build(const void* pNext = nullptr);

	private:
		vk::Device m_Device;
		std::vector<vk::DescriptorSetLayoutBinding> m_Bindings{};
		vk::DescriptorSetLayoutCreateFlags m_Flags{};
	};

	class DescriptorPool
	{
	public:
		class Builder
		{
		public:
			Builder(vk::Device device);

			Builder& AddSize(vk::DescriptorType type, u32 count);
			Builder& SetMaxSets(u32 maxSets);
			Builder& SetFlags(vk::DescriptorPoolCreateFlags flags);
			DescriptorPool Build();

		private:
			vk::Device m_Device;
			u32 m_MaxSets{};
			std::vector<vk::DescriptorPoolSize> m_Sizes{};
			vk::DescriptorPoolCreateFlags m_Flags{};
		};

		DescriptorPool(vk::Device device, const std::vector<vk::DescriptorPoolSize>& sizes, u32 maxSets, vk::DescriptorPoolCreateFlags flags);
		~DescriptorPool();

		DescriptorPool(const DescriptorPool& other) = delete;
		DescriptorPool& operator=(const DescriptorPool& other) = delete;
		DescriptorPool(DescriptorPool&& other) noexcept;
		DescriptorPool& operator=(DescriptorPool&& other) noexcept;

		std::vector<vk::DescriptorSet> Allocate(const std::vector<vk::DescriptorSetLayout>& layouts) const;
		vk::DescriptorSet Allocate(const vk::DescriptorSetLayout& layout) const;

		[[nodiscard]] vk::DescriptorPool GetPool() const { return m_Pool; }

	private:
		vk::Device m_Device;
		vk::DescriptorPool m_Pool;
	};

	class DescriptorWriter
	{
	public:
		DescriptorWriter(vk::Device device, vk::DescriptorSet descriptorSet);

		void WriteBuffer(const vk::DescriptorBufferInfo& bufferInfo, u32 dstBinding, vk::DescriptorType type);
		void WriteImage(const vk::DescriptorImageInfo& imageInfo, u32 dstBinding, vk::DescriptorType type);
		void WriteImageBindless(const vk::DescriptorImageInfo& imageInfo, u32 dstBinding, u32 arrayElement, vk::DescriptorType type);
		void WriteAccelStructure(const vk::WriteDescriptorSetAccelerationStructureKHR* accelInfo, u32 dstBinding);
		void Write();

		void ClearWrites();

	private:
		vk::Device m_Device;
		vk::DescriptorSet m_DescriptorSet{};
		std::vector<vk::WriteDescriptorSet> m_DescriptorWrites{};
	};
}
