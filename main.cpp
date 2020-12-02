#include <iostream>
#include <string>
#include <vector>
#include "afcapi.h"

const std::string APP_NAME = "watchinspector";

int main(int argc, char *argv[]) {
    std::cout << "watchinspector by icslab\n" << std::endl;

    static char *udid = NULL;
    idevice_t device = NULL;

    std::cout << "[i] Detecting connected Apple Watch..." << std::endl;
    auto client = detect_and_pair_device(&udid, &device, APP_NAME.c_str());
    if (client == NULL) {
        idevice_free(device);
        free(udid);
        return -1;
    }

    std::cout << "[i] Validated paring with device " << udid << std::endl;

    auto afc_client = start_afc_service(&device, &client);
    if (afc_client == NULL) {
        lockdownd_client_free(client);
        idevice_free(device);
        free(udid);
        return -1;
    }

    std::cout << "[i] Starting walking Apple Watch's directory..." << std::endl;
    walk_directory(&afc_client, "");
    std::cout << "[i] Directory walk has been ended." << std::endl;

    /* Cleanup */
    idevice_free(device);
    free(udid);

    return 0;
}