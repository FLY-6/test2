#include "ST1VAFE.h"
extern
uint8_t ST1VAFE6AX_CheckSPI(void) {
    uint8_t who_am_i = ST1VAFE6AX_ReadReg(0x0F);  // 读取 WHO_AM_I 寄存器
    return (who_am_i == 0x71) ? 1 : 0;
}

// 底层读函数（SPI 模式3）
uint8_t ST1VAFE6AX_ReadReg(uint8_t reg) {
    uint8_t tx_cmd = reg | 0x80;  // 读命令
    uint8_t rx_data = 0;

    HAL_GPIO_WritePin(ST1VAFE6_SPI_CS_GPIO_Port, ST1VAFE6_SPI_CS_Pin, GPIO_PIN_RESET); // CS 拉低
    HAL_SPI_Transmit(&hspi1, &tx_cmd, 1, HAL_MAX_DELAY); // 发送命令
    HAL_SPI_Receive(&hspi1, &rx_data, 1, HAL_MAX_DELAY); // 接收数据
    HAL_GPIO_WritePin(ST1VAFE6_SPI_CS_GPIO_Port, ST1VAFE6_SPI_CS_Pin, GPIO_PIN_SET);   // CS 拉高

    return rx_data;
}
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
    ST1VAFE6AX_WriteReg(0x10, 0x60);  // CTRL1 (10h): ODR_XL = 240Hz, 高性能模式
    HAL_Delay(10);

    // 启用vAFE，连接AH1/BIO1和AH2/BIO2引脚，输入阻抗默认2.4GΩ
    ST1VAFE6AX_WriteReg(0x16, 0x1C);  // CTRL7 (16h): AH_BIO_EN=1, AH_BIO1_EN=1, AH_BIO2_EN=1, INT2_DRDY_AH_BIO=0
    // 注：若需数据就绪中断可自行配置

    // 2. 配置陀螺仪（可选，若不使用可保持掉电）
    // 设置陀螺仪ODR = 240Hz，高性能模式，满量程±2000dps
    ST1VAFE6AX_WriteReg(0x11, 0x60);  // CTRL2 (11h): ODR_G = 240Hz, 高性能模式
    ST1VAFE6AX_WriteReg(0x15, 0x38);  // CTRL6 (15h): FS_G = ±2000dps (对应 0100)

    // 3. 配置vAFE数字滤波器（可选，默认无滤波）
    // 若需50/60Hz陷波，设置AH_BIO_LPF=1，此时ODR自动变为120Hz
    // ST1VAFE6AX_WriteReg(0x18, 0x80);  // CTRL9 (18h) AH_BIO_LPF=1
}    
