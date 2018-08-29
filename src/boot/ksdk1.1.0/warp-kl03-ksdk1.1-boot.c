/*
	Authored 2016. Phillip Stanley-Marbell.
	Adapted for FRDM-KL03Z 2018. Youchao Wang

	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	*	Redistributions of source code must retain the above
		copyright notice, this list of conditions and the following
		disclaimer.

	*	Redistributions in binary form must reproduce the above
		copyright notice, this list of conditions and the following
		disclaimer in the documentation and/or other materials
		provided with the distribution.

	*	Neither the name of the author nor the names of its
		contributors may be used to endorse or promote products
		derived from this software without specific prior written
		permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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



#define					kWarpConstantStringI2cFailure		"\rI2C failed, reg 0x%02x, code %d\n"
#define					kWarpConstantStringInvalidVoltage	"\r\n\n\nInvalid supply voltage [%d] mV\n\n\n"


volatile WarpSPIDeviceState		deviceADXL362State;
volatile WarpI2CDeviceState		deviceBMX055accelState;
volatile WarpI2CDeviceState		deviceBMX055gyroState;
volatile WarpI2CDeviceState		deviceBMX055magState;
volatile WarpI2CDeviceState		deviceMMA8451QState;
volatile WarpI2CDeviceState		deviceMAG3110State;
volatile WarpI2CDeviceState		deviceL3GD20HState;
volatile WarpI2CDeviceState		deviceBMP180State;
volatile WarpI2CDeviceState		deviceTMP006BState;
volatile WarpUARTDeviceState		devicePAN1326BState;


/*
 *	Initialization: Devices hanging off I2C
 */
void					initBMX055accel(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer);
void					initBMX055gyro(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer);
void					initBMX055mag(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer);
void					initMMA8451Q(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer);
void					initMAG3110(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer);
void					initL3GD20H(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer);
void					initBMP180(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer);
void					initTMP006B(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer);

/*
 *	Initialization: Devices hanging off SPI
 */
void					initADXL362(WarpSPIDeviceState volatile *  deviceStatePointer);

/*
 *	Initialization: Devices hanging off UART
 */
void					initPAN1326B(WarpUARTDeviceState volatile *  deviceStatePointer);


/*
 *	For devices that need to be explicitly put into sleep/low-power mode after POR
 */
void					sleepBMX055accel(void);
void					sleepBMX055gyro(void);
void					sleepBMX055mag(void);
void					sleepMMA8451Q(void);
void					sleepMAG3110(void);
void					sleepL3GD20H(void);
void					sleepBMP180(void);
void					sleepTMP006B(void);
void					sleepADXL362(void);
void					sleepPAN1326B(void);


/*
 *	Reading from sensor registers
 */
WarpStatus				readSensorRegisterADXL362(uint8_t deviceRegister);
WarpStatus				readSensorRegisterBMX055accel(uint8_t deviceRegister);
WarpStatus				readSensorRegisterBMX055gyro(uint8_t deviceRegister);
WarpStatus				readSensorRegisterBMX055mag(uint8_t deviceRegister);
WarpStatus				readSensorRegisterMMA8451Q(uint8_t deviceRegister);
WarpStatus				readSensorRegisterMAG3110(uint8_t deviceRegister);
WarpStatus				readSensorRegisterL3GD20H(uint8_t deviceRegister);
WarpStatus				readSensorRegisterBMP180(uint8_t deviceRegister);
WarpStatus				readSensorRegisterTMP006B(uint8_t deviceRegister);

WarpStatus				writeSensorRegisterADXL362(uint8_t command, uint8_t deviceRegister, uint8_t writeValue);

void					disableSWDEnablePTA1x2x3xGpio(void);
void					enableSWDDisablePTA1x2x3xGpio(void);
void					brieflyToggleEnablingSWD(void);
void					lowPowerPinStates(void);
void					disableTPS82675(void);
void					enableTPS82675(bool mode);
void					disableTPS82740A(void);
void					enableTPS82740A(uint16_t voltageMillivolts);
void					printPinDirections(void);
void					enableI2Cpins(bool pullupEnable);
void					disableI2Cpins(void);
void					enableSPIpins(bool driveI2cPinsHighToMatchSupply);
void					disableSPIpins(bool driveI2cPinsLow);
void					dumpProcessorStateWithSwdToggles(void);
void					repeatRegisterReadForDeviceAndAddress(WarpSensorDevice warpSensorDevice, uint8_t baseAddress, 
								bool pullupEnable, bool autoIncrement, int chunkReadsPerAddress, bool chatty,
								int spinDelay, int repetitionsPerAddress, uint16_t sssupplyMillivolts,
								uint16_t adaptiveSssupplyMaxMillivolts, uint8_t referenceByte);

int					char2int(int character);
void					enableSssupply(uint16_t voltageMillivolts);
void					disableSssupply(void);
void					activateAllLowPowerSensorModes(void);
void					powerupAllSensors(void);
uint8_t					readHexByte(void);
int					read4digits(void);

WarpStatus				writeByteToI2cDeviceRegister(uint8_t i2cAddress, bool sendCommandByte, uint8_t commandByte, bool sendPayloadByte, uint8_t payloadByte);
WarpStatus				writeBytesToSpi(uint8_t *  payloadBytes, int payloadLength, bool driveI2cPinsHighToMatchSupply, bool driveI2cPinsLow);
void					warpLowPowerSecondsSleep(uint32_t sleepSeconds, bool forceAllPinsIntoLowPowerState);



/*
 *	TODO: move this and possibly others into a structure
 */
volatile i2c_master_state_t		i2cMasterState;
volatile spi_master_state_t		spiMasterState;
volatile spi_master_user_config_t	spiUserConfig;


/*
 *	TODO: move magic default numbers into constant definitions.
 */
volatile uint32_t			gWarpI2cBaudRateKbps	= 1;
volatile uint32_t			gWarpUartBaudRateKbps	= 1;
volatile uint32_t			gWarpSpiBaudRateKbps	= 1;
volatile uint32_t			gWarpSleeptimeSeconds	= 0;




/*
 *	From KSDK power_manager_demo.c <<BEGIN>>>
 */

clock_manager_error_code_t clockManagerCallbackRoutine(clock_notify_struct_t *  notify, void *  callbackData);

/*
 *	static clock callback table.
 */
clock_manager_callback_user_config_t		clockManagerCallbackUserlevelStructure =
									{
										.callback	= clockManagerCallbackRoutine,
										.callbackType	= kClockManagerCallbackBeforeAfter,
										.callbackData	= NULL
									};

static clock_manager_callback_user_config_t *	clockCallbackTable[] =
									{
										&clockManagerCallbackUserlevelStructure
									};

clock_manager_error_code_t
clockManagerCallbackRoutine(clock_notify_struct_t *  notify, void *  callbackData)
{
	clock_manager_error_code_t result = kClockManagerSuccess;

	switch (notify->notifyType)
	{
		case kClockManagerNotifyBefore:
			break;
		case kClockManagerNotifyRecover:
		case kClockManagerNotifyAfter:
			break;
		default:
			result = kClockManagerError;
		break;
	}

	return result;
}


/*
 *	Override the RTC IRQ handler
 */
void
RTC_IRQHandler(void)
{
	if (RTC_DRV_IsAlarmPending(0))
	{
		RTC_DRV_SetAlarmIntCmd(0, false);
	}
}

/*
 *	Override the RTC Second IRQ handler
 */
void
RTC_Seconds_IRQHandler(void)
{
}

/*
 *	Power manager user callback
 */
power_manager_error_code_t callback0(power_manager_notify_struct_t *  notify,
					power_manager_callback_data_t *  dataPtr)
{
	WarpPowerManagerCallbackStructure *		callbackUserData = (WarpPowerManagerCallbackStructure *) dataPtr;
	power_manager_error_code_t			status = kPowerManagerError;

	switch (notify->notifyType)
	{
		case kPowerManagerNotifyBefore:
			status = kPowerManagerSuccess;
			break;
		case kPowerManagerNotifyAfter:
			status = kPowerManagerSuccess;
			break;
		default:
			callbackUserData->errorCount++;
			break;
	}

	return status;
}

/*
 *	From KSDK power_manager_demo.c <<END>>>
 */








/*
 *	Analog Devices ADXL362.
 *
 *	From device manual, Rev. B, Page 19 of 44:
 *
 *		"
 *		The SPI port uses a multibyte structure 
 *		wherein the first byte is a command. The 
 *		ADXL362 command set is:
 *
 *		-	0x0A: write register
 *		-	0x0B: read register
 *		-	0x0D: read FIFO
 *		"
 */
void
initADXL362(WarpSPIDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->signalType	= (	kWarpTypeMaskAccelerationX |
						kWarpTypeMaskAccelerationY |
						kWarpTypeMaskAccelerationZ |
						kWarpTypeMaskTemperature
					);
	return;
}

WarpStatus
readSensorRegisterADXL362(uint8_t deviceRegister)
{	
	return writeSensorRegisterADXL362(0x0B /* command == read register */, deviceRegister, 0x00 /* writeValue */);
}

WarpStatus
writeSensorRegisterADXL362(uint8_t command, uint8_t deviceRegister, uint8_t writeValue)
{	
	/*
	 *	Populate the shift-out register with the read-register command,
	 *	followed by the register to be read, followed by a zero byte.
	 */
	deviceADXL362State.spiSourceBuffer[0] = command;
	deviceADXL362State.spiSourceBuffer[1] = deviceRegister;
	deviceADXL362State.spiSourceBuffer[2] = writeValue;

	deviceADXL362State.spiSinkBuffer[0] = 0x00;
	deviceADXL362State.spiSinkBuffer[1] = 0x00;
	deviceADXL362State.spiSinkBuffer[2] = 0x00;

	/*
	 *	First, create a falling edge on chip-select.
	 *
	 *	NOTE: we keep the kWarpPinADXL362_CS_PAN1326_nSHUTD
	 *	low at all times so that PAN1326 is shut down.
	 *
	 */
	GPIO_DRV_SetPinOutput(kWarpPinADXL362_CS_PAN1326_nSHUTD);
	OSA_TimeDelay(50);
	GPIO_DRV_ClearPinOutput(kWarpPinADXL362_CS_PAN1326_nSHUTD);


	/*
	 *	The result of the SPI transaction will be stored in deviceADXL362State.spiSinkBuffer.
	 *
	 *	Providing a device structure here is optional since it 
	 *	is already provided when we did SPI_DRV_MasterConfigureBus(),
	 *	so we pass in NULL.
	 *
	 *	TODO: the "master instance" is always 0 for the KL03 since
	 *	there is only one SPI peripheral. We however should remove
	 *	the '0' magic number and place this in a Warp-HWREV0 header
	 *	file.
	 */
	enableSPIpins(true /* driveI2cPinsHighToMatchSupply */);
	deviceADXL362State.ksdk_spi_status = SPI_DRV_MasterTransferBlocking(0 /* master instance */,
					NULL /* spi_master_user_config_t */,
					(const uint8_t * restrict)deviceADXL362State.spiSourceBuffer,
					(uint8_t * restrict)deviceADXL362State.spiSinkBuffer,
					3 /* transfer size */,
					1000 /* timeout in microseconds (unlike I2C which is ms) */);
	disableSPIpins(true /* driveI2cPinsLow */);

	/*
	 *	NOTE: we leave the kWarpPinADXL362_CS_PAN1326_nSHUTD
	 *	low at all times so that PAN1326 is shut down.
	 */

	return kWarpStatusOK;
}



/*
 *	Bosch Sensortec BMX055.
 */
void
initBMX055accel(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->i2cAddress	= i2cAddress;
	deviceStatePointer->signalType	= (	kWarpTypeMaskAccelerationX |
						kWarpTypeMaskAccelerationY |
						kWarpTypeMaskAccelerationZ |
						kWarpTypeMaskTemperature
					);
	return;
}

WarpStatus
readSensorRegisterBMX055accel(uint8_t deviceRegister)
{
	uint8_t 	cmdBuf[1]	= {0xFF};
	i2c_status_t	returnValue;


	if (deviceRegister > 0x3F)
	{
		return kWarpStatusBadDeviceCommand;
	}

	i2c_device_t slave =
	{
		.address = deviceBMX055accelState.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};

	cmdBuf[0] = deviceRegister;

	returnValue = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							(uint8_t *)deviceBMX055accelState.i2cBuffer,
							1,
							500 /* timeout in milliseconds */);

	if (returnValue == kStatus_I2C_Success)
	{
		//...
	}
	else
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}



void
initBMX055mag(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->i2cAddress	= i2cAddress;
	deviceStatePointer->signalType	= (	kWarpTypeMaskMagneticX |
						kWarpTypeMaskMagneticY |
						kWarpTypeMaskMagneticZ |
						kWarpTypeMaskTemperature
					);

	return;
}

WarpStatus
readSensorRegisterBMX055mag(uint8_t deviceRegister)
{
	uint8_t 	cmdBuf[1]	= {0xFF};
	i2c_status_t	returnValue;


	if (deviceRegister > 0x52 || deviceRegister < 0x40)
	{
		return kWarpStatusBadDeviceCommand;
	}

	i2c_device_t slave =
	{
		.address = deviceBMX055magState.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};


	cmdBuf[0] = deviceRegister;

	returnValue = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							(uint8_t *)deviceBMX055magState.i2cBuffer,
							1,
							500 /* timeout in milliseconds */);

	if (returnValue == kStatus_I2C_Success)
	{
		//...
	}
	else
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}



void
initBMX055gyro(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->i2cAddress	= i2cAddress;
	deviceStatePointer->signalType	= (	kWarpTypeMaskAngularRateX |
						kWarpTypeMaskAngularRateY |
						kWarpTypeMaskAngularRateZ |
						kWarpTypeMaskTemperature
					);
	return;
}

WarpStatus
readSensorRegisterBMX055gyro(uint8_t deviceRegister)
{
	uint8_t 	cmdBuf[1]	= {0xFF};
	i2c_status_t	returnValue;


	if (deviceRegister > 0x3F)
	{
		return kWarpStatusBadDeviceCommand;
	}

	i2c_device_t slave =
	{
		.address = deviceBMX055gyroState.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};


	cmdBuf[0] = deviceRegister;

	returnValue = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							(uint8_t *)deviceBMX055gyroState.i2cBuffer,
							1,
							500 /* timeout in milliseconds */);

	if (returnValue == kStatus_I2C_Success)
	{
		//...
	}
	else
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}



void
initMMA8451Q(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->i2cAddress	= i2cAddress;
	deviceStatePointer->signalType	= (	kWarpTypeMaskAccelerationX |
						kWarpTypeMaskAccelerationY |
						kWarpTypeMaskAccelerationZ |
						kWarpTypeMaskTemperature
					);
	return;
}

WarpStatus
readSensorRegisterMMA8451Q(uint8_t deviceRegister)
{
	uint8_t cmdBuf[1]	= {0xFF};
	i2c_status_t		returnValue;


	switch (deviceRegister)
	{
		case 0x00: case 0x01: case 0x02: case 0x03: 
		case 0x04: case 0x05: case 0x06: case 0x09:
		case 0x0a: case 0x0b: case 0x0c: case 0x0d:
		case 0x0e: case 0x0f: case 0x10: case 0x11:
		case 0x12: case 0x13: case 0x14: case 0x15:
		case 0x16: case 0x17: case 0x18: case 0x1d:
		case 0x1e: case 0x1f: case 0x20: case 0x21:
		case 0x22: case 0x23: case 0x24: case 0x25:
		case 0x26: case 0x27: case 0x28: case 0x29:
		case 0x2a: case 0x2b: case 0x2c: case 0x2d:
		case 0x2e: case 0x2f: case 0x30: case 0x31:
		{
			/* OK */
			break;
		}
		
		default:
		{
			return kWarpStatusBadDeviceCommand;
		}
	}

	i2c_device_t slave =
	{
		.address = deviceMMA8451QState.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};


	cmdBuf[0] = deviceRegister;

	returnValue = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							(uint8_t *)deviceMMA8451QState.i2cBuffer,
							1,
							500 /* timeout in milliseconds */);

	if (returnValue == kStatus_I2C_Success)
	{
		//...
	}
	else
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}



void
initMAG3110(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->i2cAddress	= i2cAddress;
	deviceStatePointer->signalType	= (
						kWarpTypeMaskMagneticX |
						kWarpTypeMaskMagneticY |
						kWarpTypeMaskMagneticZ |
						kWarpTypeMaskTemperature
					);

	return;
}

WarpStatus
readSensorRegisterMAG3110(uint8_t deviceRegister)
{
	uint8_t		cmdBuf[1]	= {0xFF};
	i2c_status_t	returnValue;


	i2c_device_t slave =
	{
		.address = deviceMAG3110State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};


	/*
	 *	Steps (Repeated single-byte read. See Section 4.2.2 of MAG3110 manual.):
	 *
	 *	(1) Write transaction beginning with start condition, slave address, and pointer address.
	 *
	 *	(2) Read transaction beginning with start condition, followed by slave address, and read 1 byte payload
	*/

	cmdBuf[0] = deviceRegister;

	returnValue = I2C_DRV_MasterSendDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							NULL,
							0,
							500 /* timeout in milliseconds */);

	returnValue = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							(uint8_t *)deviceMAG3110State.i2cBuffer,
							1,
							500 /* timeout in milliseconds */);

	if (returnValue == kStatus_I2C_Success)
	{
		//...
	}
	else
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}



void
initL3GD20H(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->i2cAddress	= i2cAddress;
	deviceStatePointer->signalType	= (	kWarpTypeMaskAngularRateX |
						kWarpTypeMaskAngularRateY |
						kWarpTypeMaskAngularRateZ |
						kWarpTypeMaskTemperature
					);
	return;
}

WarpStatus
readSensorRegisterL3GD20H(uint8_t deviceRegister)
{
	uint8_t 	cmdBuf[1]	= {0xFF};
	i2c_status_t	returnValue;


	if ((deviceRegister < 0x0F) || (deviceRegister > 0x39))
	{
		return kWarpStatusBadDeviceCommand;
	}

	i2c_device_t slave =
	{
		.address = deviceL3GD20HState.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};


	cmdBuf[0] = deviceRegister;


	/*
	 *	Steps (Repeated single-byte read. See STable 15 and Table 16 of L3GD20H manual.):
	 *
	 *	(1) Write transaction beginning with start condition, slave address, and sub address.
	 *
	 *	(2) Read transaction beginning with start condition, followed by slave address, and read 1 byte payload
	 */

	returnValue = I2C_DRV_MasterSendDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							NULL,
							0,
							500 /* timeout in milliseconds */);		

	returnValue = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							(uint8_t *)deviceL3GD20HState.i2cBuffer,
							1,
							500 /* timeout in milliseconds */);

	if (returnValue == kStatus_I2C_Success)
	{
		//...
	}
	else
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}



void
initBMP180(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->i2cAddress	= i2cAddress;
	deviceStatePointer->signalType	= (kWarpTypeMaskPressure | kWarpTypeMaskTemperature);

	return;
}

WarpStatus
readSensorRegisterBMP180(uint8_t deviceRegister)
{
	uint8_t 	cmdBuf[1]	= {0xFF};
	i2c_status_t	returnValue;


	switch (deviceRegister)
	{
		case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF:
		case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5:
		case 0xB6: case 0xB7: case 0xB8: case 0xB9: case 0xBA: case 0xBB:
		case 0xBC: case 0xBD: case 0xBE: case 0xBF:
		case 0xD0: case 0xE0: case 0xF4: case 0xF6: case 0xF7: case 0xF8:
		{
			/* OK */
			break;
		}
		
		default:
		{
			return kWarpStatusBadDeviceCommand;
		}
	}


	i2c_device_t slave =
	{
		.address = deviceBMP180State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};


	cmdBuf[0] = deviceRegister;


	returnValue = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							(uint8_t *)deviceBMP180State.i2cBuffer,
							1,
							500 /* timeout in milliseconds */);

	if (returnValue == kStatus_I2C_Success)
	{
		//...
	}
	else
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}



void
initTMP006B(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->i2cAddress 	= i2cAddress;
	deviceStatePointer->signalType	= kWarpTypeMaskTemperature;

	return;
}

WarpStatus
readSensorRegisterTMP006B(uint8_t deviceRegister)
{
	uint8_t		cmdBuf[1] = {0xFF};
	i2c_status_t	returnValue;

	i2c_device_t slave =
	{
		.address = deviceTMP006BState.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};

	cmdBuf[0] = deviceRegister;

	returnValue = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							(uint8_t *)deviceTMP006BState.i2cBuffer,
							2,
							500 /* timeout in milliseconds */);

	if (returnValue == kStatus_I2C_Success)
	{
		//...
	}
	else
	{
		return kWarpStatusDeviceCommunicationFailed;
	}


	return kWarpStatusOK;
}


void
initPAN1326B(WarpUARTDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->signalType	= kWarpTypeMaskTemperature;

	/*
	 *	Shutdown the Module
	 */
	GPIO_DRV_ClearPinOutput(kWarpPinADXL362_CS_PAN1326_nSHUTD);
}


void
enableSPIpins(bool driveI2cPinsHighToMatchSupply)
{
	CLOCK_SYS_EnableSpiClock(0);

	/*	Warp KL03_SPI_MISO	--> PTA6	(ALT3)		*/
	PORT_HAL_SetMuxMode(PORTA_BASE, 6, kPortMuxAlt3);

	/*	Warp KL03_SPI_MOSI	--> PTA7	(ALT3)		*/
	PORT_HAL_SetMuxMode(PORTA_BASE, 7, kPortMuxAlt3);

	/*	Warp KL03_SPI_SCK	--> PTA9	(ALT3)		*/
	PORT_HAL_SetMuxMode(PORTA_BASE, 9, kPortMuxAlt3);


	/*
	 *	Initialize SPI master. See KSDK13APIRM.pdf Section 70.4
	 *
	 */
	uint32_t			calculatedBaudRate;
	spiUserConfig.polarity		= kSpiClockPolarity_ActiveHigh;
	spiUserConfig.phase		= kSpiClockPhase_FirstEdge;
	spiUserConfig.direction		= kSpiMsbFirst;
	spiUserConfig.bitsPerSec	= gWarpSpiBaudRateKbps * 1000;
	SPI_DRV_MasterInit(0 /* SPI master instance */, (spi_master_state_t *)&spiMasterState);
	SPI_DRV_MasterConfigureBus(0 /* SPI master instance */, (spi_master_user_config_t *)&spiUserConfig, &calculatedBaudRate);

	/*
	 *	Whenever we drive the SPI_SCK high, it will connect the
	 *	I2C_PULLUP to SSSUPPLY. We therefore want both I2C pins
	 *	driven high when SPI is active.
	 *
	 *	Drive the I2C pins high:
	 */
	if (driveI2cPinsHighToMatchSupply)
	{
		GPIO_DRV_SetPinOutput(kWarpPinI2C0_SDA);
		GPIO_DRV_SetPinOutput(kWarpPinI2C0_SCL);	
	}
}



void
disableSPIpins(bool driveI2cPinsLow)
{
	SPI_DRV_MasterDeinit(0);


	/*	Warp KL03_SPI_MISO	--> PTA6	(GPI)		*/
	PORT_HAL_SetMuxMode(PORTA_BASE, 6, kPortMuxAsGpio);

	/*	Warp KL03_SPI_MOSI	--> PTA7	(GPIO)		*/
	PORT_HAL_SetMuxMode(PORTA_BASE, 7, kPortMuxAsGpio);

	/*	Warp KL03_SPI_SCK	--> PTA9	(GPIO)		*/
	PORT_HAL_SetMuxMode(PORTA_BASE, 9, kPortMuxAsGpio);

	GPIO_DRV_ClearPinOutput(kWarpPinSPI_MOSI);
	GPIO_DRV_ClearPinOutput(kWarpPinSPI_MISO);
	GPIO_DRV_ClearPinOutput(kWarpPinSPI_SCK_I2C_PULLUP_EN);

	/*
	 *	Drive the I2C pins low (we drove them high when we enabled SPI so that the SPI_SCK toggle, which controls pullup enable, is not causing drain.):
	 */
	if (driveI2cPinsLow)
	{
		GPIO_DRV_ClearPinOutput(kWarpPinI2C0_SDA);
		GPIO_DRV_ClearPinOutput(kWarpPinI2C0_SCL);
	}

	CLOCK_SYS_DisableSpiClock(0);
}



void
enableI2Cpins(bool pullupEnable)
{
	CLOCK_SYS_EnableI2cClock(0);

	/*	Warp KL03_I2C0_SCL	--> PTB3	(ALT2 == I2C)		*/
	PORT_HAL_SetMuxMode(PORTB_BASE, 3, kPortMuxAlt2);

	/*	Warp KL03_I2C0_SDA	--> PTB4	(ALT2 == I2C)		*/
	PORT_HAL_SetMuxMode(PORTB_BASE, 4, kPortMuxAlt2);

	/*	Warp KL03_SPI_SCK	--> PTA9	(ALT1 = GPIO)		*/
	PORT_HAL_SetMuxMode(PORTA_BASE, 9, kPortMuxAsGpio);


	I2C_DRV_MasterInit(0 /* I2C instance */, (i2c_master_state_t *)&i2cMasterState);


	/*
	 *	Drive kWarpPinSPI_SCK_I2C_PULLUP_EN high to turn on the pullups
	 */
	if (pullupEnable)
	{
		GPIO_DRV_SetPinOutput(kWarpPinSPI_SCK_I2C_PULLUP_EN);
	}
}



void
disableI2Cpins(void)
{
	I2C_DRV_MasterDeinit(0 /* I2C instance */);	


	/*	Warp KL03_I2C0_SCL	--> PTB3	(GPIO)			*/
	PORT_HAL_SetMuxMode(PORTB_BASE, 3, kPortMuxAsGpio);

	/*	Warp KL03_I2C0_SDA	--> PTB4	(GPIO)			*/
	PORT_HAL_SetMuxMode(PORTB_BASE, 4, kPortMuxAsGpio);
	
	/*	Warp KL03_SPI_SCK	--> PTA9	(ALT1 = GPIO)		*/
	PORT_HAL_SetMuxMode(PORTA_BASE, 9, kPortMuxAsGpio);
	
	/*
	 *	Clear the pullup.
	 */
	GPIO_DRV_ClearPinOutput(kWarpPinSPI_SCK_I2C_PULLUP_EN);

	/*
	 *	Drive the I2C pins low
	 */
	GPIO_DRV_ClearPinOutput(kWarpPinI2C0_SDA);
	GPIO_DRV_ClearPinOutput(kWarpPinI2C0_SCL);


	CLOCK_SYS_DisableI2cClock(0);
}



void
disableSWDEnablePTA1x2x3xGpio(void)
{
	/*	Warp LED 1 / TS5A3154_IN	--> PTA0	(ALT1)		*/
	PORT_HAL_SetMuxMode(PORTA_BASE, 0, kPortMuxAsGpio);

	/*	Warp LED 2 / TS5A3154_EN	--> PTA1	(ALT1)		*/
	PORT_HAL_SetMuxMode(PORTA_BASE, 1, kPortMuxAsGpio);

	/*	Warp LED 3 / SI4705_nRST	--> PTA2	(ALT1)		*/
	PORT_HAL_SetMuxMode(PORTA_BASE, 2, kPortMuxAsGpio);
}



void
enableSWDDisablePTA1x2x3xGpio(void)
{
	/*
	 *	NOTE: we switch them to ALT3 to get back the SWD functionality.
	 *
	 *	Because we have set FOPT, the default on PTA1 is no longer RESET_b.
	 */

	/*	Warp LED 1 / TS5A3154_IN	--> SWD_CLK	(ALT3)		*/
	PORT_HAL_SetMuxMode(PORTA_BASE, 0, kPortMuxAlt3);

	/*	Warp LED 3 / SI4705_nRST	--> SWD_DIO	(ALT3)		*/
	PORT_HAL_SetMuxMode(PORTA_BASE, 2, kPortMuxAlt3);
}



void
brieflyToggleEnablingSWD(void)
{
	/*
	 *	Briefly enable SWD to let attached JLINK read the RTT buffer for print().
	 */
	enableSWDDisablePTA1x2x3xGpio();
	OSA_TimeDelay(50);
	disableSWDEnablePTA1x2x3xGpio();	
}



void
lowPowerPinStates(void)
{
	/*
	 *	Following Section 5 of "Power Management for Kinetis L Family" (AN5088.pdf),
	 *	we configure all pins as output and set them to a known state. We choose
	 *	to set them all to '0' since it happens that the devices we want to keep
	 *	deactivated (SI4705, PAN1326) also need '0'.
	 */

	/*
	 *			PORT A
	 */
	PORT_HAL_SetMuxMode(PORTA_BASE, 0, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 1, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 2, kPortMuxAsGpio);
	
	/*
	 *	PTA3 and PTA4 are the EXTAL/XTAL
	 */
	PORT_HAL_SetMuxMode(PORTA_BASE, 3, kPortPinDisabled);
	PORT_HAL_SetMuxMode(PORTA_BASE, 4, kPortPinDisabled);

	PORT_HAL_SetMuxMode(PORTA_BASE, 5, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 6, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 7, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 8, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 9, kPortMuxAsGpio);
	
	/*
	 *	NOTE: The KL03 has no PTA10 or PTA11
	 */

	PORT_HAL_SetMuxMode(PORTA_BASE, 12, kPortMuxAsGpio);

	/*
	 *			PORT B
	 */
	PORT_HAL_SetMuxMode(PORTB_BASE, 0, kPortMuxAsGpio);
	
	/*
	 *	PTB1 is connected to VDD. Keep 'disabled as analog'
	 */
	PORT_HAL_SetMuxMode(PORTB_BASE, 1, kPortPinDisabled);

	PORT_HAL_SetMuxMode(PORTB_BASE, 2, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 3, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 4, kPortMuxAsGpio);
	
	PORT_HAL_SetMuxMode(PORTB_BASE, 5, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 6, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 7, kPortMuxAsGpio);

	/*
	 *	NOTE: The KL03 has no PTB8 or PTB9
	 */

	/*
	 *		PTB10 is unconnected in Rev 0.2 HW
	 */
	PORT_HAL_SetMuxMode(PORTB_BASE, 10, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 11, kPortMuxAsGpio);

	/*
	 *	NOTE: The KL03 has no PTB12
	 */

	PORT_HAL_SetMuxMode(PORTB_BASE, 13, kPortMuxAsGpio);

	/*
	 *	Now, set all the pins (except kWarpPinKL03_VDD_ADC) to 0
	 */
	/* GPIO_DRV_ClearPinOutput(kWarpPinKL03_VDD_ADC); */	
	GPIO_DRV_ClearPinOutput(kWarpPinTPS82675_MODE);
	GPIO_DRV_ClearPinOutput(kWarpPinI2C0_SDA);
	GPIO_DRV_ClearPinOutput(kWarpPinI2C0_SCL);
	GPIO_DRV_ClearPinOutput(kWarpPinSPI_MOSI);
	GPIO_DRV_ClearPinOutput(kWarpPinSPI_MISO);
	GPIO_DRV_ClearPinOutput(kWarpPinSPI_SCK_I2C_PULLUP_EN);
	GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL2);
	GPIO_DRV_ClearPinOutput(kWarpPinADXL362_CS_PAN1326_nSHUTD);
	GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_CTLEN);
	GPIO_DRV_ClearPinOutput(kWarpPinTPS82675_EN);
	GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL1);
	GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL3);
	GPIO_DRV_ClearPinOutput(kWarpPinCLKOUT32K);

	GPIO_DRV_ClearPinOutput(kWarpPinLED1_TS5A3154_IN);
	GPIO_DRV_ClearPinOutput(kWarpPinLED2_TS5A3154_nEN);
	GPIO_DRV_ClearPinOutput(kWarpPinLED3_SI4705_nRST);

	GPIO_DRV_ClearPinOutput(kWarpPinUnusedPTB6);
	GPIO_DRV_ClearPinOutput(kWarpPinUnusedPTB7);
	GPIO_DRV_ClearPinOutput(kWarpPinUnusedPTB10);

	/*
	 *	HCI_RX / kWarpPinI2C0_SCL is an input. Set it low.
	 */
	//GPIO_DRV_SetPinOutput(kWarpPinI2C0_SCL);

	/*
	 *	HCI_TX / kWarpPinI2C0_SDA is an output. Set it high.
	 */
	//GPIO_DRV_SetPinOutput(kWarpPinI2C0_SDA);

	/*
	 *	HCI_RTS / kWarpPinSPI_MISO is an output. Set it high.
	 */
	//GPIO_DRV_SetPinOutput(kWarpPinSPI_MISO);

	/*
	 *	From PAN1326 manual, page 10:
	 *
	 *		"When HCI_CTS is high, then CC256X is not allowed to send data to Host device"
	 */
	//GPIO_DRV_SetPinOutput(kWarpPinSPI_MOSI);
}



void
disableTPS82675(void)
{
	/*
	 *	Disable the TPS8267[1,5]. From Manual:
	 *
	 *		"MODE = LOW: The device is operating in regulated frequency
	 *		pulse width modulation mode (PWM) at high-load currents and
	 *		in pulse frequency modulation mode (PFM) at light load currents.
	 *		MODE = HIGH: Low-noise mode is enabled and regulated frequency
	 *		PWM operation is forced."
	 */
	GPIO_DRV_ClearPinOutput(kWarpPinTPS82675_MODE);
	GPIO_DRV_ClearPinOutput(kWarpPinTPS82675_EN);
}



void
enableTPS82675(bool mode)
{
	/*
	 *	Enable the TPS8267[1,5]. From Manual:
	 *
	 *		"MODE = LOW: The device is operating in regulated frequency
	 *		pulse width modulation mode (PWM) at high-load currents and
	 *		in pulse frequency modulation mode (PFM) at light load currents.
	 *		MODE = HIGH: Low-noise mode is enabled and regulated frequency
	 *		PWM operation is forced."
	 */
	if (mode)
	{
		GPIO_DRV_SetPinOutput(kWarpPinTPS82675_MODE);
	}
	else
	{
		GPIO_DRV_ClearPinOutput(kWarpPinTPS82675_MODE);
	}

	GPIO_DRV_SetPinOutput(kWarpPinTPS82675_EN);
	OSA_TimeDelay(1);


	/*
	 *	Make sure the TS5A3154 power switch is enabled.
	 */
	GPIO_DRV_ClearPinOutput(kWarpPinLED2_TS5A3154_nEN);

	/*
	 *	Select the TS5A3154 to use the output of the TPS82675
	 *
	 *	IN = high selects the output of the TPS82740:
	 *	IN = low selects the output of the TPS82675:
	 */
	GPIO_DRV_ClearPinOutput(kWarpPinLED1_TS5A3154_IN);
}



void
disableTPS82740A(void)
{
	/*
	 *	Enable/disable the TPS82740A. From Manual:
	 *
	 *		VSEL1 VSEL2 VSEL3:	000-->1.8V, 111-->2.5V
	 */
	GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL1);
	GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL2);
	GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL3);
	GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_CTLEN);
}



void
enableTPS82740A(uint16_t voltageMillivolts)
{
	switch(voltageMillivolts)
	{
		case 1800:
		{
			GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL1);
			GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL2);
			GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL3);
			
			break;
		}

		case 1900:
		{
			GPIO_DRV_SetPinOutput(kWarpPinTPS82740A_VSEL1);
			GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL2);
			GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL3);
			
			break;
		}

		case 2000:
		{
			GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL1);
			GPIO_DRV_SetPinOutput(kWarpPinTPS82740A_VSEL2);
			GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL3);
			
			break;
		}

		case 2100:
		{
			GPIO_DRV_SetPinOutput(kWarpPinTPS82740A_VSEL1);
			GPIO_DRV_SetPinOutput(kWarpPinTPS82740A_VSEL2);
			GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL3);
			
			break;
		}

		case 2200:
		{
			GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL1);
			GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL2);
			GPIO_DRV_SetPinOutput(kWarpPinTPS82740A_VSEL3);
			
			break;
		}

		case 2300:
		{
			GPIO_DRV_SetPinOutput(kWarpPinTPS82740A_VSEL1);
			GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL2);
			GPIO_DRV_SetPinOutput(kWarpPinTPS82740A_VSEL3);
			
			break;
		}

		case 2400:
		{
			GPIO_DRV_ClearPinOutput(kWarpPinTPS82740A_VSEL1);
			GPIO_DRV_SetPinOutput(kWarpPinTPS82740A_VSEL2);
			GPIO_DRV_SetPinOutput(kWarpPinTPS82740A_VSEL3);
			
			break;
		}

		case 2500:
		{
			GPIO_DRV_SetPinOutput(kWarpPinTPS82740A_VSEL1);
			GPIO_DRV_SetPinOutput(kWarpPinTPS82740A_VSEL2);
			GPIO_DRV_SetPinOutput(kWarpPinTPS82740A_VSEL3);
			
			break;
		}

		/*
		 *	NOTE/TODO: If voltage passed in does not match a desired level,
		 *	we match it to closest in supportedrange.
		 */
		default:
		{
			SEGGER_RTT_printf(0, kWarpConstantStringInvalidVoltage, voltageMillivolts);brieflyToggleEnablingSWD();
		}
	}

	GPIO_DRV_SetPinOutput(kWarpPinTPS82740A_CTLEN);
	OSA_TimeDelay(1);


	/*
	 *	Make sure the TS5A3154 power switch is enabled.
	 */
	GPIO_DRV_ClearPinOutput(kWarpPinLED2_TS5A3154_nEN);

	/*
	 *	Select the TS5A3154 to use the output of the TPS82740
	 *
	 *		IN = high selects the output of the TPS82740:
	 *		IN = low selects the output of the TPS82675:
	 */
	GPIO_DRV_SetPinOutput(kWarpPinLED1_TS5A3154_IN);
}



void
enableSssupply(uint16_t voltageMillivolts)
{
	if (voltageMillivolts == 1800)
	{
		enableTPS82675(0 /* mode */);
		disableTPS82740A();
	}
	else if (voltageMillivolts > 1800 && voltageMillivolts <= 2500)
	{
		enableTPS82740A(voltageMillivolts);
		disableTPS82675();
	}
	else
	{
		SEGGER_RTT_printf(0, kWarpConstantStringInvalidVoltage, voltageMillivolts);brieflyToggleEnablingSWD();
	}
}



void
disableSssupply(void)
{
	disableTPS82740A();
	disableTPS82675();
	
	/*
	 *	Clear the pin. This sets the TS5A3154 to use the output of the TPS82675,
	 *	which shouldn't matter in any case. The main objective here is to clear
	 *	the pin to reduce pwoer drain.
	 *
	 *		IN = high selects the output of the TPS82740:
	 *		IN = low selects the output of the TPS82675:
	 */
	GPIO_DRV_ClearPinOutput(kWarpPinLED1_TS5A3154_IN);
}



void
warpLowPowerSecondsSleep(uint32_t sleepSeconds, bool forceAllPinsIntoLowPowerState)
{
	/*
	 *	Set all pins into low-power states. We don't just disable all pins, as the various devices hanging off will be left in higher power draw state. And manuals say set pins to output to reduce power.
	 */
	if (forceAllPinsIntoLowPowerState)
	{
		lowPowerPinStates();
	}

	warpSetLowPowerMode(kWarpPowerModeVLPR, 0);
	warpSetLowPowerMode(kWarpPowerModeVLPS, sleepSeconds);
}




int
main(void)
{
	uint8_t					key;
	WarpSensorDevice		menuTargetSensor = kWarpSensorMMA8451Q;
	bool					menuI2cPullupEnable = true;
	uint8_t					menuRegisterAddress = 0x00;
	uint16_t				menuSupplyVoltage = 0;


	rtc_datetime_t				warpBootDate;

	power_manager_user_config_t		warpPowerModeWaitConfig;
	power_manager_user_config_t		warpPowerModeStopConfig;
	power_manager_user_config_t		warpPowerModeVlpwConfig;
	power_manager_user_config_t		warpPowerModeVlpsConfig;
	power_manager_user_config_t		warpPowerModeVlls0Config;
	power_manager_user_config_t		warpPowerModeVlls1Config;
	power_manager_user_config_t		warpPowerModeVlls3Config;
	power_manager_user_config_t		warpPowerModeRunConfig;

	const power_manager_user_config_t	warpPowerModeVlprConfig = {
							.mode			= kPowerManagerVlpr,
							.sleepOnExitValue	= false,
							.sleepOnExitOption	= false
						};

	power_manager_user_config_t const *	powerConfigs[] = {
							/*
							 *	NOTE: This order is depended on by POWER_SYS_SetMode()
							 *
							 *	See KSDK13APIRM.pdf Section 55.5.3
							 */
							&warpPowerModeWaitConfig,
							&warpPowerModeStopConfig,
							&warpPowerModeVlprConfig,
							&warpPowerModeVlpwConfig,
							&warpPowerModeVlpsConfig,
							&warpPowerModeVlls0Config,
							&warpPowerModeVlls1Config,
							&warpPowerModeVlls3Config,
							&warpPowerModeRunConfig,
						};

	WarpPowerManagerCallbackStructure			powerManagerCallbackStructure;

	/*
	 *	Callback configuration structure for power manager
	 */
	const power_manager_callback_user_config_t callbackCfg0 = {
							callback0,
							kPowerManagerCallbackBeforeAfter,
							(power_manager_callback_data_t *) &powerManagerCallbackStructure};

	/*
	 *	Pointers to power manager callbacks.
	 */
	power_manager_callback_user_config_t const *	callbacks[] = {
								&callbackCfg0
						};



	/*
	 *	Enable clock for I/O PORT A and PORT B
	 */
	CLOCK_SYS_EnablePortClock(0);
	CLOCK_SYS_EnablePortClock(1);



	/*
	 *	Setup board clock source.
	 */
	g_xtal0ClkFreq = 32768U;



	/*
	 *	Initialize KSDK Operating System Abstraction layer (OSA) layer.
	 */
	OSA_Init();



	/*
	 *	Configure all the pins (GPIO, ADC, I2C, SPI, UART, etc.)
	 *
	 *	NOTE:	Be careful not to assign SWD pins as GPIO, since 
	 *	this can prevent debugger from gaining control of the 
	 *	processor.
	 *
	 *	Before taking over the SWD pins, wait 
	 *	to give debugger a chance to force CPU into debug mode
	 *	if attached. This requires the SWD_DIO and SWD_CLK pins
	 *	to not yet be configured as GPIO. The SWD_RST pin could
	 *	indeed be set as GPIO by the FOPT byte, but that is not
	 *	needed to get access to the CPU. See jlink manual for
	 *	"Reset strategy" and "reset types".
	 *
	 */
	OSA_TimeDelay(3000);


	/*
	 *	An additional busy delay loop to make assurance doubly-sure.
	 */
	do
	{
		int32_t	i;
		for (i = 0; i < 500; i++)
		{
			__asm("nop");
		}
	} while (0);



	/*
	 *	Setup SEGGER RTT to output as much as fits in buffers.
	 *
	 *	Using SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL can lead to deadlock, since
	 *	we might have SWD disabled at time of blockage.
	 */
	SEGGER_RTT_Init();
	SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);


	SEGGER_RTT_WriteString(0, "\n\n\n\rBooting Warp-adpated in 3... ");
	OSA_TimeDelay(1000);
	SEGGER_RTT_WriteString(0, "2... ");
	OSA_TimeDelay(1000);
	SEGGER_RTT_WriteString(0, "1...\n\r");
	OSA_TimeDelay(1000);



	/*
	 *	Configure Clock Manager to default, and set callback for Clock Manager mode transition.
	 *
	 *	See "Clocks and Low Power modes with KSDK and Processor Expert" document (Low_Power_KSDK_PEx.pdf)
	 */
	CLOCK_SYS_Init(	g_defaultClockConfigurations,
			CLOCK_CONFIG_NUM,
			&clockCallbackTable,
			ARRAY_SIZE(clockCallbackTable)
			);
	CLOCK_SYS_UpdateConfiguration(CLOCK_CONFIG_INDEX_FOR_RUN, kClockManagerPolicyForcible);


	/*
	 *	Initialize RTC Driver
	 */
	RTC_DRV_Init(0);



	/*
	 *	Set initial date to 1st January 2016 00:00, and set date via RTC driver
	 */
	warpBootDate.year	= 2016U;
	warpBootDate.month	= 1U;
	warpBootDate.day	= 1U;
	warpBootDate.hour	= 0U;
	warpBootDate.minute	= 0U;
	warpBootDate.second	= 0U;
	RTC_DRV_SetDatetime(0, &warpBootDate);



	/*
	 *	Setup Power Manager Driver
	 */
	memset(&powerManagerCallbackStructure, 0, sizeof(WarpPowerManagerCallbackStructure));


	warpPowerModeVlpwConfig = warpPowerModeVlprConfig;
	warpPowerModeVlpwConfig.mode = kPowerManagerVlpw;
	
	warpPowerModeVlpsConfig = warpPowerModeVlprConfig;
	warpPowerModeVlpsConfig.mode = kPowerManagerVlps;
	
	warpPowerModeWaitConfig = warpPowerModeVlprConfig;
	warpPowerModeWaitConfig.mode = kPowerManagerWait;
	
	warpPowerModeStopConfig = warpPowerModeVlprConfig;
	warpPowerModeStopConfig.mode = kPowerManagerStop;

	warpPowerModeVlls0Config = warpPowerModeVlprConfig;
	warpPowerModeVlls0Config.mode = kPowerManagerVlls0;

	warpPowerModeVlls1Config = warpPowerModeVlprConfig;
	warpPowerModeVlls1Config.mode = kPowerManagerVlls1;

	warpPowerModeVlls3Config = warpPowerModeVlprConfig;
	warpPowerModeVlls3Config.mode = kPowerManagerVlls3;

	warpPowerModeRunConfig.mode = kPowerManagerRun;

	POWER_SYS_Init(	&powerConfigs,
			sizeof(powerConfigs)/sizeof(power_manager_user_config_t *),
			&callbacks,
			sizeof(callbacks)/sizeof(power_manager_callback_user_config_t *)
			);


	/*
	 *	Switch CPU to Very Low Power Run (VLPR) mode
	 */
	warpSetLowPowerMode(kWarpPowerModeVLPR, 0);



	/*
	 *	Initialize the GPIO pins with the appropriate pull-up, etc.,
	 *	defined in the inputPins and outputPins arrays (gpio_pins.c).
	 *
	 *	See also Section 30.3.3 GPIO Initialization of KSDK13APIRM.pdf
	 */
	GPIO_DRV_Init(inputPins  /* input pins */, outputPins  /* output pins */);
	lowPowerPinStates();



	/*
	 *	Initialize all the sensors
	 */
	initBMX055accel(0x18	/* i2cAddress */,	&deviceBMX055accelState	);
	initBMX055gyro(	0x68	/* i2cAddress */,	&deviceBMX055gyroState	);
	initBMX055mag(	0x10	/* i2cAddress */,	&deviceBMX055magState	);
	initMMA8451Q(	0x1D	/* i2cAddress */,	&deviceMMA8451QState	);	
	initMAG3110(	0x0E	/* i2cAddress */,	&deviceMAG3110State	);
	initL3GD20H(	0x6A	/* i2cAddress */,	&deviceL3GD20HState	);
	initBMP180(		0x77	/* i2cAddress */,	&deviceBMP180State	);
	initTMP006B(	0x45	/* i2cAddress */,	&deviceTMP006BState	);



	/*
	 *	Initialization: Devices hanging off SPI
	 */
	initADXL362(&deviceADXL362State);



	/*
	 *	Initialization: Devices hanging off UART
	 */
	initPAN1326B(&devicePAN1326BState);

	/*
	 *	writing the control bytes
	 */
	
	uint8_t		i2cAddress, payloadByte[1], commandByte[1];
	i2c_status_t	i2cStatus;
	WarpStatus	status;

	i2cAddress = 0x1D; /* acc addr, 7-bit */
	
	i2c_device_t slave =
	{
		.address = i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};
	
	enableSssupply(menuSupplyVoltage);
	enableI2Cpins(menuI2cPullupEnable);

	/*
	 *	Wait for supply and pull-ups to settle.
	 */
	OSA_TimeDelay(1000);

	menuRegisterAddress = 0x09; /*	reigster F_SETUP	*/ 
	commandByte[0] = menuRegisterAddress;
	payloadByte[0] = 0x00; /*	Disable FIFO	*/
		
	i2cStatus = I2C_DRV_MasterSendDataBlocking(
							0 /* I2C instance */,
							&slave,
							commandByte,
							1,
							payloadByte,
							1,
							1000);
	if (i2cStatus != kStatus_I2C_Success)
	{
		SEGGER_RTT_printf(0, "\r\n\tI2C write failed, error %d.\n\n", i2cStatus);brieflyToggleEnablingSWD();
	}
	
	menuRegisterAddress = 0x2A; /* reigster CTRL_REG1 */ 
	commandByte[0] = menuRegisterAddress;
	payloadByte[0] = 0x03;  /* Enable fast read 8bit, 800Hz, normal, active mode */
		
	i2cStatus = I2C_DRV_MasterSendDataBlocking(
							0 /* I2C instance */,
							&slave,
							commandByte,
							1,
							payloadByte,
							1,
							1000);
	if (i2cStatus != kStatus_I2C_Success)
	{
		SEGGER_RTT_printf(0, "\r\n\tI2C write failed, error %d.\n\n", i2cStatus);brieflyToggleEnablingSWD();
	}
	
	disableI2Cpins();
	/*	Essential writes complete */	

	while (1)
	{
				/*
		 *	Adapted from Warp-firmware by Youchao Wang
		 */
		
		/*
		 *	Read values from the accel sensor, the default full scale value range is 2g
		 *	and the high-pass filter is disabled
		 *	the measurement range is -2g to +1.99975g, and each count corresponds to 1g/64 (15.6mg)
		 */
		
		/*
		 *	Parameters known: mass m = 2kg, cable diameter D = 0.3m, cable length l = 1.2m, deltaT = 2e-5
		 *	density rho = 1.0kg/m^3, viscosity mu = 0.89*10^3 Pa/s (kg/m*s)
		 */
		
		double l = 1.2;
		double m = 2.0;
		double deltaT = 2e-5;
		double rho = 1.0;
		double mu = 0.00089;

		double PIOne[100];
		
		uint8_t	accX[100], accY[100], accZ[100];
		double accValueX[100], accValueY[100], accValueZ[100]; 
		double accValueSqrt[100], accValueTotal[100];  
		
		double speValueX[100], speValueY[100], speValueZ[100];
		double speValueSqrt[100], speValueTotal[100];

		double forceValue[100];
		
		/*
		 *	I2C operations
	     */

		enableI2Cpins(1);
		
		uint8_t cmdBuf[1]	= {0xFF};
		
		i2c_status_t		returnValue; /* saved for use later in debugging */

		i2c_device_t slave =
		{
			.address = deviceMMA8451QState.i2cAddress,
			.baudRate_kbps = gWarpI2cBaudRateKbps
		};

		/*	
		 *	Update the data buffer in a loop, as the first fill (100 sets of data)
		 */
	//	if(countLoops == 0)
	//	{
	//		N = 100;
	//	}
	//	else
	//	{
	//		N = 10;
	//	}
//

		for(int i = 0; i < 100; i++)
		{
			cmdBuf[0] = 0x01;
			returnValue = I2C_DRV_MasterReceiveDataBlocking(
									0 /* I2C peripheral instance */,
									&slave,
									cmdBuf,
									1,
									(uint8_t *)deviceMMA8451QState.i2cBuffer,
									1,
									500 /* timeout in milliseconds */);
			accX[i] = deviceMMA8451QState.i2cBuffer[0];
			accValueX[i] = accX[i] / 64;
									
			cmdBuf[0] = 0x03;
			returnValue = I2C_DRV_MasterReceiveDataBlocking(
									0 /* I2C peripheral instance */,
									&slave,
									cmdBuf,
									1,
									(uint8_t *)deviceMMA8451QState.i2cBuffer,
									1,
									500 /* timeout in milliseconds */);
			accY[i] = deviceMMA8451QState.i2cBuffer[0];
			accValueY[i] = accY[i] / 64;
			
			cmdBuf[0] = 0x05;
			returnValue = I2C_DRV_MasterReceiveDataBlocking(
									0 /* I2C peripheral instance */,
									&slave,
									cmdBuf,
									1,
									(uint8_t *)deviceMMA8451QState.i2cBuffer,
									1,
									500 /* timeout in milliseconds */);	
			accZ[i] = deviceMMA8451QState.i2cBuffer[0];
			accValueZ[i] = accZ[i] / 64;	/* as a proof of concept, using 8bit resolution */				
			
			accValueSqrt[i] = accValueX[i]*accValueX[i]+accValueY[i]*accValueY[i]+accValueZ[i]*accValueZ[i];
			accValueTotal[i] = sqrt(accValueSqrt[i]);
			
			speValueX[i] = m * accValueX[i] * deltaT;
			speValueY[i] = m * accValueY[i] * deltaT;
			speValueZ[i] = m * accValueZ[i] * deltaT;
			
			speValueSqrt[i] = speValueX[i]*speValueX[i]+speValueY[i]*speValueY[i]+speValueZ[i]*speValueZ[i];
			speValueTotal[i] = sqrt(speValueSqrt[i]);

			forceValue[i] = accValueTotal[i] * m;
		
	//		for(int j = 0; j < 8; j++)
	//		{
	//			switch (j) /* Might exist a better way to calculate these values */
	//			{
	//				case 0x00:
	//				{
	//					PI[i][j] = l * speValueTotal[i] * rho / mu;
	//					break;
	//				}	
	//				case 0x01:
	//				{	
	//					PI[i][j] = l * l * speValueTotal[i] * speValueTotal[i] * rho / forceValue[i];
	//					break;
	//				}
	//				case 0x02:
	//				{	
	//					PI[i][j] = rho * mu * forceValue[i];
	//					break;	
	//				}
	//				case 0x03:
	//				{	
	//					PI[i][j] = l * speValueTotal[i] * rho / mu;
	//					break;
	//				}
	//				case 0x04:
	//				{	
	//					PI[i][j] = l * l * speValueTotal[i] * speValueTotal[i] * rho / forceValue[i];
	//					break;
	//				}
	//				case 0x05:
	//				{	
	//					PI[i][j] = rho * mu * forceValue[i];
	//					break;							
	//				}
	//				case 0x06:
	//				{	
	//					PI[i][j] = l * speValueTotal[i] * rho / mu;
	//					break;
	//				}
	//				case 0x07:
	//				{	
	//					PI[i][j] = l * l * speValueTotal[i] * speValueTotal[i] * rho / forceValue[i];
	//					break;
	//				}
	//				default:
	//				{
	//					break;
	//				}
	//			} /* end of switch */

	//			SEGGER_RTT_printf(0, "%f,",PI[i][j]);brieflyToggleEnablingSWD();

	//		}
		
			
			SEGGER_RTT_printf(0,"//(%d)::://",i);brieflyToggleEnablingSWD();

		}
		
		SEGGER_RTT_printf(0,"\n");brieflyToggleEnablingSWD();

		/*
		 *	TODO in While(1), read the values continuously, and update the buffer FIFO or depending on a specific algorithm
		 */
		
	//	if(countLoops == 250)
	//	{
	//		countLoops = 2;
	//	}
	}

	return 0;
}



void
repeatRegisterReadForDeviceAndAddress(WarpSensorDevice warpSensorDevice, uint8_t baseAddress, bool pullupEnable, bool autoIncrement, int chunkReadsPerAddress, bool chatty, int spinDelay, int repetitionsPerAddress, uint16_t sssupplyMillivolts, uint16_t adaptiveSssupplyMaxMillivolts, uint8_t referenceByte)
{
	WarpStatus		status;
	uint8_t			address = baseAddress;
	int			readCount = repetitionsPerAddress + 1;
	int			nSuccesses = 0;
	int			nFailures = 0;
	int			nCorrects = 0;
	int			nBadCommands = 0;
	uint16_t		actualSssupplyMillivolts = sssupplyMillivolts;
	uint16_t		voltageTrace[readCount];


	enableSssupply(actualSssupplyMillivolts);
	OSA_TimeDelay(100);

	if (warpSensorDevice != kWarpSensorADXL362)
	{
		enableI2Cpins(pullupEnable);
	}

	switch (warpSensorDevice)
	{
		case kWarpSensorADXL362:
		{
			/*
			 *	ADXL362: VDD 1.6--3.5
			 */
			SEGGER_RTT_WriteString(0, "\r\nADXL362:\n\r");brieflyToggleEnablingSWD();
			ADXL362loop: if (address < 0x2E)
			{
				for (int i = 0; i < readCount; i++) for (int j = 0; j < chunkReadsPerAddress; j++)
				{
					voltageTrace[i] = actualSssupplyMillivolts;
					status = readSensorRegisterADXL362(address+j);
					if (status == kWarpStatusOK)
					{
						nSuccesses++;
						if (actualSssupplyMillivolts > sssupplyMillivolts)
						{
							actualSssupplyMillivolts -= 100;
							enableSssupply(actualSssupplyMillivolts);
						}

						if (referenceByte == deviceADXL362State.spiSinkBuffer[2])
						{
							nCorrects++;
						}

						if (chatty)
						{
							SEGGER_RTT_printf(0, "\r0x%02x --> [0x%02x 0x%02x 0x%02x]\n",
								address+j,
								deviceADXL362State.spiSinkBuffer[0],
								deviceADXL362State.spiSinkBuffer[1],
								deviceADXL362State.spiSinkBuffer[2]);brieflyToggleEnablingSWD();
						}
					}
					else if (status == kWarpStatusDeviceCommunicationFailed)
					{
						SEGGER_RTT_printf(0, "\r0x%02x --> ----\n",
							address+j);brieflyToggleEnablingSWD();			
						
						nFailures++;
						if (actualSssupplyMillivolts < adaptiveSssupplyMaxMillivolts)
						{
							actualSssupplyMillivolts += 100;
							enableSssupply(actualSssupplyMillivolts);
						}
					}
					else if (status == kWarpStatusBadDeviceCommand)
					{
						nBadCommands++;
					}

					if (spinDelay > 0) OSA_TimeDelay(spinDelay);
				}

				if (autoIncrement)
				{
					address++;
					goto ADXL362loop;
				}
			}
			break;
		}

		case kWarpSensorMMA8451Q:
		{
			/*
			 *	MMA8451Q: VDD 1.95--3.6
			 */
			SEGGER_RTT_WriteString(0, "\r\nMMA8451Q:\n\r");brieflyToggleEnablingSWD();
			MMA8451Qloop: if (address < 0x32)
			{
				for (int i = 0; i < readCount; i++) for (int j = 0; j < chunkReadsPerAddress; j++)
				{
					voltageTrace[i] = actualSssupplyMillivolts;
					status = readSensorRegisterMMA8451Q(address+j);
					if (status == kWarpStatusOK)
					{
						nSuccesses++;
						if (actualSssupplyMillivolts > sssupplyMillivolts)
						{
							actualSssupplyMillivolts -= 100;
							enableSssupply(actualSssupplyMillivolts);
						}

						if (referenceByte == deviceMMA8451QState.i2cBuffer[0])
						{
							nCorrects++;
						}


						if (chatty)
						{
							SEGGER_RTT_printf(0, "\r0x%02x --> 0x%02x\n",
								address+j,
								deviceMMA8451QState.i2cBuffer[0]);brieflyToggleEnablingSWD();			
						}
					}
					else if (status == kWarpStatusDeviceCommunicationFailed)
					{
						SEGGER_RTT_printf(0, "\r0x%02x --> ----\n",
							address+j);brieflyToggleEnablingSWD();

						nFailures++;
						if (actualSssupplyMillivolts < adaptiveSssupplyMaxMillivolts)
						{
							actualSssupplyMillivolts += 100;
							enableSssupply(actualSssupplyMillivolts);
						}
					}
					else if (status == kWarpStatusBadDeviceCommand)
					{
						nBadCommands++;
					}

					if (spinDelay > 0) OSA_TimeDelay(spinDelay);
				}

				if (autoIncrement)
				{
					address++;
					goto MMA8451Qloop;
				}
			}

			break;
		}

		case kWarpSensorBMP180:
		{
			/*
			 *	BMP180: VDD 1.6--3.6
			 */
			SEGGER_RTT_WriteString(0, "\r\nBMP180:\n\r");brieflyToggleEnablingSWD();
			BMP180loop: if (address >= 0xAA && address <= 0xF8)
			{
				for (int i = 0; i < readCount; i++) for (int j = 0; j < chunkReadsPerAddress; j++)
				{
					voltageTrace[i] = actualSssupplyMillivolts;
					status = readSensorRegisterBMP180(address+j);
					if (status == kWarpStatusOK)
					{
						nSuccesses++;
						if (actualSssupplyMillivolts > sssupplyMillivolts)
						{
							actualSssupplyMillivolts -= 100;
							enableSssupply(actualSssupplyMillivolts);
						}

						if (referenceByte == deviceBMP180State.i2cBuffer[0])
						{
							nCorrects++;
						}


						if (chatty)
						{
							SEGGER_RTT_printf(0, "\r0x%02x --> 0x%02x\n",
								address+j,
								deviceBMP180State.i2cBuffer[0]);brieflyToggleEnablingSWD();
						}
					}
					else if (status == kWarpStatusDeviceCommunicationFailed)
					{
						SEGGER_RTT_printf(0, "\r0x%02x --> ----\n",
							address+j);brieflyToggleEnablingSWD();

						nFailures++;
						if (actualSssupplyMillivolts < adaptiveSssupplyMaxMillivolts)
						{
							actualSssupplyMillivolts += 100;
							enableSssupply(actualSssupplyMillivolts);
						}
					}
					else if (status == kWarpStatusBadDeviceCommand)
					{
						nBadCommands++;
					}

					if (spinDelay > 0) OSA_TimeDelay(spinDelay);
				}

				if (autoIncrement)
				{
					address++;
					goto BMP180loop;
				}
			}

			break;
		}

		case kWarpSensorBMX055accel:
		{
			/*
			 *	BMX055accel: VDD 2.4V -- 3.6V
			 */
			SEGGER_RTT_WriteString(0, "\r\nBMX055accel:\n\r");brieflyToggleEnablingSWD();
			BMX055accelloop: if (address < 0x40)
			{
				for (int i = 0; i < readCount; i++) for (int j = 0; j < chunkReadsPerAddress; j++)
				{
					voltageTrace[i] = actualSssupplyMillivolts;
					status = readSensorRegisterBMX055accel(address+j);
					if (status == kWarpStatusOK)
					{
						nSuccesses++;
						if (actualSssupplyMillivolts > sssupplyMillivolts)
						{
							actualSssupplyMillivolts -= 100;
							enableSssupply(actualSssupplyMillivolts);
						}

						if (referenceByte == deviceBMX055accelState.i2cBuffer[0])
						{
							nCorrects++;
						}


						if (chatty)
						{
							SEGGER_RTT_printf(0, "\r0x%02x --> 0x%02x\n",
								address+j,
								deviceBMX055accelState.i2cBuffer[0]);brieflyToggleEnablingSWD();
						}
					}
					else if (status == kWarpStatusDeviceCommunicationFailed)
					{
						SEGGER_RTT_printf(0, "\r0x%02x --> ----\n",
							address+j);brieflyToggleEnablingSWD();

						nFailures++;
						if (actualSssupplyMillivolts < adaptiveSssupplyMaxMillivolts)
						{
							actualSssupplyMillivolts += 100;
							enableSssupply(actualSssupplyMillivolts);
						}
					}
					else if (status == kWarpStatusBadDeviceCommand)
					{
						nBadCommands++;
					}

					if (spinDelay > 0) OSA_TimeDelay(spinDelay);
				}

				if (autoIncrement)
				{
					address++;
					goto BMX055accelloop;
				}
			}

			break;
		}

		case kWarpSensorBMX055gyro:
		{
			/*
			 *	BMX055gyro: VDD 2.4V -- 3.6V
			 */
			SEGGER_RTT_WriteString(0, "\r\nBMX055gyro:\n\r");brieflyToggleEnablingSWD();
			BMX055gyroloop: if (address < 0x40)
			{
				for (int i = 0; i < readCount; i++) for (int j = 0; j < chunkReadsPerAddress; j++)
				{
					voltageTrace[i] = actualSssupplyMillivolts;
					status = readSensorRegisterBMX055gyro(address+j);
					if (status == kWarpStatusOK)
					{
						nSuccesses++;
						if (actualSssupplyMillivolts > sssupplyMillivolts)
						{
							actualSssupplyMillivolts -= 100;
							enableSssupply(actualSssupplyMillivolts);
						}

						if (referenceByte == deviceBMX055gyroState.i2cBuffer[0])
						{
							nCorrects++;
						}


						if (chatty)
						{
							SEGGER_RTT_printf(0, "\r0x%02x --> 0x%02x\n",
								address+j,
								deviceBMX055gyroState.i2cBuffer[0]);brieflyToggleEnablingSWD();
						}
					}
					else if (status == kWarpStatusDeviceCommunicationFailed)
					{
						SEGGER_RTT_printf(0, "\r0x%02x --> ----\n",
							address+j);brieflyToggleEnablingSWD();

						nFailures++;
						if (actualSssupplyMillivolts < adaptiveSssupplyMaxMillivolts)
						{
							actualSssupplyMillivolts += 100;
							enableSssupply(actualSssupplyMillivolts);
						}
					}
					else if (status == kWarpStatusBadDeviceCommand)
					{
						nBadCommands++;
					}

					if (spinDelay > 0) OSA_TimeDelay(spinDelay);
				}

				if (autoIncrement)
				{
					address++;
					goto BMX055gyroloop;
				}
			}

			break;
		}

		case kWarpSensorBMX055mag:
		{
			/*
			 *	BMX055mag: VDD 2.4V -- 3.6V
			 */
			/*
			SEGGER_RTT_WriteString(0, "\r\nBMX055mag:\n\r");brieflyToggleEnablingSWD();
			BMX055magloop: if (address >= 0x40 && address < 0x53)
			{
				for (int i = 0; i < readCount; i++) for (int j = 0; j < chunkReadsPerAddress; j++)
				{
					voltageTrace[i] = actualSssupplyMillivolts;
					status = readSensorRegisterBMX055mag(address+j);
					if (status == kWarpStatusOK)
					{
						nSuccesses++;
						if (actualSssupplyMillivolts > sssupplyMillivolts)
						{
							actualSssupplyMillivolts -= 100;
							enableSssupply(actualSssupplyMillivolts);
						}

						if (referenceByte == deviceBMX055magState.i2cBuffer[0])
						{
							nCorrects++;
						}


						if (chatty)
						{
							SEGGER_RTT_printf(0, "\r0x%02x --> 0x%02x\n",
								address+j,
								deviceBMX055magState.i2cBuffer[0]);brieflyToggleEnablingSWD();
						}
					}
					else if (status == kWarpStatusDeviceCommunicationFailed)
					{
						SEGGER_RTT_printf(0, "\r0x%02x --> ----\n",
							address+j);brieflyToggleEnablingSWD();

						nFailures++;
						if (actualSssupplyMillivolts < adaptiveSssupplyMaxMillivolts)
						{
							actualSssupplyMillivolts += 100;
							enableSssupply(actualSssupplyMillivolts);
						}
					}
					else if (status == kWarpStatusBadDeviceCommand)
					{
						nBadCommands++;
					}

					if (spinDelay > 0) OSA_TimeDelay(spinDelay);
				}

				if (autoIncrement)
				{
					address++;
					goto BMX055magloop;
				}
			}
			*/

			break;
		}

		case kWarpSensorTMP006B:
		{
			/*
			 *	TMP006B: VDD 2.2V
			 */
			SEGGER_RTT_WriteString(0, "\r\nTMP006B:\n\r");brieflyToggleEnablingSWD();
			TMP006Bloop1: if (address <= 0x02)
			{
				for (int i = 0; i < readCount; i++) for (int j = 0; j < chunkReadsPerAddress; j++)
				{
					voltageTrace[i] = actualSssupplyMillivolts;
					status = readSensorRegisterTMP006B(address+j);
					if (status == kWarpStatusOK)
					{
						nSuccesses++;
						if (actualSssupplyMillivolts > sssupplyMillivolts)
						{
							actualSssupplyMillivolts -= 100;
							enableSssupply(actualSssupplyMillivolts);
						}

						/*
						 *	NOTE: Unlike for other snesors, we compare the reference 
						 *	byte to i2cBuffer[1], not i2cBuffer[0].
						 */
						if (referenceByte == deviceTMP006BState.i2cBuffer[1])
						{
							nCorrects++;
						}


						if (chatty)
						{
							SEGGER_RTT_printf(0, "\r0x%02x --> [0x%02x 0x%02x]\n",
								address,
								deviceTMP006BState.i2cBuffer[0],
								deviceTMP006BState.i2cBuffer[1]);brieflyToggleEnablingSWD();
						}
					}
					else if (status == kWarpStatusDeviceCommunicationFailed)
					{
						nFailures++;
						if (actualSssupplyMillivolts < adaptiveSssupplyMaxMillivolts)
						{
							actualSssupplyMillivolts += 100;
							enableSssupply(actualSssupplyMillivolts);
						}
					}
					else if (status == kWarpStatusBadDeviceCommand)
					{
						nBadCommands++;
					}

					if (spinDelay > 0) OSA_TimeDelay(spinDelay);
				}

				if (autoIncrement)
				{
					address++;
					if (address > 0x02) address = 0xFE;
					goto TMP006Bloop1;
				}
			}
			TMP006Bloop2: if (address >= 0xFE && address <= 0xFF)
			{
				for (int i = 0; i < readCount; i++) for (int j = 0; j < chunkReadsPerAddress; j++)
				{
					voltageTrace[i] = actualSssupplyMillivolts;
					status = readSensorRegisterTMP006B(address+j);
					if (status == kWarpStatusOK)
					{
						nSuccesses++;
						if (actualSssupplyMillivolts > sssupplyMillivolts)
						{
							actualSssupplyMillivolts -= 100;
							enableSssupply(actualSssupplyMillivolts);
						}

						/*
						 *	NOTE: Unlike for other snesors, we compare the reference 
						 *	byte to i2cBuffer[1], not i2cBuffer[0].
						 */
						if (referenceByte == deviceTMP006BState.i2cBuffer[1])
						{
							nCorrects++;
						}

						if (chatty)
						{
							SEGGER_RTT_printf(0, "\r0x%02x --> [0x%02x 0x%02x]\n",
								address,
								deviceTMP006BState.i2cBuffer[0],
								deviceTMP006BState.i2cBuffer[1]);brieflyToggleEnablingSWD();
						}
					}
					else if (status == kWarpStatusDeviceCommunicationFailed)
					{
						nFailures++;
						if (actualSssupplyMillivolts < adaptiveSssupplyMaxMillivolts)
						{
							actualSssupplyMillivolts += 100;
							enableSssupply(actualSssupplyMillivolts);
						}
					}
					else if (status == kWarpStatusBadDeviceCommand)
					{
						nBadCommands++;
					}

					if (spinDelay > 0) OSA_TimeDelay(spinDelay);
				}

				if (autoIncrement)
				{
					address++;
					goto TMP006Bloop2;
				}
			}

			break;
		}

		case kWarpSensorMAG3110:
		{
			/*
			 *	MAG3110: VDD 1.95 -- 3.6
			 */
			SEGGER_RTT_WriteString(0, "\r\nMAG3110:\n\r");brieflyToggleEnablingSWD();
			MAG3110loop: if (address < 0x12)
			{
				for (int i = 0; i < readCount; i++) for (int j = 0; j < chunkReadsPerAddress; j++)
				{
					voltageTrace[i] = actualSssupplyMillivolts;
					status = readSensorRegisterMAG3110(address+j);
					if (status == kWarpStatusOK)
					{
						nSuccesses++;
						if (actualSssupplyMillivolts > sssupplyMillivolts)
						{
							actualSssupplyMillivolts -= 100;
							enableSssupply(actualSssupplyMillivolts);
						}

						if (referenceByte == deviceMAG3110State.i2cBuffer[0])
						{
							nCorrects++;
						}


						if (chatty)
						{
							SEGGER_RTT_printf(0, "\r0x%02x --> 0x%02x\n",
								address+j,
								deviceMAG3110State.i2cBuffer[0]);brieflyToggleEnablingSWD();
						}
					}
					else if (status == kWarpStatusDeviceCommunicationFailed)
					{
						SEGGER_RTT_printf(0, "\r0x%02x --> ----\n",
							address+j);brieflyToggleEnablingSWD();
						
						nFailures++;
						if (actualSssupplyMillivolts < adaptiveSssupplyMaxMillivolts)
						{
							actualSssupplyMillivolts += 100;
							enableSssupply(actualSssupplyMillivolts);
						}
					}
					else if (status == kWarpStatusBadDeviceCommand)
					{
						nBadCommands++;
					}

					if (spinDelay > 0) OSA_TimeDelay(spinDelay);
				}

				if (autoIncrement)
				{
					address++;
					goto MAG3110loop;
				}
			}

			break;
		}

		case kWarpSensorL3GD20H:
		{
			/*
			 *	L3GD20H: VDD 2.2V -- 3.6V
			 */
			SEGGER_RTT_WriteString(0, "\r\nL3GD20H:\n\r");brieflyToggleEnablingSWD();
			L3GD20Hloop: if (address >= 0x0F && address <= 0x39)
			{
				for (int i = 0; i < readCount; i++) for (int j = 0; j < chunkReadsPerAddress; j++)
				{
					voltageTrace[i] = actualSssupplyMillivolts;
					status = readSensorRegisterL3GD20H(address+j);
					if (status == kWarpStatusOK)
					{
						nSuccesses++;
						if (actualSssupplyMillivolts > sssupplyMillivolts)
						{
							actualSssupplyMillivolts -= 100;
							enableSssupply(actualSssupplyMillivolts);
						}

						if (referenceByte == deviceL3GD20HState.i2cBuffer[0])
						{
							nCorrects++;
						}


						if (chatty)
						{
							SEGGER_RTT_printf(0, "\r0x%02x --> 0x%02x\n",
								address+j,
								deviceL3GD20HState.i2cBuffer[0]);brieflyToggleEnablingSWD();
						}
					}
					else if (status == kWarpStatusDeviceCommunicationFailed)
					{
						SEGGER_RTT_printf(0, "\r0x%02x --> ----\n",
							address+j);brieflyToggleEnablingSWD();

						nFailures++;
						if (actualSssupplyMillivolts < adaptiveSssupplyMaxMillivolts)
						{
							actualSssupplyMillivolts += 100;
							enableSssupply(actualSssupplyMillivolts);
						}
					}
					else if (status == kWarpStatusBadDeviceCommand)
					{
						nBadCommands++;
					}

					if (spinDelay > 0) OSA_TimeDelay(spinDelay);
				}

				if (autoIncrement)
				{
					address++;
					goto L3GD20Hloop;
				}
			}

			break;
		}

		default:
		{
			SEGGER_RTT_printf(0, "\r\tInvalid warpSensorDevice [%d] passed to repeatRegisterReadForDeviceAndAddress.\n", warpSensorDevice);brieflyToggleEnablingSWD();
		}
	}

	if (warpSensorDevice != kWarpSensorADXL362)
	{
		disableI2Cpins();
	}

	/*
	 *	To make printing of stats robust, we switch to VLPR (assuming we are not already in VLPR).
	 *
	 *	As of circa issue-58 implementation, RTT printing when in RUN mode was flaky (achievable SWD speed too slow for buffer fill rate?)
	 */
	warpSetLowPowerMode(kWarpPowerModeVLPR, 0 /* sleep seconds : irrelevant here */);

	SEGGER_RTT_printf(0, "\r\n\t%d/%d success rate.\n", nSuccesses, (nSuccesses + nFailures));brieflyToggleEnablingSWD();
	SEGGER_RTT_printf(0, "\r\t%d/%d successes matched ref. value of 0x%02x.\n", nCorrects, nSuccesses, referenceByte);brieflyToggleEnablingSWD();
	SEGGER_RTT_printf(0, "\r\t%d bad commands.\n\n", nBadCommands);brieflyToggleEnablingSWD();
	SEGGER_RTT_printf(0, "\r\tVoltage trace:\n", nBadCommands);brieflyToggleEnablingSWD();

	for (int i = 0; i < readCount; i++)
	{
		SEGGER_RTT_printf(0, "\r\t\t%d\t%d\n", i, voltageTrace[i]);

		/*
		 *	To give debug interface time to catch up with the prints.
		 *
		 *	Even when we force CPU into VLPR mode, we need a longer delay
		 *	than the 10ms that we now use in brieflyToggleEnablingSWD(),
		 *	since RTT buffer will be filling quickly.
		 */		
		enableSWDDisablePTA1x2x3xGpio();
		OSA_TimeDelay(50);
		disableSWDEnablePTA1x2x3xGpio();
	}
}



int
char2int(int character)
{
	if (character >= '0' && character <= '9')
	{
		return character - '0';
	}

	if (character >= 'a' && character <= 'f')
	{
		return character - 'a' + 10;
	}

	if (character >= 'A' && character <= 'F')
	{
		return character - 'A' + 10;
	}

	return 0;
}



uint8_t
readHexByte(void)
{
	uint8_t		topNybble, bottomNybble;

	enableSWDDisablePTA1x2x3xGpio();
	topNybble = SEGGER_RTT_WaitKey();
	bottomNybble = SEGGER_RTT_WaitKey();
	disableSWDEnablePTA1x2x3xGpio();

	return (char2int(topNybble) << 4) + char2int(bottomNybble);
}



int
read4digits(void)
{
	uint8_t		digit1, digit2, digit3, digit4;
	
	enableSWDDisablePTA1x2x3xGpio();
	digit1 = SEGGER_RTT_WaitKey();
	digit2 = SEGGER_RTT_WaitKey();
	digit3 = SEGGER_RTT_WaitKey();
	digit4 = SEGGER_RTT_WaitKey();
	disableSWDEnablePTA1x2x3xGpio();

	return (digit1 - '0')*1000 + (digit2 - '0')*100 + (digit3 - '0')*10 + (digit4 - '0');
}



WarpStatus
writeByteToI2cDeviceRegister(uint8_t i2cAddress, bool sendCommandByte, uint8_t commandByte, bool sendPayloadByte, uint8_t payloadByte)
{
	i2c_status_t	status;
	uint8_t		commandBuffer[1];
	uint8_t		payloadBuffer[1];
	i2c_device_t	i2cSlaveConfig =
			{
				.address = i2cAddress,
				.baudRate_kbps = gWarpI2cBaudRateKbps
			};

	commandBuffer[0] = commandByte;
	payloadBuffer[0] = payloadByte;
	enableI2Cpins(true /* pullupEnable*/);
	status = I2C_DRV_MasterSendDataBlocking(
						0	/* instance */,
						&i2cSlaveConfig,
						commandBuffer,
						(sendCommandByte ? 1 : 0),
						payloadBuffer,
						(sendPayloadByte ? 1 : 0),
						1000	/* timeout in milliseconds */);
	disableI2Cpins();

	return (status == kStatus_I2C_Success ? kWarpStatusOK : kWarpStatusDeviceCommunicationFailed);
}



WarpStatus
writeBytesToSpi(uint8_t *  payloadBytes, int payloadLength, bool driveI2cPinsHighToMatchSupply, bool driveI2cPinsLow)
{
	uint8_t		inBuffer[payloadLength];
	spi_status_t	status;
	
	enableSPIpins(driveI2cPinsHighToMatchSupply);
	status = SPI_DRV_MasterTransferBlocking(0		/* master instance */,
						NULL		/* spi_master_user_config_t */,
						payloadBytes,
						inBuffer,
						payloadLength	/* transfer size */,
						1000		/* timeout in microseconds (unlike I2C which is ms) */);
	disableSPIpins(driveI2cPinsLow);

	return (status == kStatus_SPI_Success ? kWarpStatusOK : kWarpStatusCommsError);
}



void
powerupAllSensors(void)
{
	WarpStatus	status;

	/*
	 *	BMX055mag
	 *
	 *	Write '1' to power control bit of register 0x4B. See page 134.
	 */
	status = writeByteToI2cDeviceRegister(	deviceBMX055magState.i2cAddress		/*	i2cAddress		*/,
						true					/*	sendCommandByte		*/,
						0x4B					/*	commandByte		*/,
						true					/*	sendPayloadByte		*/,
						(1 << 0)				/*	payloadByte		*/);
	if (status != kWarpStatusOK)
	{
		SEGGER_RTT_printf(0, "\r\tPowerup command failed, code=%d, for BMX055mag @ 0x%02x.\n", status, deviceBMX055magState.i2cAddress);brieflyToggleEnablingSWD();
	}
}



void
activateAllLowPowerSensorModes(void)
{
	WarpStatus	status;



	/*
	 *	ADXL362:	See Power Control Register (Address: 0x2D, Reset: 0x00).
	 *
	 *	POR values are OK.
	 */



	/*
	 *	BMX055accel: At POR, device is in Normal mode. Move it to Deep Suspend mode.
	 *
	 *	Write '1' to deep suspend bit of register 0x11, and write '0' to suspend bit of register 0x11. See page 23.
	 */
	status = writeByteToI2cDeviceRegister(	deviceBMX055accelState.i2cAddress	/*	i2cAddress		*/,
						true					/*	sendCommandByte		*/,
						0x11					/*	commandByte		*/,
						true					/*	sendPayloadByte		*/,
						(1 << 5)				/*	payloadByte		*/);
	if (status != kWarpStatusOK)
	{
		SEGGER_RTT_printf(0, "\r\tPowerdown command failed, code=%d, for BMX055accel @ 0x%02x.\n", status, deviceBMX055accelState.i2cAddress);brieflyToggleEnablingSWD();
	}


	/*
	 *	BMX055gyro: At POR, device is in Normal mode. Move it to Deep Suspend mode.
	 *
	 *	Write '1' to deep suspend bit of register 0x11. See page 81.
	 */
	status = writeByteToI2cDeviceRegister(	deviceBMX055gyroState.i2cAddress	/*	i2cAddress		*/,
						true					/*	sendCommandByte		*/,
						0x11					/*	commandByte		*/,
						true					/*	sendPayloadByte		*/,
						(1 << 5)				/*	payloadByte		*/);
	if (status != kWarpStatusOK)
	{
		SEGGER_RTT_printf(0, "\r\tPowerdown command failed, code=%d, for BMX055gyro @ 0x%02x.\n", status, deviceBMX055gyroState.i2cAddress);brieflyToggleEnablingSWD();
	}



	/*
	 *	L3GD20H: See CTRL1 at 0x20 (page 36).
	 *
	 *	POR state seems to be powered down.
	 */
	status = writeByteToI2cDeviceRegister(	deviceL3GD20HState.i2cAddress	/*	i2cAddress		*/,
						true				/*	sendCommandByte		*/,
						0x20				/*	commandByte		*/,
						true				/*	sendPayloadByte		*/,
						0x00				/*	payloadByte		*/);
	if (status != kWarpStatusOK)
	{
		SEGGER_RTT_printf(0, "\r\tPowerdown command failed, code=%d, for L3GD20H @ 0x%02x.\n", status, deviceL3GD20HState.i2cAddress);brieflyToggleEnablingSWD();
	}



	/*
	 *	TMP006B: Configuration Register at address 0x02. At POR, is in "Sensor and die continuous conversion" mode.
	 *
	 *	Set config register to 0x00 (see page 20).
	 */
	status = writeByteToI2cDeviceRegister(	deviceTMP006BState.i2cAddress	/*	i2cAddress		*/,
						true				/*	sendCommandByte		*/,
						0x02				/*	commandByte		*/,
						true				/*	sendPayloadByte		*/,
						0x00				/*	payloadByte		*/);
	if (status != kWarpStatusOK)
	{
		SEGGER_RTT_printf(0, "\r\tPowerdown command failed, code=%d, for TMP006B @ 0x%02x.\n", status, deviceTMP006BState.i2cAddress);brieflyToggleEnablingSWD();
	}


	/*
	 *	PAN1326.
	 *
	 *	For now, simply hold its reset line low.
	 */
	GPIO_DRV_ClearPinOutput(kWarpPinADXL362_CS_PAN1326_nSHUTD);
}