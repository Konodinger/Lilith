#ifndef __LTH_PIPELINE_HPP__
#define __LTH_PIPELINE_HPP__

#include "../lth_device.hpp"

#include <string>
#include <vector>
#include "../lth_shader_compiler.hpp"

#include <slang/slang-com-ptr.h>
#include <slang/slang.h>

namespace lth {

	class LthPipeline {
	public:
		LthPipeline(LthDevice& device, VkPipelineLayout pipelineLayout, LthShaderCompiler& shaderCompiler)
			: lthDevice(device), lthPipelineLayout(pipelineLayout), lthShaderCompiler{ shaderCompiler } {};
		virtual ~LthPipeline() {};

		LthPipeline() = default;
		LthPipeline(const LthPipeline&) = delete;
		LthPipeline& operator=(const LthPipeline&) = delete;
		virtual void clearPipeline() = 0;
		virtual void reloadPipeline() = 0;

		bool checkForUpdatesAndReload();
		virtual void bind(VkCommandBuffer commandBuffer) = 0;
	protected:
		static std::vector<char> readFile(const std::string& filePath);
		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
		void createShaderModule(const Slang::ComPtr<slang::IBlob>& code, VkShaderModule* shaderModule);
		
		LthDevice& lthDevice;
		const VkPipelineLayout lthPipelineLayout;
		LthShaderCompiler& lthShaderCompiler;
		std::vector<LthSlangSession> slangSessions = {};
	};
}

#endif