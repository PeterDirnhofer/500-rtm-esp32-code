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

int runtime_ms()
{

    int64_t rt = esp_timer_get_time() - controller_start_time;
    int rtInt = (int)(rt / 1000);
    return rtInt;
}

/**@brief Initialialze vspi and i2c. Start single controllerLoop. Initialize 1 ms Timer for restarting controllerLoop
 * @details
 * Init vspi for X,Y,Z DAC Piezo control.
 * Init i2c for ADC reading tunnel-crrent.
 * Start single run controllerLoop.
 * Init 1 ms Timer tG0 to trigger single controllerLoop run.
 */
extern "C" void controllerStart()
{
    controller_start_time = esp_timer_get_time();

    ESP_LOGI(TAG, "+++ controllerStart\n");
    printf("+++++++++++++++++++++ controllerStart\n");

    esp_err_t errTemp = i2cInit(); // Init ADC
    if (errTemp != 0)
    {
        ESP_LOGE(TAG, "ERROR. Cannot init I2C. Returncode != 0. Returncode is : %d\n", errTemp);
    }

    // Init DACs
    vspiDacStart();              // Init and loop for DACs
    vTaskResume(handleVspiLoop); // Start for one run. Will suspend itself

    xTaskCreatePinnedToCore(controllerLoop, "controllerLoop", 10000, NULL, 2, &handleControllerLoop, 1);
    printf("Timer Period set to 1200*1000\n");
    // timer_tg0_initialise(1200*5000); // us -> 10^6us/860SPS = 1162 -> 1200
    timer_tg0_initialise(1200 * 1);
    // controllerLoop(NULL);
}

/**@brief Run one Measure Cycle. Save data if valid. Control Position of microscopes measure tip.
 *
 *
 * @Steps:
 * - Controllerloop is started cyclic by timer (timer_tg0_isr)
 * - Measure ADC current
 * - If ADC current is in limit:
 *    - controllerloop saves current ADC value and X,Y,Z Position to queue.
 *    - If 100 values are in queue, pause loop and send data to PC
 *    - Request next XY Position
 *    - resume vspiDacLoop to move tip to new X Y position
 *    - stop ControllerLoop. Wait for next timer
 * - If ADC current is out off limit. Correction of Z distance
 *    - calculate new Z value and request new Z position
 *    - resume vspiDacLoop to move tip to new Z position
 *    - stop ControllerLoop. Wait for next timer
 */
extern "C" void controllerLoop(void *unused)
{
    ESP_LOGI(TAG, "+++ controllerLoopStart\n");
    printf("+++ controllerLoopStart\n");
    static double e, w, r, y, eOld, yOld = 0;
    uint16_t ySaturate = 0;
    w = destinationTunnelCurrentnA;
    uint16_t unsentDatasets = 0;

    // printf("nvs_maxX %d nvs_maxY%d\n", nvs_maxX, nvs_maxY);
    // rtmGrid.setMaxX(nvs_maxX);
    // rtmGrid.setMaxY(nvs_maxY);

    //
    while (true)
    {
        // printf("%i: 1000 tick \n",runtime_ms());
        // ESP_LOGW(TAG,"%i: 1000 tick \n",runtime_ms());
        vTaskSuspend(NULL); // Wecken durch timer

        // uint16_t adcValue = readAdc(); // read current voltageoutput of preamplifier
        uint16_t adcValue = 111; // read current voltageoutput of preamplifier

        currentTunnelCurrentnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) / (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM); // max value 20.48 with preAmpResistor = 100MOhm and 2048mV max voltage
        r = currentTunnelCurrentnA;                                                                           // conversion from voltage to equivalent current

        // abweichung     = soll             - ist
        // regeldifferenz = fuehrungsgroesse - rueckfuehrgroesse
        e = w - r;

        // Abweichung im Limit ?
        if (abs(e) <= remainingTunnelCurrentDifferencenA)
        {
            // ESP_LOGI(TAG,"OR TRUE regeldifferenz im limit. Messen.\n");

            // save to queue
            dataQueue.emplace(dataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac));
            // printf("unsent datasets %i\n",unsentDatasets);
            // printf("queue Size %i\n", dataQueue.size());

            // Paket mit 100 Messwerten vollständig -> Unterbrechen und senden                                                                        // increment
            if (++unsentDatasets >= sendDataAfterXDatasets)
            // if (++ unsentDatasets >=10)

            {
                unsentDatasets = sendPaketWithData();
            }

            // Berechne nächste X,Y Piezo Position. move tip
            if (!rtmGrid.moveOn())
            {
                // noch nicht alle Positionen angefahren
                // ESP_LOGW(TAG,"!rtmGrid.moveOn\n");
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! PeDi
                // HAb ich eingeführt
                // vTaskResume(handleVspiLoop); // will suspend itself
            }
            else
            {
                // Alle Positionen erledigt
                ESP_LOGW(TAG, "Alle Positionen erledigt\n");
                timer_pause(TIMER_GROUP_0, TIMER_0); // pause timer, will be resumed at next new scan
                // Send rest of data
                if (!dataQueue.empty())
                {
                    sendPaketWithData();
                }

                sendPaketWithData(1);
                esp_restart();
            }
        }
        // regeldifferenz ist zu gross
        // Z Wert nachjustieren
        else
        {
            printf("Off limits\n");
            y = kP * e + kI * eOld + yOld; // stellgroesse = kP*regeldifferenz + kI* regeldifferenz_alt + stellgroesse_alt

            eOld = e;
            ySaturate = saturate16bit((uint32_t)y, 0, DAC_VALUE_MAX); // set to boundaries of DAC
            currentZDac = ySaturate;                                  // set new z height
            printf("new ZDac: %i. resume vspiDacLoop\n", currentZDac);
            ESP_LOGI(TAG, "new ZDac: %i. resume vspiDacLoop\n", currentZDac);

            // handleVspiLoop stellt neue Z Position auf currentZDac
            vTaskResume(handleVspiLoop); // will suspend itself
        }

        yOld = ySaturate;
        // end while
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

extern "C" int sendPaketWithData(bool terminate)
{

    timer_pause(TIMER_GROUP_0, TIMER_0); // pause timer during dataset sending

    while (!dataQueue.empty())
    {
        dataQueue.front();
        uint16_t X = dataQueue.front().getDataX();
        uint16_t Y = dataQueue.front().getDataY();
        uint16_t Z = dataQueue.front().getDataZ();
        // if (X == 0)
        //     printf("DATA,%d,%d,%d\n", X, Y, Z);

        UsbPcInterface::send("DATA,%d,%d,%d\n", X, Y, Z);
        // printf("queue Size vor pop %i\n", dataQueue.size());
        dataQueue.pop(); // remove from queue
        // printf("queue Size nach pop %i\n", dataQueue.size());
    }
    if (terminate == true)
    {
        UsbPcInterface::send("DATA,DONE,");
    }

    timer_start(TIMER_GROUP_0, TIMER_0); // resume timer
    return 0;
}

/**@brief Loop diplaying tunnelcurrent to help adjusting measure tip
 *
 */
extern "C" void displayTunnelCurrent()
{
    ESP_LOGI(TAG, "+++ START Display Current Channel");

    esp_err_t errTemp = i2cInit(); // Init I2C for XYZ DACs
    if (errTemp != 0)
    {
        ESP_LOGE(TAG, "ERROR. Cannot init I2C. Returncode != 0. Returncode is : %d\n", errTemp);
    }

    // Set X,Y,Z to 0
    currentXDac = 0;
    currentYDac = 0;
    currentZDac = 0;
    vspiDacStart();              // Init and loop for DACs
    vTaskResume(handleVspiLoop); // Start for one run. Will suspend itself

    ESP_LOGI(TAG, "+++ Display Tunnel Current\n");
    ESP_LOGI(TAG, "Set X,Y,Z =");

    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
    int ledLevel = 1;

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
