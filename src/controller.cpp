#include <stdint.h>

#include "driver/timer.h"

#include "globalVariables.h"
#include "controller.h"
#include "adc.h"
#include "spi.h"
#include "dataStoring.h"
#include "timer.h"

void controllerStart()
{

    
    esp_err_t errTemp = i2cInit(); // Init ADC
    if (errTemp != 0)
    {
        printf("ERROR. Cannot init I2C. Retruncode != 0. Retruncode is : %d\n", errTemp);
    }
    // printf("I2C Init returncode must be 0: %d\n", errTemp);
    vspiStart(); // Init and loop for DACs
    // xTaskCreatePinnedToCore(sendDatasets, "sendDatasets", 10000, NULL, 3, &handleSendDatasets, 0);
    // timer_tg0_initialise(1000000); //ns ->
    xTaskCreatePinnedToCore(controllerLoop, "controllerLoop", 10000, NULL, 2, &handleControllerLoop, 1);
    timer_tg0_initialise(1200); // us -> 10^6us/860SPS = 1162 -> 1200
}

void controllerLoop(void *unused)
{

    printf("*** controllerLoop started\n");
    static double e, w, r, y, eOld, yOld = 0;
    uint16_t ySaturate = 0;
    w = destinationTunnelCurrentnA;
    uint16_t unsentDatasets = 0;
    // TickType_t xLastWakeTime = xTaskGetTickCount(); //set current for vTaskDelayUntil

    // TickType_t xFrequency = 10 / portTICK_PERIOD_MS; //1ms
    // timer_start(TIMER_GROUP_0, TIMER_0);
    while (1)
    {
        // vTaskDelayUntil( &xLastWakeTime, xFrequency );
        vTaskSuspend(NULL);
        // printf("controllerLoop");
        uint16_t adcValue = readAdc(); // read current voltageoutput of preamplifier
        // printf("%d\n", adcValue);
        // vTaskDelay(10);
        currentTunnelCurrentnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) / (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM); // max value 20.48 with preAmpResistor = 100MOhm and 2048mV max voltage
        r = currentTunnelCurrentnA;                                                                           // conversion from voltage to equivalent current

        e = w - r; // regeldifferenz = fuehrungsgroesse - rueckfuehrgroesse
        // printf("e: %f, remaining: %f \n", e, remainingTunnelCurrentDifferencenA);
        if (std::abs(e) <= remainingTunnelCurrentDifferencenA)
        {
            // save to queue
            dataQueue.emplace(dataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac)); // add dateElememt to queue
            unsentDatasets++;                                                                          // increment
            if (unsentDatasets >= sendDataAfterXDatasets)
            {
                // printf("enough datasets to send \n");
                timer_pause(TIMER_GROUP_0, TIMER_0); // pause timer during dataset sending
                unsentDatasets = 0;
                dataReady = true;

                vTaskResume(handleHspiLoop); // sends datasets to raspberry pi, will resume after task for sending suspends itself
                vTaskSuspend(NULL);
                unsentDatasets = 0;
                timer_start(TIMER_GROUP_0, TIMER_0); // resume timer
            }
            else
            {
                // printf("Not enough datasets to send %d\n", unsentDatasets);
            }

            // move tip
            if (rtmGrid.moveOn()) // moveTip
            {
                timer_pause(TIMER_GROUP_0, TIMER_0); // pause timer, will be resumed at next new scan
                // all points scanned, end controllerLoop
                if (!dataQueue.empty())
                {
                    printf("Last datasets to send \n");
                    vTaskResume(handleHspiLoop); // sends datasets to raspberry pi, will resume after task for sending suspends itself
                }
                else
                {
                    printf("All points scanned, already sent all datasets.\n");
                }
                vTaskDelete(NULL);
            }
        }
        else
        {
            y = kP * e + kI * eOld + yOld; // stellgroesse = kP*regeldifferenz + kI* regeldifferenz_alt + stellgroesse_alt

            eOld = e;
            ySaturate = saturate16bit((uint32_t)y, 0, DAC_VALUE_MAX); // set to boundaries of DAC
            currentZDac = ySaturate;                                  // set new z height
            vTaskResume(handleVspiLoop);                              // will suspend itself
        }
        yOld = ySaturate;
    }
}

uint16_t saturate16bit(uint32_t input, uint16_t min, uint16_t max)
{
    if (input < min)
    {
        return min;
    }
    if (input > max)
    {
        return max;
    }
    return (uint16_t)input;
}
