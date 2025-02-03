#include "VulkanWindow.h"

#include <stdexcept>

VulkanWindow::VulkanWindow(int width, int height, const char* title) : m_width{ width }, m_height{ height }, m_title{ title }
{
	initWindow();
}

VulkanWindow::~VulkanWindow()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void VulkanWindow::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_window = glfwCreateWindow(m_width, m_height, m_title, nullptr, nullptr);

	if (!m_window)
		throw std::runtime_error("Failed to create Vulkan window!");
}


