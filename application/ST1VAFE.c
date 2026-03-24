#include "ST1VAFE.h"
#include "stm32f4xx_hal_gpio.h"
int16_t hr;
extern SPI_HandleTypeDef hspi1;
uint8_t ST1VAFE6AX_ReadReg(uint8_t reg) {
    uint8_t tx_cmd = reg | 0x80;  // 读命令
    uint8_t rx_data = 0;

    HAL_GPIO_WritePin(ST1VAFE6_SPI_CS_GPIO_Port, ST1VAFE6_SPI_CS_Pin, GPIO_PIN_RESET); // CS 拉低
    HAL_SPI_Transmit(&hspi1, &tx_cmd, 1, HAL_MAX_DELAY); // 发送命令
    HAL_SPI_Receive(&hspi1, &rx_data, 1, HAL_MAX_DELAY); // 接收数据
    HAL_GPIO_WritePin(ST1VAFE6_SPI_CS_GPIO_Port, ST1VAFE6_SPI_CS_Pin, GPIO_PIN_SET);
    return rx_data;
}
uint8_t ST1VAFE6AX_CheckSPI(void) {
    uint8_t who_am_i = ST1VAFE6AX_ReadReg(0x0F);  // 读取 WHO_AM_I 寄存器
    return (who_am_i == 0x71) ? 1 : 0;
}

// 底层读函数（SPI 模式3）

void ST1VAFE6AX_WriteReg(uint8_t reg, uint8_t data) {
    uint8_t tx[2] = { reg & 0x7F, data };  // 写命令：寄存器地址最高位为0
    HAL_GPIO_WritePin(ST1VAFE6_SPI_CS_GPIO_Port, ST1VAFE6_SPI_CS_Pin, GPIO_PIN_RESET); // CS 拉低
    HAL_SPI_Transmit(&hspi1, tx, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(ST1VAFE6_SPI_CS_GPIO_Port, ST1VAFE6_SPI_CS_Pin, GPIO_PIN_SET);   // CS 拉高
}

/* 从指定寄存器读取一个字节 */
uint8_t ST1VAFE6AX_ReadData(uint8_t reg) {
    uint8_t tx = reg | 0x80;  // 读命令：寄存器地址最高位为1
    uint8_t rx = 0;
    HAL_GPIO_WritePin(ST1VAFE6_SPI_CS_GPIO_Port, ST1VAFE6_SPI_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &tx, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, &rx, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(ST1VAFE6_SPI_CS_GPIO_Port, ST1VAFE6_SPI_CS_Pin, GPIO_PIN_SET);   // CS 拉高
    return rx;
}
void ST1VAFE6AX_Init(void) {
    // 复位设备（可选）
    ST1VAFE6AX_WriteReg(0x12, 0x01);  // CTRL3 (12h) SW_RESET = 1
    HAL_Delay(10);

    // 1. 启用vAFE通道（必须先配置加速计为高性能模式）
    // 加速计ODR = 240Hz，高性能模式（默认）
    ST1VAFE6AX_WriteReg(0x10, 0x00);  //关闭加速计，配置ODR和性能模式
     ST1VAFE6AX_WriteReg(0x11, 0x00);  //关闭陀螺仪，配置ODR和性能模式
    HAL_Delay(10);

    // 启用vAFE，连接AH1/BIO1和AH2/BIO2引脚，输入阻抗默认2.4GΩ
    ST1VAFE6AX_WriteReg(0x16, 0xCC);  // CTRL7 (16h): AH_BIO_EN=1, AH_BIO1_EN=1, AH_BIO2_EN=1, INT2_DRDY_AH_BIO=1
    //配置加速度计
    ST1VAFE6AX_WriteReg(0x18,0x88);//配置加速度计低通滤波,使能VAFE低通滤波
    ST1VAFE6AX_WriteReg(0x17,0x90);//配置加速度计量程+-2g，使能VAFE高通滤波
    ST1VAFE6AX_WriteReg(0x10, 0x08);//配置加速度计ODR = 480Hz，高性能模式，满量程±2g (对应 0000)
    // 2. 配置陀螺仪（可选，若不使用可保持掉电）
    // 设置陀螺仪ODR = 240Hz，高性能模式，满量程±2000dps
    ST1VAFE6AX_WriteReg(0x15, 0x62);  // CTRL6 (15h): FS_G = ±500dps ，选择低通滤波
    ST1VAFE6AX_WriteReg(0x11, 0x08);  // CTRL2 (11h): ODR_G = 480Hz, 高性能模式
} 
int16_t ST1VAFE6AX_ReadHeartRate(void) {
    uint8_t low  = ST1VAFE6AX_ReadReg(0x3A);  // AH_BIO_OUT_L
    uint8_t high = ST1VAFE6AX_ReadReg(0x3B);  // AH_BIO_OUT_H
    return (int16_t)((high << 8) | low);
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == ST1VAFE6_INT2_Pin) {
        // 可选择性检查状态寄存器，但直接读取更简单
        hr = ST1VAFE6AX_ReadHeartRate();
        // 将心率值放入全局队列或设置标志供主循环处理
    }
}
