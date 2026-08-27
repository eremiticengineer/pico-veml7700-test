#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "VEML7700.h"

/*
 * Each task is assigned a priority from 0 to ( configMAX_PRIORITIES - 1 ),
 * where configMAX_PRIORITIES is defined within FreeRTOSConfig.h.
 * Low priority numbers denote low priority tasks. The idle task has priority zero (tskIDLE_PRIORITY).
 * The task placed into the Running state is always the highest priority task that is able to run.
 */
#define VEML7700_TASK_PRIORITY (tskIDLE_PRIORITY + 2UL)

SemaphoreHandle_t i2c_mutex;
float luxValue;

namespace veml770_config {
    inline constexpr i2c_inst_t* I2C_INSTANCE = i2c0;
    inline constexpr uint8_t ADDRESS = 0x10;
    inline constexpr uint SDA = 8;
    inline constexpr uint SCL = 9;
}

void veml7700_task(void *params) {
    VEML7700 sensor(veml770_config::I2C_INSTANCE,
                    veml770_config::ADDRESS,
                    veml770_config::SDA,
                    veml770_config::SCL);

    if (xSemaphoreTake(i2c_mutex, portMAX_DELAY)) {
      if (!sensor.begin()) {
          xSemaphoreGive(i2c_mutex);
          printf("VEML7700 init failed\n");
          vTaskDelete(NULL);
      }
      xSemaphoreGive(i2c_mutex);
    }

    while (true) {
        if (xSemaphoreTake(i2c_mutex, portMAX_DELAY)) {
          if (sensor.readLux(luxValue)) {
              printf("Lux: %.2f\n", luxValue);
          }
          else {
              printf("Failed to read lux\n");
          }
          xSemaphoreGive(i2c_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1s delay
    }
}

int main( void )
{
    stdio_init_all();

    i2c_init(veml770_config::I2C_INSTANCE, 100 * 1000);
    gpio_set_function(veml770_config::SDA, GPIO_FUNC_I2C);
    gpio_set_function(veml770_config::SCL, GPIO_FUNC_I2C);
    gpio_pull_up(veml770_config::SDA);
    gpio_pull_up(veml770_config::SCL);

    sleep_ms(2000);

    i2c_mutex = xSemaphoreCreateMutex();

    BaseType_t result = xTaskCreate(veml7700_task, "VEML7700Task", 1024, nullptr, VEML7700_TASK_PRIORITY, nullptr);

    printf("xTaskCreate: %ld\n", result);

    vTaskStartScheduler();

    return 0;
}
