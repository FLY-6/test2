#include "main.h"
void ST1VAFE6AX_Init(void);
uint8_t ST1VAFE6AX_CheckSPI(void);
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} imu_data_t;

typedef struct {
    int16_t bio_val;    // vAFE 心率原始电位值
    imu_data_t accel;   // 对应的加速度
    imu_data_t gyro;    // 对应的陀螺仪
} synchronized_data_t;