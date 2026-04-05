#include "ST1VAFE.h"
#include "stm32f4xx_hal_gpio.h"
int16_t hr;
extern SPI_HandleTypeDef hspi1;
#define SPI_READ_BIT          0x80
#define FIFO_DATA_OUT_TAG     0x78
synchronized_data_t synchronized_data;
float x;
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

    // 启用vAFE，，输入阻抗默认2.4GΩ
    ST1VAFE6AX_WriteReg(0x16, 0x8C);  // CTRL7 (16h): AH_BIO_EN=1, AH_BIO1_EN=1, AH_BIO2_EN=1, INT2_DRDY_AH_BIO=0
    //配置加速度计
    ST1VAFE6AX_WriteReg(0x18,0x80);//配置加速度计低通滤波,使能VAFE低通滤波
    ST1VAFE6AX_WriteReg(0x17,0x10);//配置加速度计量程+-2g，使能VAFE高通滤波
    ST1VAFE6AX_WriteReg(0x10, 0x06);  //开启加速计，配置加速度计ODR为120HZ设置为高性能模式
    // 2. 配置陀螺仪（可选，若不使用可保持掉电）
    // 设置陀螺仪ODR = 240Hz，高性能模式，满量程±2000dps
    ST1VAFE6AX_WriteReg(0x15, 0x62);  // CTRL6 (15h): FS_G = ±500dps ，选择低通滤波
    ST1VAFE6AX_WriteReg(0x11, 0x06);  //开启陀螺仪，配置陀螺仪ODR为120HZ设置为高性能模式
    //配置FIFO
    ST1VAFE6AX_WriteReg(0x0A, 0x00);//关闭FIFO
    ST1VAFE6AX_WriteReg(0x09, 0x66);//配置FIF0,将陀螺仪和加速度计数据速率设为120Hz
    ST1VAFE6AX_WriteReg(0x0B, 0x04);//Enables analog hub / vAFE batching in FIFO
    // FIFO阈值设置为90字
    ST1VAFE6AX_WriteReg(0x07, 0x5A); 
    ST1VAFE6AX_WriteReg(0x08, 0x00);
    // 开启FIFO设置为连续模式
    ST1VAFE6AX_WriteReg(0x0D, 0x08);//配置中断INT1
    ST1VAFE6AX_WriteReg(0x0A, 0x06);

} 
int16_t ST1VAFE6AX_ReadHeartRate(void) {
    uint8_t low  = ST1VAFE6AX_ReadReg(0x3A);  // AH_BIO_OUT_L
    uint8_t high = ST1VAFE6AX_ReadReg(0x3B);  // AH_BIO_OUT_H
    return (int16_t)((high << 8) | low);
}
void SPI_Read_Burst(uint8_t reg, uint8_t *pData, uint16_t size) {
    uint8_t addr = reg | SPI_READ_BIT; // 设置最高位为 1，表示读取
    
    HAL_GPIO_WritePin(ST1VAFE6_SPI_CS_GPIO_Port, ST1VAFE6_SPI_CS_Pin, GPIO_PIN_RESET);
    
    HAL_SPI_Transmit(&hspi1, &addr, 1, 100);             // 发送起始寄存器地址
    HAL_SPI_Receive(&hspi1, pData, size, 100);          // 连续接收 size 个字节
    
    HAL_GPIO_WritePin(ST1VAFE6_SPI_CS_GPIO_Port, ST1VAFE6_SPI_CS_Pin, GPIO_PIN_SET);   // 拉高 CS 结束通讯
}
void ST1VAFE6AX_Read_FIFO_SPI(void) {
    uint8_t status[2];
    // 1. 读取 FIFO 状态寄存器 (1Bh) 获取字数
    SPI_Read_Burst(0x1B, status, 2);
    uint16_t num_words = status[0] | ((status[1] & 0x03) << 8);

    if (num_words == 0) return;

    // 2. 循环读取每个 Word (每个 Word 7 字节)
    for (uint16_t i = 0; i < num_words; i++) {
        uint8_t raw_data[7];
        // 从 0x78 开始读取 7 字节 (TAG + 6字节数据)
        SPI_Read_Burst(FIFO_DATA_OUT_TAG, raw_data, 7);

        uint8_t tag = raw_data[0] >> 3; // 获取 TAG
        
        // 3. 数据解析逻辑
        int16_t val_x = (int16_t)((raw_data[2] << 8) | raw_data[1]);
        int16_t val_y = (int16_t)((raw_data[4] << 8) | raw_data[3]);
        int16_t val_z = (int16_t)((raw_data[6] << 8) | raw_data[5]);

        switch (tag) {
            case 0x02: // Accelerometer
                // 处理加速度数据...
                synchronized_data.accel.x = val_x;
                synchronized_data.accel.y = val_y; 
                synchronized_data.accel.z = val_z;
                x=(float)val_z*0.061/1000;
                break;
            case 0x01: // Gyroscope
                 synchronized_data.gyro.x = val_x;
                synchronized_data.gyro.y = val_y;
                synchronized_data.gyro.z = val_z;
                // 处理陀螺仪数据...
                break;
            case 0x1F: // vAFE (Heart Rate / Biopotential)
                // val_x 此时即为 bio_potential 原始电位值
                // 启动你的运动伪影滤除算法
                synchronized_data.bio_val = val_x;
                break;
        }
    }
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == ST1VAFE6_INT1_Pin) {
        // 可选择性检查状态寄存器，但直接读取更简单
       ST1VAFE6AX_Read_FIFO_SPI(); // 读取 FIFO 数据并处理
        // 将心率值放入全局队列或设置标志供主循环处理
    }
}
