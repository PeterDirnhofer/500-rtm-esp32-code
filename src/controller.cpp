#include <stdint.h>

#include "driver/timer.h"

#include "globalVariables.h"
#include "controller.h"
#include "adc.h"
#include "spi.h"
#include "dataStoring.h"
#include "timer.h"


/**
 * @brief Initialisiering vspi und i2c. Start Controllerloop. Initialisierung 1 ms Timer tG0
 * Initialisiering vspi zur X,Y,Z DAC Piezo Ansteuerung.
 * Initialisierung i2c zur ADC Tunnelstrom Abfrage.
 * Start  Controllerloop.
 * Initialisierung 1 ms Timer tG0 zum retriggern der ControllerLoop.
 */


/// @brief Initialisiering vspi und i2c. Start Controllerloop. Initialisierung 1 ms Timer\n 
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

/* *********************************************************************************************************
 * controllerLoop
 *
 * Steuerung des RTM. Berechnen der nächsten Piezo Position. Messung Tunnelstrom. Berechnung neuer Abstand Z zur Probe
 * Die while Schleife unterbricht sich selbst (mit vTaskSuspend) und wird vom 1 ms Timer oder die hspiLoop retriggert
 *  
 * Ablauf:
 * Lesen Tunnelstrom 'adcValue'. Berechnung der Regeldifferenz 
 * 
 * Falls die Regeldifferenz nicht zu gross ist, werden die Messwerte CurrentX, CurrentY und CurrentZac in der Queue gespeichert  
 * Wurden genügend Messwerte in der queue gespeichert, wird der 1 ms Timer gestoppt und die Daten in der HspiLoop an den Raspberry gesendet  
 * Gibt es noch nicht genügend Messwerte, wird die nächste Piezo Position im grid berechnet und der Timer gestoppt. 
 * Die neue Piezo Position wird dann in der vpsiLoop angefahren
 * 
 * War die Regeldifferenz zu gross (der Tunnelstron weicht zu sehr ab) wird eine korrigierte Z position 'currentZDac'
 * berechnet und die vpsLoop gestartet um diesen neuen Z Wert einzustellen.
 */

void controllerLoop(void *unused)
{

    static double e, w, r, y, eOld, yOld = 0;
    uint16_t ySaturate = 0;
    w = destinationTunnelCurrentnA;
    uint16_t unsentDatasets = 0;
    
    while (1)
    {
        vTaskSuspend(NULL);

        uint16_t adcValue = readAdc(); // read current voltageoutput of preamplifier

        currentTunnelCurrentnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) / (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM); // max value 20.48 with preAmpResistor = 100MOhm and 2048mV max voltage
        r = currentTunnelCurrentnA;                                                                           // conversion from voltage to equivalent current

        e = w - r; // regeldifferenz = fuehrungsgroesse - rueckfuehrgroesse

        if (std::abs(e) <= remainingTunnelCurrentDifferencenA)
        {
            printf("regeldifferenz: kleiner remainingTunnelCurrentDifferencenA\n");
            // save to queue
            dataQueue.emplace(dataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac)); // add dateElememt to queue
            unsentDatasets++;  
            
            // Genügend Messwerte aufgenommen                                                                        // increment
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

            // Berechne nächste X,Y Piezo Position. move tip
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
            printf("+++ resume vspiLoop (controller)\n");
            vTaskResume(handleVspiLoop); // will suspend itself
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
