#ifndef __LTH_DESCRIPTORS_HPP__
#define __LTH_DESCRIPTORS_HPP__

#include "lth_device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace lth {

    class LthDescriptorSetLayout {
    public:
        class Builder {
        public:
            Builder(LthDevice& lthDevice) : lthDevice{ lthDevice } {}

            Builder& addBinding(
                uint32_t binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                uint32_t count = 1);
            std::unique_ptr<LthDescriptorSetLayout> build() const;

        private:
            LthDevice& lthDevice;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
        };

        LthDescriptorSetLayout(
            LthDevice& lthDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
        ~LthDescriptorSetLayout();
        LthDescriptorSetLayout(const LthDescriptorSetLayout&) = delete;
        LthDescriptorSetLayout& operator=(const LthDescriptorSetLayout&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

    private:
        LthDevice& lthDevice;
        VkDescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

        friend class LthDescriptorWriter;
    };

    class LthDescriptorPool {
    public:
        class Builder {
        public:
            Builder(LthDevice& lthDevice) : lthDevice{ lthDevice } {}

            Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& setMaxSets(uint32_t count);
            std::unique_ptr<LthDescriptorPool> build() const;

        private:
            LthDevice& lthDevice;
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };

        LthDescriptorPool(
            LthDevice& lthDevice,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize>& poolSizes);
        ~LthDescriptorPool();
        LthDescriptorPool(const LthDescriptorPool&) = delete;
        LthDescriptorPool& operator=(const LthDescriptorPool&) = delete;

        bool allocateDescriptorSets(
            const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

        void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

        void resetPool();

    private:
        LthDevice& lthDevice;
        VkDescriptorPool descriptorPool;

        friend class LthDescriptorWriter;
    };

    class LthDescriptorWriter {
    public:
        LthDescriptorWriter(LthDescriptorSetLayout& setLayout, LthDescriptorPool& pool);

        LthDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
        LthDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

        bool build(VkDescriptorSet& set);
        void overwrite(VkDescriptorSet& set);

    private:
        LthDescriptorSetLayout& setLayout;
        LthDescriptorPool& pool;
        std::vector<VkWriteDescriptorSet> writes;
    };

}

#endif