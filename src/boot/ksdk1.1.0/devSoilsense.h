

#ifndef WARP_BUILD_ENABLE_DEVSoilsense
#define WARP_BUILD_ENABLE_DEVSoilsense
#endif


i2c_status_t readRegisterSoil(i2c_device_t slave, uint8_t reg_start, uint8_t reg_end, uint8_t * i2c_buffer, uint16_t menuI2cPullupValue);

