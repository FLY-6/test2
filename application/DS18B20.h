#include "main.h"
void DS18B20_StartConvert(void);
int16_t DS18B20_ReadTemperature(void);
float DS18B20_RawToCelsius(int16_t raw);