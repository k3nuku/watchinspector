#include <vector>
#include <string>
#include <libimobiledevice/afc.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/notification_proxy.h>
#include <plist/plist.h>

lockdownd_client_t detect_and_pair_device(char **udid, idevice_t *device, std::string APP_NAME);
afc_client_t start_afc_service(idevice_t *device, lockdownd_client_t *client);
std::vector<std::string> get_file_info(afc_client_t *afc, std::string path);
std::vector<std::string> read_directory(afc_client_t *afc, std::string directory);
void walk_directory(afc_client_t *afc, std::string root);