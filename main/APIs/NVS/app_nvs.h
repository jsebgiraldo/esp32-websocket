/*
 * app_nvs.h
 *
*  Created on: Apr 04, 2022
 *      Author: Juan Sebastian Giraldo Duque
 */

#ifndef MAIN_APP_NVS_H_
#define MAIN_APP_NVS_H_

void app_nvs_flash_setup(void);

/**
 * Saves station mode Wifi credentials to NVS
 * @return ESP_OK if successful.
 */
esp_err_t app_nvs_save_sta_creds(void);

/**
 * Loads the previously saved credentials from NVS.
 * @return true if previously saved credentials were found.
 */
bool app_nvs_load_sta_creds(void);

/**
 * Clears station mode credentials from NVS
 * @return ESP_OK if successful.
 */
esp_err_t app_nvs_clear_sta_creds(void);

#endif /* MAIN_APP_NVS_H_ */
