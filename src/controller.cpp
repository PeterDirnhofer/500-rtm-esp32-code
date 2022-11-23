#include <stdint.h>

#include "driver/timer.h"

#include "globalVariables.h"
#include "controller.h"
#include "adc.h"
#include "spi.h"
#include "dataStoring.h"
#include "timer.h"
#include "esp_log.h"
#include <stdarg.h>
#include "UsbPcInterface.h"

using namespace std;

static const char *TAG = "controller";

/**@brief Initialialze vspi and i2c. Start single controllerLoop. Initialize 1 ms Timer for restarting controllerLoop
 * @details
 * Init vspi for X,Y,Z DAC Piezo control.
 * Init i2c for ADC reading tunnel-crrent.
 * Start single run controllerLoop.
 * Init 1 ms Timer tG0 to trigger single controllerLoop run.
 */
extern "C" void controllerStart()
{
    ESP_LOGI(TAG,"+++ controllerStart\n");

    esp_err_t errTemp = i2cInit(); // Init ADC
    if (errTemp != 0)
    {
        ESP_LOGE(TAG,"ERROR. Cannot init I2C. Returncode != 0. Returncode is : %d\n", errTemp);
    }
    
    vspiDacStart(); // Init and loop for DACs
    xTaskCreatePinnedToCore(controllerLoop, "controllerLoop", 10000, NULL, 2, &handleControllerLoop, 1);
    timer_tg0_initialise(1200); // us -> 10^6us/860SPS = 1162 -> 1200
}

int sendPaketWithData()
{

    timer_pause(TIMER_GROUP_0, TIMER_0); // pause timer during dataset sending

    rtmDataReady = true; // damit weiss hspiLoop, dass Daten verfügbar sind

    //vTaskResume(handleHspiLoop); // sends datasets to raspberry pi, will resume after task for sending suspends itself
    UsbPcInterface::sendData();
    vTaskSuspend(NULL);
    timer_start(TIMER_GROUP_0, TIMER_0); // resume timer
    return 0;
}


/**@brief 
 * Steuerung des RTM. Messen Tunnelstrom. Berechnen neue Piezoposition für X,Y und Z. Speichern Messdaten
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
 *    - resume vspiDacLoop
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
    
        // abweichung     = soll             - ist
        // regeldifferenz = fuehrungsgroesse - rueckfuehrgroesse
        e = w - r; 

        // Abweichung im Limit ?
        if (abs(e) <= remainingTunnelCurrentDifferencenA)
        {
            ESP_LOGI(TAG,"regeldifferenz im limit. Messen.\n");
            // save to queue
            dataQueue.emplace(dataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac)); 

            // Paket mit 100 Messwerten vollständig -> Unterbrechen und senden                                                                        // increment
            if (++ unsentDatasets >= sendDataAfterXDatasets)
            {
                unsentDatasets= sendPaketWithData();
            }

            // Berechne nächste X,Y Piezo Position. move tip
            if (!rtmGrid.moveOn()) 
            {
                // noch nicht alle Positionen angefahren
                
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! PeDi
                // HAb ich eingeführt
                vTaskResume(handleVspiLoop); // will suspend itself 

            }
            else
            {
                // Alle Positionen erledigt
                timer_pause(TIMER_GROUP_0, TIMER_0); // pause timer, will be resumed at next new scan
                // Send rest of data
                if (!dataQueue.empty())
                {
                    sendPaketWithData();
                }
                vTaskDelete(NULL);
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
            ESP_LOGI(TAG,"new ZDac: %i. resume vspiDacLoop\n",currentZDac);

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
/**@brief Loop diplaying tunnelcurrent to help adjusting measure tip 
 * 
 */
extern "C" void displayTunnelCurrent()
{
    ESP_LOGI(TAG,"+++ START Display Current Channel");
    
    esp_err_t errTemp = i2cInit(); // Init I2C for XYZ DACs
    if (errTemp != 0)
    {
        ESP_LOGE(TAG,"ERROR. Cannot init I2C. Returncode != 0. Returncode is : %d\n", errTemp);
    }

    // Set X,Y,Z to 0
    currentXDac=0;
    currentYDac=0;
    currentZDac=0;  
    vspiDacStart(); // Init and loop for DACs  
    vTaskResume(handleVspiLoop); // Start for one run. Will suspend itself 
    
    ESP_LOGI(TAG,"+++ Display Tunnel Current\n");
    ESP_LOGI(TAG,"Set X,Y,Z =");
  
    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
    int ledLevel=1;
    
    while (1)
    {
        
        uint16_t adcValue = readAdc(); // read current voltageoutput of preamplifier
        currentTunnelCurrentnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) / (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM); 
        // max value 20.48 with preAmpResistor = 100MOhm and 2048mV max voltage
        UsbPcInterface::send("ADJUST,%f\n", currentTunnelCurrentnA);
      
        // Invert Blue LED        
        gpio_set_level(BLUE_LED, ledLevel % 2);
        ledLevel++;
        
        vTaskDelay(xDelay);

    }
}

