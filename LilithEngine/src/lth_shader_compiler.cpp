#include "lth_shader_compiler.hpp"

#include <stdexcept>
#include <iostream>
#include <fstream>

using namespace slang;
using namespace Slang;

namespace lth {

	LthShaderCompiler::LthShaderCompiler(LthDevice& device) : lthDevice{ device } {
		SlangGlobalSessionDesc globalSessionDesc = {};
		globalSessionDesc.enableGLSL = true;
		createGlobalSession(&globalSessionDesc, globalSession.writeRef());

		defaultSearchPaths = {};
		defaultSearchPaths.push_back("shaders");
		defaultSearchPaths.push_back("shaders/rayTracing");

		defaultTargetDesc = {};
		defaultTargetDesc.format = SLANG_SPIRV;
		defaultTargetDesc.profile = globalSession->findProfile("spirv_1_4");
		defaultTargetDesc.flags = 0;

		defaultSessionDesc.searchPathCount = defaultSearchPaths.size();
		defaultSessionDesc.searchPaths = defaultSearchPaths.data();
		defaultSessionDesc.targets = &defaultTargetDesc;
		defaultSessionDesc.targetCount = 1;
		defaultSessionDesc.allowGLSLSyntax = true;
	}

	LthSlangSession LthShaderCompiler::createSlangSession(const SessionDesc& sessionDesc) {
		LthSlangSession lthSession;
		if (SLANG_FAILED(globalSession->createSession(sessionDesc, lthSession.handle.writeRef()))) {
			throw std::runtime_error("Failed to create a slang shader compilation session.");
		}
		lthSession.sessionDesc = sessionDesc;
		return lthSession;
	}

	VkShaderModule LthShaderCompiler::createShaderModule(
		const std::string& filePath,
		LthSlangSession* slangSession,
		const std::string& entryPointName,
		bool checkForUpdate) {

		const uint32_t* pCode;
		size_t codeSize;


		if (filePath.ends_with(".spv")) {
			std::ifstream file(filePath, std::ios::ate | std::ios::binary);

			if (!file.is_open()) {
				throw std::runtime_error("Failed to open file: " + filePath + "!");
			}

			codeSize = static_cast<size_t>(file.tellg());
			std::vector<char> buffer(codeSize);

			file.seekg(0);
			file.read(buffer.data(), codeSize);
			file.close();

			pCode = reinterpret_cast<const uint32_t*>(buffer.data());
		}
		else if (filePath.ends_with(".slang")) {
			if (slangSession == nullptr) {
				throw std::runtime_error("Can't compile a slang shader without defining a proper slang session.");
			}

			ComPtr<IBlob> diagnosticBlob;
			SlangResult result;
			IModule* slangModule = slangSession->handle->loadModule(filePath.c_str(), diagnosticBlob.writeRef());
			if (diagnosticBlob) {
				std::cout << (const char*)diagnosticBlob->getBufferPointer() << std::endl;
			}
			if (!slangModule) {
				throw std::runtime_error("Failed to load a slang shader module");
			}

			ComPtr<IEntryPoint> entryPoint;
			slangModule->findEntryPointByName(entryPointName.c_str(), entryPoint.writeRef());

			std::vector<IComponentType*> componentTypes;
			componentTypes.push_back(slangModule);
			componentTypes.push_back(entryPoint);

			ComPtr<IComponentType> composedProgram;
			result = slangSession->handle->createCompositeComponentType(
				componentTypes.data(),
				componentTypes.size(),
				composedProgram.writeRef(),
				diagnosticBlob.writeRef());
			if (diagnosticBlob) {
				std::cout << (const char*)diagnosticBlob->getBufferPointer() << std::endl;
			}
			if (SLANG_FAILED(result)) {
				throw std::runtime_error("Failed to create a slang composite component");
			}

			ComPtr<IComponentType> linkedProgram;
			result = composedProgram->link(
				linkedProgram.writeRef(),
				diagnosticBlob.writeRef());
			if (diagnosticBlob) {
				std::cout << (const char*)diagnosticBlob->getBufferPointer() << std::endl;
			}
			if (SLANG_FAILED(result)) {
				throw std::runtime_error("Failed to link a slang composite component");
			}
			

			ComPtr<IBlob> spirvCode;
			result = composedProgram->getEntryPointCode(
				0,
				0,
				spirvCode.writeRef(),
				diagnosticBlob.writeRef());
			if (diagnosticBlob) {
				std::cout << (const char*)diagnosticBlob->getBufferPointer() << std::endl;
			}
			if (SLANG_FAILED(result)) {
				throw std::runtime_error("Failed to get the entry point of a slang composite component");
			}

			codeSize = spirvCode->getBufferSize();
			pCode = reinterpret_cast<const uint32_t*>(spirvCode->getBufferPointer());

			if (checkForUpdate) {
				IBlob* serializedModule;
				slangModule->serialize(&serializedModule);
				slangSession->modulesToUpdate[filePath] = serializedModule;				
			}
		}
		else {
			throw std::runtime_error("Cannot generate a shader module out of a file that is neither a .spv or .slang file.");
		}

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = codeSize;
		createInfo.pCode = pCode;

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(lthDevice.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create shader module!");
		}

		return shaderModule;
	}

	bool LthShaderCompiler::checkForUpdates(LthSlangSession& slangSession, bool autoRebootSession) {
		Slang::ComPtr<slang::ISession> newSession;
		if (SLANG_FAILED(globalSession->createSession(slangSession.sessionDesc, newSession.writeRef()))) {
			throw std::runtime_error("Failed to create a slang shader compilation session.");
		}

		bool update = false;
		for (auto& module : slangSession.modulesToUpdate) {
			if (!newSession->isBinaryModuleUpToDate(module.first.c_str(), module.second)) {
				std::cout << "The module " << module.first << " needs an update." << std::endl;
				update = true;
			}
		}

		if (update && autoRebootSession) {
			slangSession.handle = newSession;
			slangSession.modulesToUpdate.clear();
		}
		return update;
	}

}