// controller.cpp
#include "controller.h"
using namespace std;
static const char *TAG = "controller";



/**@brief Initialialze vspi and i2c. Start single controllerLoop. Initialize 1 ms Timer for restarting controllerLoop
 * @details
 * Init vspi for X,Y,Z DAC Piezo control.
 * Init i2c for ADC reading tunnel-crrent.
 * Start single run controllerLoop.
 * Init 1 ms Timer tG0 to trigger single controllerLoop run.
 */
extern "C" void measurementStart()
{
   

    esp_err_t errTemp = i2cAdcInit(); // Init I2C for ADC
    if (errTemp != 0)
    {
        ESP_LOGE(TAG, "ERROR. Cannot init I2C. Returncode != 0. Returncode is : %d\n", errTemp);
    }

    // Init DACs
    vspiDacStart();              // Init and loop for DACs
    vTaskResume(handleVspiLoop); // Start for one run. Will suspend itself

    xTaskCreatePinnedToCore(measurementLoop, "controllerLoop", 10000, NULL, 2, &handleControllerLoop, 1);
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
extern "C" void measurementLoop(void *unused)
{
    ESP_LOGI(TAG, "+++ controllerLoopStart\n");
    printf("+++ controllerLoopStart\n");
    static double e, w, r, y, eOld, yOld = 0;
    uint16_t ySaturate = 0;
    w = destinationTunnelCurrentnA;
    uint16_t unsentDatasets = 0;

    while (true)
    {

        vTaskSuspend(NULL); // Sleep. Will be restarted by timer

        uint16_t adcValue = readAdc(); // read current voltageoutput of preamplifier

        currentTunnelCurrentnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) / (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM); // max value 20.48 with preAmpResistor = 100MOhm and 2048mV max voltage
        r = currentTunnelCurrentnA;                                                                           // conversion from voltage to equivalent current

        // abweichung     = soll (10.0)      - ist
        // regeldifferenz = fuehrungsgroesse - rueckfuehrgroesse
        e = w - r;

        // Abweichung soll/ist im limit --> Messwerte speichern und nächste XY Position anfordern
        if (abs(e) <= remainingTunnelCurrentDifferencenA)
        {
            // save to queue  Grid(x)  Grid(y)  Z_DAC
            dataQueue.emplace(dataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac));

            // Paket with 100 results complete -> Interrupt mesuring and send data vis USB
            if (++unsentDatasets >= sendDataAfterXDatasets)
            {
                unsentDatasets = sendPaketWithData();
            }

            // New XY calculation. Move tip XY
            if (!rtmGrid.moveOn())
            {
                // Last XY position not yet reached
                // PEDI: hab ich eingeführt
                vTaskResume(handleVspiLoop); // will suspend itself
            }
            else
            {
                // All X Y positions complete
                ESP_LOGW(TAG, "All X Y positions complete\n");
                timer_pause(TIMER_GROUP_0, TIMER_0); // pause timer, will be resumed at next new scan
                sendPaketWithData(1);                // 1: Send rest of data and 'DONE'
                esp_restart();
            }
        }
        // Abweichung soll/ist zu gross --> Z muss nachgeregelt werden
        else
        {
            //               1000                10
            y = kP * e + kI * eOld + yOld; // stellgroesse = kP*regeldifferenz + kI* regeldifferenz_alt + stellgroesse_alt
            eOld = e;
            ySaturate = m_saturate16bit((uint32_t)y, 0, DAC_VALUE_MAX); // set to boundaries of DAC
            currentZDac = ySaturate;                                  // set new z height
            printf("new ZDac: %i. resume vspiDacLoop\n", currentZDac);
            ESP_LOGI(TAG, "new ZDac: %i. resume vspiDacLoop\n", currentZDac);

            // handleVspiLoop stellt neue Z Position auf currentZDac
            vTaskResume(handleVspiLoop); // will suspend itself
        }

        yOld = ySaturate;
    }
}

/**@brief Loop diplaying tunnelcurrent to help adjusting measure tip
 *
 */
extern "C" void displayTunnelCurrentLoop(UsbPcInterface usb)
{

    esp_err_t errTemp = i2cAdcInit(); // Init I2C for ADC
    if (errTemp != 0)
    {
        ESP_LOGE(TAG, "ERROR. Cannot init I2 for ADC. Returncode != 0. Returncode is : %d\n", errTemp);
    }


    vspiDacStart(); // Init and loop for DACs
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "+++ Display Tunnel Current\n");
    const TickType_t xDelay = 1000 / portTICK_PERIOD_MS;
    int ledLevel = 1;

    while (1)
    {
        //usb.getCommandsFromPC();
        //ESP_LOGI(TAG, "getWorkingMode %d", usb.getWorkingMode());
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

uint16_t m_saturate16bit(uint32_t input, uint16_t min, uint16_t max)
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