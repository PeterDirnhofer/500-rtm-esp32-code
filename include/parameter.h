#ifndef PARAMETER
#define PARAMETER


//esp_err_t nvsGet(int16_t &value, const char* key);
//esp_err_t nvsSet(int16_t &value, const char* key);
//esp_err_t nvsSetString(const char * value,const char * key);
//esp_err_t nvsGetString(const char * value,const char * key);
esp_err_t print_what_saved(void);
esp_err_t save_restart_counter(void);
esp_err_t saveParameter(int32_t myValue);
esp_err_t saveStringParameter(const char* label, const char* value);
esp_err_t readStringParameter(const char* label, char* ptr);
#endif