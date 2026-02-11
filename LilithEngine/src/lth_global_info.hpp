#ifndef __LTH_GLOBAL_INFO_HPP__
#define __LTH_GLOBAL_INFO_HPP__

#include <stdint.h>

#define SHADERSPIRVFOLDERPATH(fileName) "shadersSpirv/" + std::string(fileName) + ".spv"
#define MODELSFOLDERPATH(fileName) "models/" + std::string(fileName)
#define TEXTURESFOLDERPATH(fileName) "textures/" + std::string(fileName)
#define IMGUIFONTSFOLDERPATH(fileName) "src/libraries/imgui/misc/fonts/" fileName
#define DEFAULTTEXTURE "white_pixel.png"

namespace lth {
	static constexpr int WIDTH = 1200;
	static constexpr int HEIGHT = 800;

	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
	static constexpr uint32_t GLOBALPOOLMAXSETS = 100;
	static constexpr uint32_t TEXTUREARRAYSIZE = 10;

	enum LTH_UPDATE_DT_MODE {
		LTH_UPDATE_DT_MODE_CONSTANT_DT_ONE_CALL,
		LTH_UPDATE_DT_MODE_CONSTANT_DT_MULTIPLE_CALL,
		LTH_UPDATE_DT_MODE_CONSTANT_DT_MULTIPLE_CALL_CAPPED,
		LTH_UPDATE_DT_MODE_ADAPTIVE_DT,
		LTH_UPDATE_DT_MODE_ADAPTIVE_DT_CAPPED
	};
	static LTH_UPDATE_DT_MODE UPDATE_DELTA_TIME_MODE = LTH_UPDATE_DT_MODE_CONSTANT_DT_MULTIPLE_CALL_CAPPED;

	static bool LTH_CONSTANT_UPDATE_DT = true; //This parameters is in caps lock because it could become an enum in the future.
	static constexpr float MAX_FRAME_TIME = 0.25f;
	static constexpr float UPDATE_DT = 0.01f;
	static constexpr float UPDATE_DT_HALF = UPDATE_DT / 2.f;

	static constexpr uint32_t MAX_RAY_RECURSION_DEPTH = 3U;
}

#endif