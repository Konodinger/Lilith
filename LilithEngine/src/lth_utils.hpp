#ifndef __LTH_UTILS_HPP__
#define __LTH_UTILS_HPP__

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <functional>

namespace lth {

	const float PI_OVER_TWO = 1.5707963268f;
	const float PI = 3.1415926536f;
	const float THREE_PI_OVER_TWO = 4.7123889804f;
	const float TWO_PI = 6.2831853072f;

	// from: https://stackoverflow.com/a/57595105
	template <typename T, typename... Rest>
	void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
		seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		(hashCombine(seed, rest), ...);
	};

}

#endif