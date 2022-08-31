#ifndef PARAMETER
#define PARAMETER






esp_err_t print_what_saved(void);
esp_err_t save_restart_counter(void);
esp_err_t saveParameter(int32_t myValue);
esp_err_t saveStringParameter(const char *myLabel, const char *myValue);
esp_err_t readStringParameter(const char* myLabel, char* ptr);
void setupDefaultData();
#endif