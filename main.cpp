#include <iostream>
#include <string>
#include <vector>
#include "afcapi.h"

int main(int argc, char *argv[]) {
    std::cout << "watchinspector by icslab\n" << std::endl;
    afcapi api = afcapi();

    if (api.isInitialized()) {
        std::cout << "[i] Starting walking Apple Watch's directory..." << std::endl;
        auto count = api.walk_directory("", NULL); // putting "" actually search from root('/') directory.
        std::cout << "[i] Directory walk has been ended. " << count << " files and folders found." << std::endl;
    }
    else std::cout << "ERROR: AFCAPI initialization failed." << std::endl;

    return 0;
}