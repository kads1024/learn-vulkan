#include "VulkanPipeline.h"
#include <fstream>
#include <stdexcept>
#include <iostream>

VulkanPipeline::VulkanPipeline(const char* vertFilePath, const char* fragFilePath)
{
	createGraphicsPipeline(vertFilePath, fragFilePath);
}

std::vector<char> VulkanPipeline::readFile(const char* filePath)
{
	std::ifstream file{ filePath, std::ios::ate | std::ios::binary };

	if (!file.is_open()) {
		throw std::runtime_error(std::string("Failed to open file: ") + filePath);
	}

	size_t fileSize = static_cast<size_t>(file.tellg()); 

	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

void VulkanPipeline::createGraphicsPipeline(const char* vertFilePath, const char* fragFilePath)
{
	auto vertCode = readFile(vertFilePath);
	auto fragCode = readFile(fragFilePath);

	std::cout << "Vertex Shader Code Size: " << vertCode.size() << std::endl;
	std::cout << "Fragment Shader Code Size: " << fragCode.size() << std::endl;
}
