//
// Created by rdast on 21.08.2025.
//

#include "Application.h"


int main() {
    try {
        Application app;
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
