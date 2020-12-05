#include <vector>
#include <string>
#include <libimobiledevice/afc.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/notification_proxy.h>
#include <plist/plist.h>

#ifndef AFCAPI_H
#define AFCAPI_H

#define BUF_SIZE 4096

class afcapi {
private:
    const char *APP_NAME = "afcapi";
    char *udid = NULL;
    idevice_t device = NULL;
    lockdownd_client_t client = NULL;
    afc_client_t afc = NULL;
    bool _connected = false;
    
    bool detect_and_pair_device();
    bool start_afc_service();
public:
    afcapi();
    ~afcapi();
    bool is_initialized();
    uint64_t read_file(std::string path, char **data);
    bool copy_file_to_disk(std::string path_on_device, std::string dest);
    std::vector<std::string> get_file_info(std::string path);
    std::vector<std::string> read_directory(std::string directory);
    int walk_directory(std::string root, char *dest);
};

#endif
