#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <string>
#include <fstream>

const uint16_t WINDOW_WIDTH = 800;
const uint16_t WINDOW_HEIGHT = 600;

const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif // NDEBUG

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


// Proxy Function for Debug Util Messenger==================================================================
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
//=======================================================================================================
class HelloTriangleApplication {
public:
    void run() 
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* m_window;
    VkInstance m_vulkanInstance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkSurfaceKHR m_surface;

    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;

    // swap chain info
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;

    std::vector<VkImageView> m_swapChainImageViews;

    VkRenderPass m_renderPass;
  
    VkPipelineLayout m_pipelineLayout;

    VkPipeline m_graphicsPipeline;

    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    VkCommandPool m_commandPool;
    VkCommandBuffer m_commandBuffer;
private:
    void initWindow() 
    {
        if (glfwInit() != GLFW_TRUE)
            throw std::runtime_error("FAILED TO INITIALIZE GLFW!");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Tutorial", nullptr, nullptr);

        if(m_window == nullptr)
            throw std::runtime_error("FAILED TO CREATE WINDOW!");
    }

    void initVulkan()
    {
        createVulkanInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFrameBuffers();
        createCommandPool();
        createCommandBuffer();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);

        for (auto framebuffer : m_swapChainFramebuffers) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }

        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        for (const VkImageView& imageView : m_swapChainImageViews) {
            vkDestroyImageView(m_device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
        vkDestroyDevice(m_device, nullptr);

        if (ENABLE_VALIDATION_LAYERS)
            DestroyDebugUtilsMessengerEXT(m_vulkanInstance, m_debugMessenger, nullptr);

        vkDestroySurfaceKHR(m_vulkanInstance, m_surface, nullptr);
        vkDestroyInstance(m_vulkanInstance, nullptr);

        glfwDestroyWindow(m_window);
        glfwTerminate();

    }

    void createVulkanInstance()
    {
        // If validation layer is enabled AND one layer is not supported, crash
        if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport())
            throw std::runtime_error("Validation layers requested, but not all are available!");
        
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

      
        std::vector<const char*> extensions = getRequiredExtensions();

        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;
        instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (ENABLE_VALIDATION_LAYERS) {
            instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            instanceCreateInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
        }

        VkResult createInstanceResult = vkCreateInstance(&instanceCreateInfo, nullptr, &m_vulkanInstance);

        if (createInstanceResult != VK_SUCCESS) 
            throw std::runtime_error("FAILED TO CREATE VULKAN INSTANCE!");
        
    }

    bool checkValidationLayerSupport()
    {
        uint32_t validationLayerCount = 0;
        vkEnumerateInstanceLayerProperties(&validationLayerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(validationLayerCount);
        vkEnumerateInstanceLayerProperties(&validationLayerCount, availableLayers.data());

        for (const char* layer : VALIDATION_LAYERS) {
            bool layerExist = false;

            for (const VkLayerProperties& layerProperties : availableLayers) {
                if (strcmp(layer, layerProperties.layerName) == 0) {
                    layerExist = true;
                    break;
                }
            }

            if (!layerExist) 
                return false;
        }

        return true;
    }

    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (ENABLE_VALIDATION_LAYERS)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return extensions;
    }

    void setupDebugMessenger()
    {
        if (!ENABLE_VALIDATION_LAYERS) return;

        VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
        populateDebugMessengerCreateInfo(debugUtilsMessengerCreateInfo);

        VkResult debugUtilsMessengerCreateResult = CreateDebugUtilsMessengerEXT(m_vulkanInstance, &debugUtilsMessengerCreateInfo, nullptr, &m_debugMessenger);
        if (debugUtilsMessengerCreateResult != VK_SUCCESS)
            throw std::runtime_error("Failed to create Debug Messenger for Validation Layer");
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCreateInfo) {
        debugUtilsMessengerCreateInfo = {};
        debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugUtilsMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugUtilsMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugUtilsMessengerCreateInfo.pfnUserCallback = debugCallback;
    }

    void pickPhysicalDevice() 
    {
        m_physicalDevice = VK_NULL_HANDLE;

        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(m_vulkanInstance, &physicalDeviceCount, nullptr);

        if (physicalDeviceCount <= 0)
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");

        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(m_vulkanInstance, &physicalDeviceCount, physicalDevices.data());

        for (const VkPhysicalDevice& physicalDevice : physicalDevices) {
            if (isPhysicalDeviceSuitable(physicalDevice)) {
                m_physicalDevice = physicalDevice;
               
                break;
            }
        }

        if (m_physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    bool isPhysicalDeviceSuitable(const VkPhysicalDevice& physicalDevice)
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapchainDetails = querySwapChainSupport(physicalDevice);
            swapChainAdequate = !swapchainDetails.formats.empty() && !swapchainDetails.presentModes.empty();
        }
        return physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            queueFamilyIndices.isComplete() &&
            extensionsSupported && swapChainAdequate;
    }

    QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physicalDevice)
    {
        QueueFamilyIndices queueFamilyIndices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        int queueFamilyIndex = 0;
        for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queueFamilyIndices.graphicsFamily = queueFamilyIndex;
            }
            
            VkBool32 presentationSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, m_surface, &presentationSupport);
            if (presentationSupport) {
                queueFamilyIndices.presentFamily = queueFamilyIndex;
            }

            queueFamilyIndex++;

            if (queueFamilyIndices.isComplete()) {
                break;
            }
        }

        return queueFamilyIndices;
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice) 
    {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount, nullptr);
        
        if (formatCount > 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount, details.formats.data());
        }


        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount, nullptr);

        if (presentModeCount > 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount, details.presentModes.data());
        }


        return details;
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);
        float queuePriority = 1.0f;

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures physicalDeviceFeatures{};

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
        deviceCreateInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

        if (ENABLE_VALIDATION_LAYERS) {
            deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
        }

        VkResult deviceCreateResult = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);

        if (deviceCreateResult != VK_SUCCESS)
            throw std::runtime_error("Failed to Create Logical Device!");

        vkGetDeviceQueue(m_device, queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
    }

    void createSurface() 
    {
        VkResult createWindowSurfaceResult = glfwCreateWindowSurface(m_vulkanInstance, m_window, nullptr, &m_surface);
        if (createWindowSurfaceResult != VK_SUCCESS)
            throw std::runtime_error("Failed to create a window surface");

    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
    {
        uint32_t extensionsCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, nullptr);

        std::vector<VkExtensionProperties> physicalDeviceExtensions(extensionsCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, physicalDeviceExtensions.data());

        std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
        
        for (const VkExtensionProperties& extension : physicalDeviceExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        for (const VkPresentModeKHR& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(m_window, &width, &height);
            
            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void createSwapChain()
    {
        SwapChainSupportDetails swapChainDetails = querySwapChainSupport(m_physicalDevice);
        QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainDetails.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainDetails.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainDetails.capabilities);
        uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;

        if (swapChainDetails.capabilities.maxImageCount > 0 && imageCount > swapChainDetails.capabilities.maxImageCount) {
            imageCount = swapChainDetails.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

       
        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        createInfo.preTransform = swapChainDetails.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VkResult swapChainCreateResult = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain);
        if (swapChainCreateResult != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Swap Chain!");
        }

        uint32_t actualImageCount;
        vkGetSwapchainImagesKHR(m_device, m_swapChain, &actualImageCount, nullptr);

        m_swapChainImages.resize(actualImageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapChain, &actualImageCount, m_swapChainImages.data());

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
    }

    void createImageViews()
    {
        m_swapChainImageViews.resize(m_swapChainImages.size()); 

        for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
            VkImageViewCreateInfo imageViewCreateInfo{};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.image = m_swapChainImages[i];
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = m_swapChainImageFormat;
            imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;

            VkResult imageViewCreateResult = vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &m_swapChainImageViews[i]);
            if (imageViewCreateResult != VK_SUCCESS) {
                throw std::runtime_error("Failed to Create Image view for Image[" + std::to_string(i) + "]");
            }
        }
    }

    void createGraphicsPipeline() 
    {
        std::vector<char> vertexByte = readFile("shaders/vert.spv");
        std::vector<char> fragmentByte = readFile("shaders/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertexByte);
        VkShaderModule fragShaderModule = createShaderModule(fragmentByte);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

       

        std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
        vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
        vertexInputCreateInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
        vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
        inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

        // Dont really need this since we are using Dynamic state
       /* VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)m_swapChainExtent.width;
        viewport.height = (float)m_swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapChainExtent;*/

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizationState{};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.depthClampEnable = VK_FALSE;
        rasterizationState.rasterizerDiscardEnable = VK_FALSE;
        rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationState.lineWidth = 1.0f;
        rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationState.depthBiasEnable = VK_FALSE;
        rasterizationState.depthBiasConstantFactor = 0.0f; // Optional
        rasterizationState.depthBiasClamp = 0.0f; // Optional
        rasterizationState.depthBiasSlopeFactor = 0.0f; // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        VkResult pipelineLayoutCreateResult = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
        if (pipelineLayoutCreateResult != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputCreateInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizationState;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        VkResult graphicsPipelineCreateResult = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline);
        if (graphicsPipelineCreateResult != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    }

    VkShaderModule createShaderModule(const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo shaderCreateInfo{};
        shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderCreateInfo.codeSize = code.size();
        shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        VkResult shaderModuleResult = vkCreateShaderModule(m_device, &shaderCreateInfo, nullptr, &shaderModule);
        if (shaderModuleResult != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module!");
        }

        return shaderModule;
    }

    static std::vector<char> readFile(const std::string& fileName) 
    {
        std::ifstream file(fileName, std::ios::ate | std::ios::binary);

        if (!file.is_open()) 
            throw std::runtime_error("Failed to open " + fileName + "!");
        

        size_t fileSize = file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;

    }

    void createRenderPass() 
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        VkResult renderPassCreateResult = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
        if (renderPassCreateResult != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass!");
    }

    void createFrameBuffers() 
    {
        m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

        for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                m_swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_swapChainExtent.width;
            framebufferInfo.height = m_swapChainExtent.height;
            framebufferInfo.layers = 1;

            VkResult frameBufferResult = vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]);
            if (frameBufferResult != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        VkResult commandPoolResult = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
        if (commandPoolResult != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void createCommandBuffer()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkResult commandBufferResult = vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer);
        if (commandBufferResult != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) 
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        VkResult beginCommandBufferResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        if (beginCommandBufferResult != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapChainExtent;
        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapChainExtent.width);
        viewport.height = static_cast<float>(m_swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);

        VkResult endCommandBufferResult = vkEndCommandBuffer(commandBuffer);
        if (endCommandBufferResult != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
};



int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "[RUNTIME_ERROR]: " << e.what() << std::endl;
        system("pause");
        return EXIT_FAILURE;
    }
    system("pause");
    return EXIT_SUCCESS;
}