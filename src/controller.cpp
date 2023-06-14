// controller.cpp
#include "controller.h"
#include "project_timer.h"
using namespace std;
static const char *TAG = "controller";

/**@brief Initialialze I2C for ADC and spi for DACs.
 * Initialize ADC.
 * Initialize DAC by Starting vspiDacLoop once
 */
extern "C" esp_err_t initHardware()
{

    esp_err_t errTemp = i2cAdcInit(); // Init I2C for ADC
    if (errTemp != 0)
    {
        ESP_LOGE(TAG, "ERROR. Cannot init I2C. Returncode != 0. Returncode is : %d\n", errTemp);
        esp_restart();
    }

    // Init DACs
    vspiDacStart(); // Init and loop for DACs
    return ESP_OK;
}

extern "C" void adjustStart()
{

    xTaskCreatePinnedToCore(adjustLoop, "adjustLoop", 10000, NULL, 2, &handleAdjustLoop, 1);
    timer_initialize(MODE_ADJUST_TEST_TIP);
}

/// @brief read ADC, convert to Volt and send via USB. Triggered by gptimer tick
extern "C" void adjustLoop(void *unused)
{

    static double e, w, r = 0;
    w = destinationTunnelCurrentnA;
    while (true)
    {
        vTaskSuspend(NULL);            // Sleep. Will be retriggered by gptimer
        uint16_t adcValue = readAdc(); // read current voltageoutput of preamplifier
        currentTunnelCurrentnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) / (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);

        r = currentTunnelCurrentnA; // conversion from voltage to equivalent current

        // abweichung     = soll (10.0)      - ist
        // regeldifferenz = fuehrungsgroesse - rueckfuehrgroesse
        e = w - r;
        // UsbPcInterface::send("abwweichung: %f soll: %f ist:%f\n", e,w,r);

        // Abweichung soll/ist im limit --> Messwerte speichern und nächste XY Position anfordern
        // Set LED
        if (abs(e) <= remainingTunnelCurrentDifferencenA)
        {
            gpio_set_level(IO_02, 1);
        }
        else
            gpio_set_level(IO_02, 0);

        // max value 20.48 with preAmpResistor = 100MOhm and 2048mV max voltage
        double adcInVolt = (adcValue * ADC_VOLTAGE_MAX * 1e2) / (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);
        UsbPcInterface::send("ADJUST,%f,%d\n", adcInVolt, adcValue);
    }
}

/**Initialize task measurementLoop*/
extern "C" void measurementStart()
{

    xTaskCreatePinnedToCore(measurementLoop, "measurementLoop", 10000, NULL, 2, &handleControllerLoop, 1);
    timer_initialize(MODE_MEASURE);
}

/**@brief method runs one ADC Current Measure cycle and positions measure tip*/ 
extern "C" void measurementLoop(void *unused)
/* Steps:
 * - method is started cyclic by timer (timer_tg0_isr)
 * - Measuring ADC tunnel current
 * - If ADC current is within limit:
 *    - send ADC measue value with  X,Y  Position to PC
 *    - request next XY Position
 *    - go to sleep and wait for next timer tick
 * - If ADC current is out off limit
 *    - calculate better value for Z position 
 *    - request new calculated Z position by starting vspiDacLoop
 *    - go to sleep and Wait for next timer tick
 */

{
    ESP_LOGI(TAG, "+++ controllerLoopStart\n");
  
    static double e, w, r, y, eOld, yOld = 0;
    uint16_t ySaturate = 0;
    w = destinationTunnelCurrentnA;

    gpio_set_level(IO_17, 1);
    while (true)
    {

        vTaskSuspend(NULL); // Sleep. Will be restarted by timer

        uint16_t adcValue = readAdc(); // read current voltageoutput of preamplifier

        currentTunnelCurrentnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) / (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM); // max value 20.48 with preAmpResistor = 100MOhm and 2048mV max voltage
        r = currentTunnelCurrentnA;                                                                           // conversion from voltage to equivalent current

        // abweichung     = soll (10.0)      - ist
        // regeldifferenz = fuehrungsgroesse - rueckfuehrgroesse
        e = w - r;
        // UsbPcInterface::send("abwweichung: %f soll: %f ist:%f\n", e,w,r);

        // Abweichung soll/ist im limit --> Messwerte speichern und nächste XY Position anfordern
        if (abs(e) <= remainingTunnelCurrentDifferencenA)
        {

            // Set LED 
            gpio_set_level(IO_02, 1);

            // save to queue  Grid(x)  Grid(y)  Z_DAC
            dataQueue.emplace(DataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac));

            // Send to PC continuously
            m_sendDataPaket();

            // New XY calculation.
            if (!rtmGrid.moveOn())
            {
                // Last XY position not yet reached
                vTaskResume(handleVspiLoop); // process new XY position
            }
            else
            {
                // All X Y positions complete
                // timer_stop();
                m_sendDataPaket(1); // 1: Send rest of data and 'DONE'
                esp_restart();
            }
        }
        // Abweichung soll/ist zu gross --> Z muss nachgeregelt werden
        else
        {
            gpio_set_level(IO_02, 0);
            //               1000                10
            // stellgroesse = kP*regeldifferenz + kI* regeldifferenz_alt + stellgroesse_alt 
            y = kP * e + kI * eOld + yOld; 
            eOld = e;
            ySaturate = m_saturate16bit((uint32_t)y, 0, DAC_VALUE_MAX); // set to boundaries of DAC
            currentZDac = ySaturate;                                    // set new z height
            

            // handleVspiLoop stellt neue Z Position auf currentZDac
            vTaskResume(handleVspiLoop); // will suspend itself
        }

        yOld = ySaturate;
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

/**
 * method sends measuredata in dataQueueE to PC. Pause measurement if too many values in dataQueue 
*/
extern "C" int  m_sendDataPaket(bool terminate)
{
    
    // TODO timer_stop only if dataQueue overflow
    bool timer_was_stopped = false;

    size_t numElements = dataQueue.size();

    if (numElements > 100) {

        timer_was_stopped = true;
        ESP_LOGW(TAG,"Stop timer. Size of dataQueue > 100\n");
        timer_stop();
    }
    
    while (!dataQueue.empty())
    {
        dataQueue.front();
        uint16_t X = dataQueue.front().getDataX();
        uint16_t Y = dataQueue.front().getDataY();
        uint16_t Z = dataQueue.front().getDataZ();
  
        // TODO Check if send unit_16 instead of strings X Y Z
        UsbPcInterface::send("DATA,%d,%d,%d\n", X, Y, Z);

        dataQueue.pop(); // remove from queue
    }
    if (terminate == true)
    {
        UsbPcInterface::send("DATA,DONE\n");
    }

    // Restart timer to trigger measurement
    if (timer_was_stopped == true)
        timer_start();
    return 0;
}