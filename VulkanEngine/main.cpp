#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "core/VulkanApplication.h"

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