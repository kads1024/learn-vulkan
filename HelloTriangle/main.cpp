#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>

const uint16_t WINDOW_WIDTH = 800;
const uint16_t WINDOW_HEIGHT = 600;

const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif // NDEBUG

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() {
        return graphicsFamily.has_value();
    }
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
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;

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
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        vkDestroyDevice(m_device, nullptr);

        if (ENABLE_VALIDATION_LAYERS)
            DestroyDebugUtilsMessengerEXT(m_vulkanInstance, m_debugMessenger, nullptr);

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

        VkPhysicalDeviceFeatures physicalDeviceFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        return physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            queueFamilyIndices.isComplete();
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
            queueFamilyIndex++;

            if (queueFamilyIndices.isComplete()) {
                break;
            }
        }

        return queueFamilyIndices;
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);
        float queuePriority = 1.0f;

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures physicalDeviceFeatures{};

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

        if (ENABLE_VALIDATION_LAYERS) {
            deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
        }

        VkResult deviceCreateResult = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);

        if (deviceCreateResult != VK_SUCCESS)
            throw std::runtime_error("Failed to Create Logical Device!");

        vkGetDeviceQueue(m_device, queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    system("pause");
    return EXIT_SUCCESS;
}