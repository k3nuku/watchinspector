#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "afcapi.h"

afcapi::afcapi() {
    // stage 1: detect with usbmuxd and pair
    std::cout << "[i] Detecting connected Apple Watch..." << std::endl;
    auto res = detect_and_pair_device();
    
    if (!res) {
        idevice_free(device);
        free(udid);
        return;
    }
    else std::cout << "[i] Validated pairing with device " << udid << std::endl;

    // stage 2: start afc service
    res = start_afc_service();
    
    if (!res) {
        lockdownd_client_free(client);
        idevice_free(device);
        free(udid);
        return;
    }
    else std::cout << "[i] afc service has been successfully started." << std::endl;

    this->_connected = true;
}

afcapi::~afcapi(void) {
    idevice_free(this->device);
    free(this->udid);
}

// Returns: true - afc client has been initialized 
bool afcapi::is_initialized() {
    return this->_connected ? true : false;
}

// returns lockdown client
bool afcapi::detect_and_pair_device() {
    if (idevice_new_with_options(&(this->device), NULL, IDEVICE_LOOKUP_USBMUX) != IDEVICE_E_SUCCESS) {
        printf("ERROR: No device found!\n");
        return false;
    }

    if (idevice_get_udid(this->device, &(this->udid)) != IDEVICE_E_SUCCESS) {
        printf("ERROR: Unable to get the device UDID.\n");
        idevice_free(this->device);
        return false;
    }

    printf("[i] Connected with UDID: %s\n", udid);

    auto lerr = lockdownd_client_new(this->device, &(this->client), this->APP_NAME);

    if (lerr != LOCKDOWN_E_SUCCESS) {
        std::cout << "ERROR: Unable to pair with the device." << std::endl;
        idevice_free(this->device);
        free(this->udid);
        return false; 
    }

    char* type = NULL;
    lerr = lockdownd_query_type(this->client, &type);

    if (lerr != LOCKDOWN_E_SUCCESS) {
        std::cout << "ERROR: querytype failed, ecode: " << lockdownd_strerror(lerr) << std::endl;
        lockdownd_client_free(client);
        idevice_free(this->device);
        free(this->udid);
        return false;
    }
    else {
        if (strcmp("com.apple.mobile.lockdown", type) != 0)
            std::cout << "ERROR: Wrong querytype responsed, " << type << std::endl;
        else std::cout << "[i] plist query success: " << type << std::endl;
        free(type); 
    }

    lerr = lockdownd_pair(this->client, NULL);

    if (lerr == LOCKDOWN_E_PASSWORD_PROTECTED) {
        std::cout << "[i] Device is in password-protected status. Please unlock the device." << std::endl;
        std::cout << "    After unlock your Apple Watch, press Enter key to continue." << std::endl;
        std::cin.get();

        lerr = lockdownd_pair(this->client, NULL);
    }
    else if (lerr == LOCKDOWN_E_PAIRING_DIALOG_RESPONSE_PENDING) {
        std::cout << "[i] Device is wait for dialog about trust your PC. Please unlock and tap 'Trust this device'." << std::endl;
        std::cout << "    After trust this PC, press Enter key to continue." << std::endl;
        std::cin.get();

        lerr = lockdownd_pair(this->client, NULL);
    }
    else if (lerr != LOCKDOWN_E_SUCCESS) {
        std::cout << "ERROR: Pairing failed, ecode: " << lockdownd_strerror(lerr) << std::endl;
        lockdownd_client_free(this->client);
        return false;
    }
    if (lerr == LOCKDOWN_E_SUCCESS)
        std::cout << "[i] Successfully paired: " << this->udid << std::endl;
    else {
        std::cout << "ERROR: FATAL Pairing error, ecode: " << lockdownd_strerror(lerr) << std::endl;
        return false;
    }

    lockdownd_client_free(client);
    auto lderr = lockdownd_client_new_with_handshake(this->device, &client, this->APP_NAME);
    if (lderr != LOCKDOWN_E_SUCCESS) {
        std::cout << "ERROR: Validation error, ecode: " << lockdownd_strerror(lderr) << std::endl;
        return false;
    }
    return true;
}

bool afcapi::start_afc_service() {
    lockdownd_service_descriptor_t service = NULL;

    afc_error_t aerr;
    auto ldret = lockdownd_start_service(this->client, "com.apple.afc", &service);
    if ((ldret == LOCKDOWN_E_SUCCESS) && service->port) {
        aerr = afc_client_new(this->device, service, &afc);
    }
    else {
        std::cout << "ERROR: error while starting afc service, errcode: " << ldret << std::endl;
        return false;
    }

    if (!service) {
        std::cout << "ERROR: Failed to start afc client, errcode: " << aerr << std::endl;
        return false;
    }

    std::cout << "[i] service '" << service->identifier << "' has been started at port " << service->port << std::endl;
    lockdownd_service_descriptor_free(service);
    service = NULL;

    return true;
}

// Returns file info
std::vector<std::string> afcapi::get_file_info(std::string path) {
    char **file_info = NULL;
    
    auto aerr = afc_get_file_info(this->afc, path.c_str(), &file_info);
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

// arguments: path = path of file, char **data = output
// Returns amount of read bytes
// **you need to FREE 'data' variable manually after use it
uint32_t afcapi::read_file(std::string path, char **data) {
    auto fileinfo = this->get_file_info(path);
    uint32_t fsize;

    if (fileinfo.size() == 0)
        return -1;

	for (int i = 0; i < fileinfo.size(); i += 2) {
		if (strcmp(fileinfo[i].c_str(), "st_size") == 0) {
			fsize = atol(fileinfo[i + 1].c_str());
			break;
		}
	}

	if (fsize == 0)
		return -1;

	uint64_t f = 0;
	afc_file_open(afc, path.c_str(), AFC_FOPEN_RDONLY, &f);
	if (!f)
		return -1;

	char *buf = (char *)malloc((uint32_t)fsize);

	uint32_t done = 0;
	while (done < fsize) {
		uint32_t bread = 0;
		afc_file_read(afc, f, buf + done, 65536, &bread);
		if (bread > 0)
			done += bread;
		else break;
	}
	
    if (done == fsize)
		*data = buf;
	else free(buf);

	afc_file_close(afc, f);
    return fsize;
}

bool afcapi::copy_file_to_disk(std::string path_on_device, std::string dest) {
    std::ofstream out_stream;
    out_stream.open(dest, std::ios::binary | std::ios::out);

    char** data;
    uint32_t fsize = read_file(path_on_device, data);

    while (fsize > 0) {
        if (fsize > BUF_SIZE)
            out_stream.write(reinterpret_cast<char*>(data), (uint32_t)BUF_SIZE);
        else out_stream.write(reinterpret_cast<char*>(data), fsize);
        fsize -= BUF_SIZE;
    }

    return false;
}

// Returns directory structure
std::vector<std::string> afcapi::read_directory(std::string directory) {
    char **list = NULL;

    auto aerr = afc_read_directory(this->afc, directory.c_str(), &list);
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

// walk whole directory
// arguments: std::string root, char* extract destination
// returns count of entities (total extracted)
int afcapi::walk_directory(std::string root, char *dest) {
    int total_entities = 0;
    auto entries = read_directory(root);
    char *sbuf = NULL;

    if (dest != NULL && mkdir(dest, 0777) == -1) {
        std::cout << "ERROR: failed to create directory " << dest << std::endl;
        return 0;
    }
    else if (dest != NULL)
        std::cout << "    [i] Directory created." << std::endl;

    for (auto entry : entries) {
        if (entry == "." || entry == "..")
            continue;
        
        std::string next_dir = root + "/" + entry;
        auto val = get_file_info(next_dir);

        if (dest != NULL) {
            sbuf = new char[strlen(dest) + next_dir.length() + 1];
            strcpy(sbuf, dest);
            strcat(sbuf, next_dir.c_str());
        }

        if (val[7] == "S_IFDIR") {
            std::cout << "  [Found] directory '" << next_dir << "'" << std::endl;
            total_entities += walk_directory(next_dir, sbuf);
        }
        else {
            std::cout << "  [Found] file '" << next_dir + "'" << std::endl;
            
            if (dest != NULL) {
                std::cout << "    [i] Copying file to PC... ";
                std::string cppstr = std::string(sbuf);
                if (!copy_file_to_disk(next_dir, cppstr))
                    std::cout << "FAILED" << std::endl;
                else std::cout << "OK" << std::endl;
            }
        }

        if (dest != NULL) {
            delete[] sbuf; // free
            sbuf = NULL;
        }
    }

    total_entities += entries.size();
    return total_entities;
}
