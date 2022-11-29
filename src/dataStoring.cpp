#include "dataStoring.h"
// #include "communication.h"
#include "globalVariables.h"

// constructor of dataElement for queue
dataElement::dataElement(uint16_t x, uint16_t y, uint16_t z)
    : x(x), y(y), z(z)
{
}

void saveConfigParamWoCommunication_h()
{
}

/*
void saveConfigParam(double param, uint8_t configDescriptionByte){

    switch (configDescriptionByte){
        case(SPECIFIER_CONFIG_KI):
            kI = param;
            break;
        case(SPECIFIER_CONFIG_KP):
            kP = param;
            break;
        case(SPECIFIER_CONFIG_DESTINATION_CURRENT):
            destinationTunnelCurrentnA = param;
            break;
        case(SPECIFIER_CONFIG_REMAINING_CURRENT):
            remainingTunnelCurrentDifferencenA = param;
            break;
        case(SPECIFIER_CONFIG_START_X):
            //startX = (uint16_t) param;
            rtmGrid.setStartX((uint16_t) param);
            break;
        case(SPECIFIER_CONFIG_START_Y):
            rtmGrid.setStartY((uint16_t) param);
            break;
        case(SPECIFIER_CONFIG_DIRECTION):
            direction = (bool) param;
            break;
        case(SPECIFIER_CONFIG_MAX_X):
            rtmGrid.setMaxX((uint16_t) param);
            break;
        case(SPECIFIER_CONFIG_MAX_Y):
            rtmGrid.setMaxY((uint16_t) param);
            break;
        case(SPECIFIER_CONFIG_MULTIPLICATOR_ADC):
            rtmGrid.setMultiplicatorGridAdc((uint16_t) param);
            break;

    }


}

*/
void sendDatasets(void *unused)
{
    // unnecessary task...
    printf("Created task for sending datasets to Pi\n");
    while (1)
    {
        // vTaskResume(handleHspiLoop);
        printf("ERSATZ HspiLoop\n");
        while (1)
            ;

        vTaskSuspend(NULL); // suspend sendDatasets task, so controller will resume
    }
}
