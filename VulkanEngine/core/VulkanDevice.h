#pragma once

#include "VulkanWindow.h"

class VulkanDevice
{
public:
	VulkanDevice(const VulkanWindow& window);
	~VulkanDevice();

	// Delete Copy Commands
	VulkanDevice(const VulkanDevice&) = delete;
	VulkanDevice& operator=(const VulkanDevice&) = delete;

	// Delete Move Commands
	VulkanDevice(VulkanDevice&&) = delete;
	VulkanDevice& operator=(VulkanDevice&&) = delete;
};

