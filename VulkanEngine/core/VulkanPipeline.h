#pragma once

#include <vector>

class VulkanPipeline
{
public:
	VulkanPipeline(const char* vertFilePath, const char* fragFilePath);

private:
	static std::vector<char> readFile(const char* filePath);

	void createGraphicsPipeline(const char* vertFilePath, const char* fragFilePath);
};

