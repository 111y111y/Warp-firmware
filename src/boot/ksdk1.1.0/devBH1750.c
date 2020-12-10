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
#include "devBH1750.h"



extern volatile uint32_t		gWarpI2cBaudRateKbps;
extern volatile uint32_t		gWarpI2cTimeoutMilliseconds;
extern volatile uint32_t		gWarpSupplySettlingDelayMilliseconds;
extern volatile uint32_t		gWarpMenuPrintDelayMilliseconds;


int readlight(void)
{
    uint8_t		    i2c_buffer[2];
	i2c_status_t	status;
	i2c_device_t	slave = {
				.address = 0x23,
				.baudRate_kbps = gWarpI2cBaudRateKbps
				};
	OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds);
	uint8_t		command[1] = {0x23};
    int         reading = 0;
OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds);

	enableI2Cpins(32768);

	OSA_TimeDelay(200);
	status = I2C_DRV_MasterSendDataBlocking(0,
							&slave,
							NULL,
							0,
							(uint8_t *) command,
							1,
							gWarpI2cTimeoutMilliseconds);

	if (status != kStatus_I2C_Success){
		SEGGER_RTT_WriteString(0, "Failed to write command :( \n");
		OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds);
	} else{
		SEGGER_RTT_WriteString(0, "Command given\n");
		OSA_TimeDelay(gWarpMenuPrintDelayMilliseconds); 
	}

	OSA_TimeDelay(200);

		status = I2C_DRV_MasterReceiveDataBlocking(0,
								&slave,
								NULL,
								0,
								(uint8_t *)i2c_buffer,
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

return reading;

}