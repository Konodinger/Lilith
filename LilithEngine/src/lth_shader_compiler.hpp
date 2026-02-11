#ifndef __LTH_SHADER_COMPILER_HPP__
#define __LTH_SHADER_COMPILER_HPP__

#include "lth_device.hpp"

#include <slang/slang-com-ptr.h>
#include <slang/slang.h>
#include <unordered_map>

namespace lth {

	struct LthSlangSession {
		slang::SessionDesc sessionDesc;
		Slang::ComPtr<slang::ISession> handle;
		std::unordered_map<std::string, slang::IBlob*> modulesToUpdate;
	};

	class LthShaderCompiler {
	public:
		LthShaderCompiler(LthDevice& device);
		~LthShaderCompiler() = default;

		LthShaderCompiler(const LthShaderCompiler&) = delete;
		LthShaderCompiler& operator=(const LthShaderCompiler&) = delete;
		LthShaderCompiler(LthShaderCompiler&&) = default;
		LthShaderCompiler& operator=(LthShaderCompiler&&) = default;

		LthSlangSession createSlangSession(const slang::SessionDesc& sessionDesc);
		LthSlangSession createDefaultSlangSession() { return createSlangSession(defaultSessionDesc); };

		// Shader module generator for both slang and Spir-V shaders.
		// Spir-V is supported for legacy purpose, but only the filePath is required in that case, the other parameter are unused.
		VkShaderModule createShaderModule(
			const std::string& filePath,
			LthSlangSession* slangSession = nullptr,
			const std::string& entryPointName = "main",
			bool checkForUpdate = true);

		bool checkForUpdates(LthSlangSession& slangSession, bool autoRebootSession = true);

	private:
		LthDevice& lthDevice;
		Slang::ComPtr<slang::IGlobalSession> globalSession;
		slang::SessionDesc defaultSessionDesc;

		std::vector<const char*> defaultSearchPaths;
		slang::TargetDesc defaultTargetDesc;
	};
}

#endif