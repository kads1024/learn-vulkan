#pragma once
#include "VulkanWindow.h"
#include "VulkanPipeline.h"

class VulkanApplication
{
public:
	static constexpr int WIDTH = 800;
	static constexpr int HEIGHT = 600;

public:
	void run();

private:
	VulkanWindow m_window{ WIDTH, HEIGHT, "Hello Vulkan!" };
	VulkanPipeline m_pipeline{ "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv" };
};

