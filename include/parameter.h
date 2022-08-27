#ifndef PARAMETER
#define PARAMETER


esp_err_t nvsGet(int16_t &value, const char* key);
esp_err_t nvsSet(int16_t &value, const char* key);
esp_err_t nvsSetString(const char * value,const char * key);
esp_err_t nvsGetString(const char * value,const char * key);
esp_err_t print_what_saved(void);
#endif