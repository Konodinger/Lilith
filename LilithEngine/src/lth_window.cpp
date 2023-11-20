#include "lth_window.hpp"

#include <stdexcept>

namespace lth {

	LthWindow::LthWindow(int w, int h, std::string name) : width(w), height(h), windowName(name) {
		initWindow();
	}

	LthWindow::~LthWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void LthWindow::initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	void LthWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface!");
		}
	}

	void LthWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto lthWindow = reinterpret_cast<LthWindow*>(glfwGetWindowUserPointer(window));
		lthWindow->framebufferResized = true;
		lthWindow->width = width;
		lthWindow->height = height;
	}
}