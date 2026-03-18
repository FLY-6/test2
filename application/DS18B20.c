#include "DS18B20.h"
#define DS18B20_OUTPUT()   { GPIO_InitTypeDef gpio = {0}; \
                              gpio.Pin = DQ_Pin; \
                              gpio.Mode = GPIO_MODE_OUTPUT_OD; \
                              gpio.Pull = GPIO_PULLUP; \
                              gpio.Speed = GPIO_SPEED_FREQ_HIGH; \
                              HAL_GPIO_Init(DQ_GPIO_Port, &gpio); }

// 设置为输入模式
#define DS18B20_INPUT()    { GPIO_InitTypeDef gpio = {0}; \
                              gpio.Pin = DQ_Pin; \
                              gpio.Mode = GPIO_MODE_INPUT; \
                              gpio.Pull = GPIO_PULLUP; \
                              HAL_GPIO_Init(DQ_GPIO_Port, &gpio); }

// 总线操作宏
#define DS18B20_LOW()       HAL_GPIO_WritePin(DQ_GPIO_Port, DQ_Pin, GPIO_PIN_RESET)
#define DS18B20_HIGH()      HAL_GPIO_WritePin(DQ_GPIO_Port, DQ_Pin, GPIO_PIN_SET)
#define DS18B20_READ()      HAL_GPIO_ReadPin(DQ_GPIO_Port, DQ_Pin)
void delay_us(uint32_t us) {
    uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000);
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < ticks);
}
uint8_t DS18B20_Reset(void) {
    uint8_t presence = 0;
    
    DS18B20_OUTPUT();
    DS18B20_LOW();
    delay_us(480);      // 复位脉冲 ≥480μs
    DS18B20_HIGH();
    delay_us(60);       // 等待设备响应
    
    DS18B20_INPUT();
    if (DS18B20_READ() == 0) presence = 1;  // 存在脉冲拉低总线
    delay_us(420);      // 等待剩余复位时间
    return presence;
}
void DS18B20_WriteBit(uint8_t bit) {
    DS18B20_OUTPUT();
    DS18B20_LOW();
    delay_us(2);        // 开始写时隙
    if (bit) {
        DS18B20_HIGH(); // 写1：15μs内释放总线
    }
    delay_us(60);       // 保持时隙≥60μs
    DS18B20_HIGH();     // 恢复总线
    delay_us(2);        // 恢复时间
}
uint8_t DS18B20_ReadBit(void) {
    uint8_t bit;
    
    DS18B20_OUTPUT();
    DS18B20_LOW();
    delay_us(2);        // 开始读时隙
    DS18B20_INPUT();    // 释放总线，设备驱动
    delay_us(10);       // 等待数据有效（15μs内）
    bit = DS18B20_READ();
    delay_us(50);       // 保持时隙≥60μs
    return bit;
}
void DS18B20_WriteByte(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        DS18B20_WriteBit(data & 0x01);
        data >>= 1;
    }
}
uint8_t DS18B20_ReadByte(void) {
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++) {
        data >>= 1;                         // 低位先接收，需要先移位再存
        if (DS18B20_ReadBit()) {
            data |= 0x80;                    // 放在最高位，最后右移后成为正确位序
        }
    }
    // 由于循环中先右移，最后数据的高位是最后接收的位，因此需调整
    // 简便方法：按传统方法接收
    // 改为：data |= (DS18B20_ReadBit() << i); 然后data最后不变
    // 我们重新实现一个清晰的版本：
    data = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (DS18B20_ReadBit()) {
            data |= (1 << i);
        }
    }
    return data;
}
void DS18B20_StartConvert(void) {
    if (DS18B20_Reset()) {
        DS18B20_WriteByte(0xCC);   // Skip ROM
        DS18B20_WriteByte(0x44);   // Convert T
    }
}
int16_t DS18B20_ReadTemperature(void) {
    int16_t tempRaw = 0;
    uint8_t tempL, tempH;
    
    if (DS18B20_Reset()) {
        DS18B20_WriteByte(0xCC);   // Skip ROM
        DS18B20_WriteByte(0xBE);   // Read Scratchpad
        
        tempL = DS18B20_ReadByte();
        tempH = DS18B20_ReadByte();
        tempRaw = (int16_t)((tempH << 8) | tempL);
    }
    return tempRaw;  // 返回原始值，需要转换为实际温度
}
float DS18B20_RawToCelsius(int16_t raw) {
    return raw * 0.0625f;
}