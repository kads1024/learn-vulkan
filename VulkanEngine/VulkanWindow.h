#pragma once

#include <GLFW/glfw3.h>

class VulkanWindow
{
public:
	VulkanWindow(int width, int height, const char* title);
	~VulkanWindow();

	VulkanWindow(const VulkanWindow&) = delete;
	VulkanWindow& operator=(const VulkanWindow&) = delete;

	inline bool isOpen() { return !glfwWindowShouldClose(m_window); }

private:
	GLFWwindow* m_window;

	const int m_width;
	const int m_height;
	const char* m_title;

private:
	void initWindow();
};

