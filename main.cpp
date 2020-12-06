#include <iostream>
#include <string>
#include <vector>
#include <getopt.h>
#include "afcapi.h"

void usage() {
    std::cout << "Usage: watchinspector -d <extract destination>" << std::endl;
    std::cout << "You would see this message even -h flag provided, or unknown argument has been passed." << std::endl;
}

int main(int argc, char *argv[]) {
    std::cout << "watchinspector by icslab\n" << std::endl;

    extern char *optarg;
    char *dest = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "hu:d:")) != -1) {
        switch (opt) {
            case 'd':
                dest = optarg + 1; // skipping first space(0x20) character
                break;
            case 'h':
            default:
            usage();
            break;
        }
    }

    if (dest == NULL)
        std::cout << "[!] directory argument was not provided, will work on walk-only mode." << std::endl;

    afcapi api = afcapi();
    if (api.is_initialized()) {
        char* new_dest;
        if (dest != NULL) {
            auto udid = api.get_current_udid();
            std::string dest_device = std::string(dest) + "/" + udid;
            new_dest = new char[dest_device.length() + 1];
            memcpy(new_dest, dest_device.c_str(), dest_device.length() + 1);
        }

        std::cout << "[i] Starting walking Apple Watch's directory..." << std::endl;
        auto count = api.walk_directory("", dest != NULL ? new_dest : dest); // putting "" actually search from root('/') directory.
        std::cout << "[i] Directory walk has been ended. " << count << " files and folders found." << std::endl;
    }
    else std::cout << "ERROR: AFCAPI initialization failed." << std::endl;

    return 0;
}