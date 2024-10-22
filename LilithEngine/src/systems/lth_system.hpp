#ifndef __SYSTEM_HPP__
#define __SYSTEM_HPP__

#include "../lth_device.hpp"
#include "../lth_descriptors.hpp"

#include <string>

namespace lth {

	class LthSystem {
	public:
		LthSystem(LthDevice& device,
			VkRenderPass renderPass,
			DescriptorSetLayouts& setLayouts) :
			lthDevice{ device } {};
	protected:
		void createPipelineLayout(VkPipelineLayout* pipelineLayout,
			const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts = {},
			const std::vector<VkPushConstantRange>& pushConstantRanges = {});
		virtual void createPipeline(VkRenderPass renderPass) = 0;
		LthDevice& lthDevice;
	};
}

#endif