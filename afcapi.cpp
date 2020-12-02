#include <iostream>
#include <string>
#include <vector>
#include "afcapi.h"

// returns lockdown client
lockdownd_client_t detect_and_pair_device(char **udid, idevice_t *device, std::string APP_NAME) {
    lockdownd_client_t client = NULL;

    if (idevice_new_with_options(device, NULL, IDEVICE_LOOKUP_USBMUX) != IDEVICE_E_SUCCESS) {
        printf("ERROR: No device found!\n");
        return NULL;
    }

    if (idevice_get_udid(*device, udid) != IDEVICE_E_SUCCESS) {
        printf("ERROR: Unable to get the device UDID.\n");
        idevice_free(*device);
        return NULL;
    }

    printf("[i] Connected with UDID: %s\n", *udid);

    auto lerr = lockdownd_client_new(*device, &client, APP_NAME.c_str());

    if (lerr != LOCKDOWN_E_SUCCESS) {
        std::cout << "ERROR: Unable to pair with the device." << std::endl;
        idevice_free(*device);
        free(udid);
        return NULL; 
    }

    char* type = NULL;
    lerr = lockdownd_query_type(client, &type);

    if (lerr != LOCKDOWN_E_SUCCESS) {
        std::cout << "ERROR: querytype failed, ecode: " << lockdownd_strerror(lerr) << std::endl;
        lockdownd_client_free(client);
        idevice_free(*device);
        free(udid);
        return NULL;
    }
    else {
        if (strcmp("com.apple.mobile.lockdown", type) != 0)
            std::cout << "ERROR: Wrong querytype responsed, " << type << std::endl;
        else std::cout << "[i] plist query success: " << type << std::endl;
        free(type); 
    }

    lerr = lockdownd_pair(client, NULL);

    if (lerr == LOCKDOWN_E_PASSWORD_PROTECTED) {
        std::cout << "[i] Device is in password-protected status. Please unlock the device." << std::endl;
        std::cout << "    After unlock your Apple Watch, press Enter key to continue." << std::endl;
        std::cin.get();

        lerr = lockdownd_pair(client, NULL);
    }
    else if (lerr == LOCKDOWN_E_PAIRING_DIALOG_RESPONSE_PENDING) {
        std::cout << "[i] Device is wait for dialog about trust your PC. Please unlock and tap 'Trust this device'." << std::endl;
        std::cout << "    After trust this PC, press Enter key to continue." << std::endl;
        std::cin.get();

        lerr = lockdownd_pair(client, NULL);
    }
    else if (lerr != LOCKDOWN_E_SUCCESS) {
        std::cout << "ERROR: Pairing failed, ecode: " << lockdownd_strerror(lerr) << std::endl;
        lockdownd_client_free(client);
        return NULL;
    }
    if (lerr == LOCKDOWN_E_SUCCESS)
        std::cout << "[i] Successfully paired: " << *udid << std::endl;
    else std::cout << "ERROR: FATAL Pairing error, ecode: " << lockdownd_strerror(lerr) << std::endl;

    lockdownd_client_free(client);
    auto lderr = lockdownd_client_new_with_handshake(*device, &client, APP_NAME.c_str());
    if (lderr != LOCKDOWN_E_SUCCESS) {
        std::cout << "ERROR: Validation error, ecode: " << lockdownd_strerror(lderr) << std::endl;
        return NULL;
    }
    return client;
}

afc_client_t start_afc_service(idevice_t *device, lockdownd_client_t *client) {
    afc_client_t afc = NULL;
    lockdownd_service_descriptor_t service = NULL;

    auto ldret = lockdownd_start_service(*client, "com.apple.afc", &service);
    if ((ldret == LOCKDOWN_E_SUCCESS) && service->port) {
        afc_client_new(*device, service, &afc);
    }
    else {
        std::cout << "ERROR: error while starting afc service, errcode: " << ldret << std::endl;
        return NULL;
    }

    std::cout << "[i] service '" << service->identifier << "' has been started at port " << service->port << std::endl;

	if (service) {
		lockdownd_service_descriptor_free(service);
		service = NULL;
	}

    return afc;
}

std::vector<std::string> get_file_info(afc_client_t *afc, std::string path) {
    char **file_info = NULL;
    
    auto aerr = afc_get_file_info(*afc, path.c_str(), &file_info);
    if (aerr != AFC_E_SUCCESS) {
        std::cout << "ERROR: Cannot read file '" << path << "', errcode: " << aerr << std::endl;
        return std::vector<std::string>();
    }

    std::vector<std::string> ret;

    for (int i = 0; file_info[i] != NULL; i++)
        ret.push_back(file_info[i]);
    
    free(file_info);
    return ret;
}

std::vector<std::string> read_directory(afc_client_t *afc, std::string directory) {
    char **list = NULL;

    auto aerr = afc_read_directory(*afc, directory.c_str(), &list);
    if (aerr != AFC_E_SUCCESS) {
        std::cout << "ERROR: Cannot read directory '" << directory << "', errcode: " << aerr << std::endl;
        return std::vector<std::string>();
    }

    std::vector<std::string> ret;

    for (int i = 0; list[i] != NULL; i++)
        ret.push_back(list[i]);
    
    free(list);
    return ret;
}

void walk_directory(afc_client_t *afc, std::string root) {
    auto entries = read_directory(afc, root);

    for (auto entry : entries) {
        if (entry == "." || entry == "..")
            continue;

        auto val = get_file_info(afc, root + "/" + entry);
        if (val[7] == "S_IFDIR") {
            std::cout << "  [Found] directory '" << root + "/" + entry + "'" << std::endl; 
            walk_directory(afc, root + "/" + entry);
        }
        else std::cout << "  [Found] file '" << root + "/" + entry + "'" << std::endl;
    }
}
