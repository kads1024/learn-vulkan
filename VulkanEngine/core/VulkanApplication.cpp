#include "VulkanApplication.h"

void VulkanApplication::run()
{
	while (m_window.isOpen()) {
		glfwPollEvents();
	}
}
