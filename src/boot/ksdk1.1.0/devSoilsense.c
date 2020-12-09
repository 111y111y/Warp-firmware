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


extern volatile uint32_t		gWarpI2cBaudRateKbps;
extern volatile uint32_t		gWarpI2cTimeoutMilliseconds;
extern volatile uint32_t		gWarpSupplySettlingDelayMilliseconds;
extern volatile uint32_t		gWarpMenuPrintDelayMilliseconds;




i2c_status_t readRegisterSoil(i2c_device_t slave, uint8_t reg_start, uint8_t reg_end, uint8_t * i2c_buffer, uint16_t menuI2cPullupValue)
{
	
	i2c_status_t	status;
	uint8_t 	payload[2] = {reg_start,reg_end};
	
	enableI2Cpins(menuI2cPullupValue);

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
		SEGGER_RTT_WriteString(0, "Command given\n");
		OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds); 
	}

    OSA_TimeDelay(500);
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
			SEGGER_RTT_printf(0, "Register value: %x   %x\n", i2c_buffer[0], i2c_buffer[1]);
		}

	disableI2Cpins();
	
	return status;
}



