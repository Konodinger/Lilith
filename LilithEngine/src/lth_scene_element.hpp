#ifndef __LTH_SCENE_ELEMENT_HPP__
#define __LTH_SCENE_ELEMENT_HPP__

#include <type_traits>
#include <unordered_map>
#include <memory>

namespace lth {

	using id_t = unsigned int;

	class LthSceneElement {
	public:
		LthSceneElement(const LthSceneElement&) = delete;
		LthSceneElement& operator=(const LthSceneElement&) = delete;
		LthSceneElement(LthSceneElement&&) = default;
		LthSceneElement& operator=(LthSceneElement&&) = default;

		id_t getId() const { return id; }

	protected:
		LthSceneElement(id_t id) : id(id) {};
		const id_t id;
	};

	template<typename T> requires std::is_base_of_v<LthSceneElement, T>
	using ElementMap = std::unordered_map<id_t, std::shared_ptr<T>>;

}

#endif