#ifndef __LTH_PIPELINE_HPP__
#define __LTH_PIPELINE_HPP__

#include "../lth_device.hpp"

#include <string>
#include <vector>

namespace lth {

	class LthPipeline {
	public:
		LthPipeline(LthDevice& device) : lthDevice(device) {};
		virtual ~LthPipeline() {};

		LthPipeline() = default;
		LthPipeline(const LthPipeline&) = delete;
		LthPipeline& operator=(const LthPipeline&) = delete;

		virtual void bind(VkCommandBuffer commandBuffer) = 0;
	protected:
		static std::vector<char> readFile(const std::string& filePath);
		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
		LthDevice& lthDevice;
	};
}

#endif