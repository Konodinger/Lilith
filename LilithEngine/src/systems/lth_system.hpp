#ifndef __SYSTEM_HPP__
#define __SYSTEM_HPP__

#include "../lth_device.hpp"
#include "../lth_descriptors.hpp"
#include "../lth_shader_compiler.hpp"

#include <string>

namespace lth {

	class LthSystem {
	public:
		LthSystem(LthDevice& device,
			LthShaderCompiler& shaderCompiler,
			VkRenderPass renderPass,
			DescriptorSetLayouts& setLayouts) :
			lthDevice{ device }, lthShaderCompiler{ shaderCompiler } {};

		virtual bool checkForPipelineUpdates() = 0;
	protected:
		void createPipelineLayout(VkPipelineLayout* pipelineLayout,
			const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts = {},
			const std::vector<VkPushConstantRange>& pushConstantRanges = {});
		virtual void createPipeline(VkRenderPass renderPass) = 0;
		LthDevice& lthDevice;
		LthShaderCompiler& lthShaderCompiler;
	};
}

#endif