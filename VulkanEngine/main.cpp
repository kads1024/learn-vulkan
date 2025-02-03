#include "VulkanApplication.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() 
{
    VulkanApplication app{};

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << __FILE__ << ": " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}