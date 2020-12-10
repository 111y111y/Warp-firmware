#include <stdlib.h>

#include "fsl_misc_utilities.h"
#include "fsl_device_registers.h"
#include "fsl_i2c_master_driver.h"
#include "fsl_spi_master_driver.h"
#include "fsl_rtc_driver.h"
#include "fsl_clock_manager.h"
#include "fsl_power_manager.h"
#include "fsl_mcglite_hal.h"
#include "fsl_port_hal.h"

#include "gpio_pins.h"
#include "SEGGER_RTT.h"
#include "warp.h"
#include "devSoilsense.h"



extern volatile uint32_t		gWarpI2cBaudRateKbps;
extern volatile uint32_t		gWarpI2cTimeoutMilliseconds;
extern volatile uint32_t		gWarpSupplySettlingDelayMilliseconds;
extern volatile uint32_t		gWarpMenuPrintDelayMilliseconds;




uint32_t readMoisture(void)
{

	uint8_t			i2c_buffer[2];
	i2c_status_t	status;
	i2c_device_t	slave = {
				.address = 0x36,
				.baudRate_kbps = gWarpI2cBaudRateKbps
				};
	uint8_t 	payload[2] = {0x0F,0x10};
	uint16_t	moisture = 0;
	enableI2Cpins(32768);
		
	OSA_TimeDelay(200);
	status = I2C_DRV_MasterSendDataBlocking(0,
							&slave,
							NULL,
							0,
							payload,
							2,
							gWarpI2cTimeoutMilliseconds);

	if (status != kStatus_I2C_Success){
		SEGGER_RTT_WriteString(0, "Failed to write command :( \n");
		OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds);
	} else{
		//SEGGER_RTT_WriteString(0, "\nCommand given");
		OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds); 
	}

    OSA_TimeDelay(200);
	//Read from the calibration register
		status = I2C_DRV_MasterReceiveDataBlocking(0,
								&slave,
								NULL,
								0,
								i2c_buffer,
								2,
								gWarpI2cTimeoutMilliseconds);

		OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds);

		if (status != kStatus_I2C_Success){
			SEGGER_RTT_WriteString(0, "Failed to read  :( \n");
			OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds);
		} else {
			OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds);
			moisture = i2c_buffer[0] << 8;
			moisture |= i2c_buffer[1];
			SEGGER_RTT_printf(0, "\nMoisture reading > %d ", moisture);

		}
	disableI2Cpins();
	
	return moisture;
}

int readTemp(void)
{

	uint8_t			i2c_buffer[4];
	i2c_status_t	status;
	i2c_device_t	slave = {
				.address = 0x36,
				.baudRate_kbps = gWarpI2cBaudRateKbps
				};
	uint8_t 	payload[2] = {0x00,0x04};
	int			temperature = 0;
	int			celsius = 0.0;
	double		test = 1.0;
	enableI2Cpins(32768);
		
	OSA_TimeDelay(200);
	status = I2C_DRV_MasterSendDataBlocking(0,
							&slave,
							NULL,
							0,
							payload,
							2,
							gWarpI2cTimeoutMilliseconds);

	if (status != kStatus_I2C_Success){
		SEGGER_RTT_WriteString(0, "Failed to write command :( \n");
		OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds);
	} else{
		//SEGGER_RTT_WriteString(0, "\nCommand given");
		OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds); 
	}

    OSA_TimeDelay(200);
	//Read from the calibration register
		status = I2C_DRV_MasterReceiveDataBlocking(0,
								&slave,
								NULL,
								0,
								i2c_buffer,
								4,
								gWarpI2cTimeoutMilliseconds);

		OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds);

		if (status != kStatus_I2C_Success){
			SEGGER_RTT_WriteString(0, "Failed to read  :( \n");
			OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds);
		} else {
			OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds);
			temperature = i2c_buffer[0] << 24;
			temperature |= i2c_buffer[1] << 16;
			temperature |= i2c_buffer[2] << 8;
			temperature |= i2c_buffer[3];
			celsius = ((1.0/(1UL << 16)) * temperature);
			SEGGER_RTT_printf(0, "\nCelsius reading > %d oC ", celsius);
			OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds);

		}
	disableI2Cpins();
	
	return celsius;
}


