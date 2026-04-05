#include "main.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal_gpio.h"
void LED_Task()
{
    while(1)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
        osDelay(1000); // 1秒钟切换一次LED状态
    }
}