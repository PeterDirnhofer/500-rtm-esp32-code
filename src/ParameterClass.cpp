// parameter.cpp


#include "ParameterClass.h"
#include "NvsStorageClass.h"
#include "ParameterClass.h"
#include <esp_log.h>


ParameterClass::ParameterClass()
{
}


ParameterClass::~ParameterClass()
{
}

static const char *TAG = "ParameterClass";

esp_err_t ParameterClass::setParameter(std::vector<std::string> params){
    // 100 1000 10.0 0.01 0 0 0 199 199``
    ESP_LOGI(TAG,"params.size %d\n",(int)params.size()); 
    if((int)params.size()!=10){
        ESP_LOGE(TAG,"setparameter needs 9+1 values. Actual %d\n",(int)params.size());
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;

}

std::vector<std::string> ParameterClass::getParameter(){
    std::vector<std::string> returnVector;
    returnVector.push_back("return1");
    returnVector.push_back("return2");
    return returnVector;

}

/*
void setupDefaultData()
{
 
    size_t required_size = 0;

    

    kI = 50;
    kP = 100;
    destinationTunnelCurrentnA = 20.0;
    remainingTunnelCurrentDifferencenA = 0.1;
    rtmGrid.setDirection(false);
    rtmGrid.setMaxX(199);
    rtmGrid.setMaxY(199);

    printf("%f \n", kI);
    printf("%f \n", kP);
    printf("%f \n", destinationTunnelCurrentnA);
    printf("%f \n", remainingTunnelCurrentDifferencenA);
    printf("%d \n", rtmGrid.getCurrentX());
    printf("%d \n", rtmGrid.getCurrentY());
    printf("%d \n", direction);
    printf("%d \n", rtmGrid.getMaxX());
    printf("%d \n", rtmGrid.getMaxY());
  
}
*/