#pragma once
#include "VulkanWindow.h"

class VulkanApplication
{
public:
	static constexpr int WIDTH = 800;
	static constexpr int HEIGHT = 600;

public:
	void run();

private:
	VulkanWindow m_window{ WIDTH, HEIGHT, "Hello Vulkan!" };
};

