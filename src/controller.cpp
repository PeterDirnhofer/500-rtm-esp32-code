#include <stdint.h>

#include "driver/timer.h"

#include "globalVariables.h"
#include "controller.h"
#include "adc.h"
#include "spi.h"
#include "dataStoring.h"
#include "timer.h"
#include "uartLocal.h"
#include "esp_log.h"

/**
 * @brief Hilfe bei Inbetriebnahme. Monitoring Tunnelstrom, Keine Regelung
 * Zyklische Anzeige:
 * Tunnelstrom als ADC Wert dezimal und hexadezimal
 * Berechneter Tunnelstrom in nA
 * 
 */

static const char *TAG = "controller";
extern "C" void displayTunnelCurrent()
{

    esp_err_t errTemp = i2cInit(); // Init ADC
    if (errTemp != 0)
    {
        ESP_LOGE(TAG,"ERROR. Cannot init I2C. Returncode != 0. Returncode is : %d\n", errTemp);
    }
    //timer_pause(TIMER_GROUP_0, TIMER_0); // pause timer during dataset sending
    static double e, w, r = 0;


    ESP_LOGI(TAG,"+++ Display Tunnel Current\n");
    w = destinationTunnelCurrentnA;
    
    
    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;

    while (1)
    {
        uint16_t adcValue = readAdc(); // read current voltageoutput of preamplifier

        currentTunnelCurrentnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) / (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM); // max value 20.48 with preAmpResistor = 100MOhm and 2048mV max voltage
        r = currentTunnelCurrentnA;                                                                           // conversion from voltage to equivalent current
        // w=r+0.001;
        //  regeldifferenz = soll - ist
        e = w - r; // regeldifferenz = fuehrungsgroesse - rueckfuehrgroesse


      
        
        char  help[256];
        sprintf(help,"%d[digits]  0x%X[hex] %f[nA]      delta: %f  soll: %f\n", adcValue,adcValue,currentTunnelCurrentnA,e, w);

        logMonitor(help);
        
        //printf("remainingTunnelCurrentDifferencenA %f\n", remainingTunnelCurrentDifferencenA);
        vTaskDelay(xDelay);
    }
}


/**@brief Initialisiering vspi und i2c. Start controllerLoop. Initialisierung 1 ms Timer für restart controllerLoop
 * @details
 * Initialisiering vspi zur X,Y,Z DAC Piezo Ansteuerung.
 * Initialisierung i2c zur ADC Tunnelstrom Abfrage.
 * Start  Controllerloop.
 * Initialisierung 1 ms Timer tG0 zum retriggern der ControllerLoop.
 */
extern "C" void controllerStart()
{
    ESP_LOGI(TAG,"+++ controllerStart\n");

    esp_err_t errTemp = i2cInit(); // Init ADC
    if (errTemp != 0)
    {
        ESP_LOGE(TAG,"ERROR. Cannot init I2C. Returncode != 0. Returncode is : %d\n", errTemp);
    }
    
   

    vspiStart(); // Init and loop for DACs

    // xTaskCreatePinnedToCore(sendDatasets, "sendDatasets", 10000, NULL, 3, &handleSendDatasets, 0);
    // timer_tg0_initialise(1000000); //ns ->
    xTaskCreatePinnedToCore(controllerLoop, "controllerLoop", 10000, NULL, 2, &handleControllerLoop, 1);
    timer_tg0_initialise(1200); // us -> 10^6us/860SPS = 1162 -> 1200
}


/**
 * @brief Steuerung des RTM. Messen Tunnelstrom. Berechnen neue Piezoposition für X,Y und Z. Speichern Messdaten
 *
 * @details
 * Messung Tunnelstrom. Berechnung neuer Abstand Z zur Probe
 *
 * Die while Schleife setzt sich gleich am Anfang in den sleep modus.(mit vTaskSuspend)
 * Die schleife wird danach geweckt durch den ms Timer oder von der hspiLoop
 *
 * *
 * @Ablauf:
 * - Timer starts controllerloop cyclic
 *  - ADC Current is in limit
 *    - controllerloop saves valid values
 *    - rtmGrid.moveOn to set next XY Position currentX and currentY
 *    - sleep
 * - Current Out off limit
 *    - controllerloop calculates new Z value currentZDac
 *    - resume vspiLoop
 *    - sleep
 *    
 * - wait for next timer

 * Lesen Tunnelstrom 'adcValue' und Berechnung der Regeldifferenz
 *
 * Regeldifferenz liegt im Limit:
 * Gültiger Messwert. 'CurrentX', 'CurrentY' und 'CurrentZac' werden in der queue gespeichert.
 * Sind 100 Messwerte in der queue, wird die hspiLoop gestartet umd die Daten zu senden. 
 * Neue XY Position berechnen i
 * Die Loop selbst geht in sleep.
 *
 * Regeldifferenz out off limit:
 * Z Wert nachjustieren. 
 * Neuen globalen Stellwert currentZDac berechnen. vpsiLoop Starten. Suspend selbst. 
 */
extern "C" void controllerLoop(void *unused)
{
    ESP_LOGI(TAG,"+++ controllerLoopStart\n");
    static double e, w, r, y, eOld, yOld = 0;
    uint16_t ySaturate = 0;
    w = destinationTunnelCurrentnA;
    uint16_t unsentDatasets = 0;
  
    //
    while (1)
    {
        vTaskSuspend(NULL);  // Wecken durch timer

        uint16_t adcValue = readAdc(); // read current voltageoutput of preamplifier

        currentTunnelCurrentnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) / (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM); // max value 20.48 with preAmpResistor = 100MOhm and 2048mV max voltage
        r = currentTunnelCurrentnA;                                                                           // conversion from voltage to equivalent current
        // w=r+0.001;
        //  regeldifferenz = soll - ist
        e = w - r; // regeldifferenz = fuehrungsgroesse - rueckfuehrgroesse

        if (std::abs(e) <= remainingTunnelCurrentDifferencenA)
        {
            ESP_LOGI(TAG,"regeldifferenz: kleiner remainingTunnelCurrentDifferencenA\n");
            // save to queue
            dataQueue.emplace(dataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac)); // add dateElememt to queue
            unsentDatasets++;

            // Paket mit 100 Messwerten vollständig                                                                        // increment
            if (unsentDatasets >= sendDataAfterXDatasets)
            {
                // printf("enough datasets to send \n");
                timer_pause(TIMER_GROUP_0, TIMER_0); // pause timer during dataset sending
                unsentDatasets = 0;
                rtmDataReady = true; // damit weiss hspiLoop, dass Daten verfügbar sind

                //vTaskResume(handleHspiLoop); // sends datasets to raspberry pi, will resume after task for sending suspends itself
                ESP_LOGI(TAG,"ERSATZ HspiLoop\n");
                while(1)
                ;
                
                vTaskSuspend(NULL);
                unsentDatasets = 0;
                timer_start(TIMER_GROUP_0, TIMER_0); // resume timer
            }

            {
                // printf("Not enough datasets to send %d\n", unsentDatasets);
            }

            // Berechne nächste X,Y Piezo Position. move tip
            if (rtmGrid.moveOn()) // true, wenn Scan fertig ist moveTip
            {
                // Scan ist abgeschlossen
                timer_pause(TIMER_GROUP_0, TIMER_0); // pause timer, will be resumed at next new scan
                // all points scanned, end controllerLoop
                if (!dataQueue.empty())
                {
                    ESP_LOGI(TAG,"Last datasets to send \n");
                    //vTaskResume(handleHspiLoop); // sends datasets to raspberry pi, will resume after task for sending suspends itself
                    ESP_LOGI(TAG,"ERSATZ HspiLoop\n");
                    while(1)
                        ;
                }
                else
                {
                    ESP_LOGI(TAG,"All points scanned, already sent all datasets.\n");
                }
                vTaskDelete(NULL);
            }
            else
            {
                // noch nicht alle Positionen angefahren
                
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! PeDi
                // HAb ich eingeführt
                vTaskResume(handleVspiLoop); // will suspend itself 

            }
        }
        // regeldifferenz ist zu gross
        // Z Wert nachjustieren
        else
        {

            y = kP * e + kI * eOld + yOld; // stellgroesse = kP*regeldifferenz + kI* regeldifferenz_alt + stellgroesse_alt

            eOld = e;
            ySaturate = saturate16bit((uint32_t)y, 0, DAC_VALUE_MAX); // set to boundaries of DAC
            currentZDac = ySaturate;                                  // set new z height
            ESP_LOGI(TAG,"controller: new ZDac: %i. resume vspiLoop\n",currentZDac);

            // handleVspiLoop stellt neue Z Position auf currentZDac
            vTaskResume(handleVspiLoop); // will suspend itself
        }

        yOld = ySaturate;
        // end while
    }
}

extern "C" uint16_t saturate16bit(uint32_t input, uint16_t min, uint16_t max)
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
