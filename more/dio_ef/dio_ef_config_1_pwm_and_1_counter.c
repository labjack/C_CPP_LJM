/**
 * Name: dio_ef_config_1_pwm_and_1_counter.c
 * Desc: Enables a 10 kHz PWM output and high speed counter, waits 1 second and
 *       reads the counter. If you jumper the counter to PWM, it should return
 *       around 10000.
 *
 * Relevant Documentation:
 *
 * LJM Library:
 *	LJM Library Installer:
 *		https://labjack.com/support/software/installers/ljm
 *	LJM Users Guide:
 *		https://labjack.com/support/software/api/ljm
 *	Opening and Closing:
 *		https://labjack.com/support/software/api/ljm/function-reference/opening-and-closing
 *	Multiple Value Functions(such as eWriteNames):
 *		https://labjack.com/support/software/api/ljm/function-reference/multiple-value-functions
 *
 * T-Series and I/O:
 *	Modbus Map:
 *		https://labjack.com/support/software/api/modbus/modbus-map
 *	Digital I/O:
 *		https://labjack.com/support/datasheets/t-series/digital-io
 *	Extended DIO Features:
 *		https://labjack.com/support/datasheets/t-series/digital-io/extended-features
 *	PWM Out:
 *		https://labjack.com/support/datasheets/t-series/digital-io/extended-features/pwm-out
 *	High-Speed Counter:
 *		https://labjack.com/support/datasheets/t-series/digital-io/extended-features/high-speed-counter
**/

// For printf
#include <stdio.h>

// For the LabJackM Library
#include <LabJackM.h>

// For LabJackM helper functions, such as OpenOrDie, PrintDeviceInfoFromHandle,
// ErrorCheck, etc.
#include "../../LJM_Utilities.h"

void dio_ef_pwm_and_counter(int handle);

int main()
{
	int handle;

	// Open first found LabJack
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
	// handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	PrintDeviceInfoFromHandle(handle);

	dio_ef_pwm_and_counter(handle);

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}

void dio_ef_pwm_and_counter(int handle)
{
	int err;
	int errAddress = INITIAL_ERR_ADDRESS;
	int pwmDIO, counterDIO;
	char DIO_EF_INDEX_PLACEHOLDER[LJM_MAX_NAME_SIZE];
	char DIO_EF_CONFIG_A_PLACEHOLDER[LJM_MAX_NAME_SIZE];
	char DIO_EF_ENABLE_PLACEHOLDER[LJM_MAX_NAME_SIZE];
	char COUNTER_INDEX_PLACEHOLDER[LJM_MAX_NAME_SIZE];
	char COUNTER_ENABLE_PLACEHOLDER[LJM_MAX_NAME_SIZE];
	char COUNTER_READ_PLACEHOLDER[LJM_MAX_NAME_SIZE];

	// Set up for reading DIO state
	enum { NUM_FRAMES_CONFIGURE = 9 };
	const char * aNamesConfigure[NUM_FRAMES_CONFIGURE] = {
		"DIO_EF_CLOCK0_DIVISOR",
		"DIO_EF_CLOCK0_ROLL_VALUE",
		"DIO_EF_CLOCK0_ENABLE",
		DIO_EF_INDEX_PLACEHOLDER,
		DIO_EF_CONFIG_A_PLACEHOLDER,
		DIO_EF_ENABLE_PLACEHOLDER,
		COUNTER_ENABLE_PLACEHOLDER,
		COUNTER_INDEX_PLACEHOLDER,
		COUNTER_ENABLE_PLACEHOLDER
	};
	double aValuesConfigure[NUM_FRAMES_CONFIGURE] = {
		1,
		8000,
		1,
		0,
		2000,
		1,
		0,
		7,
		1
	};

	// Set up for turning off PWM output and counter
	enum { NUM_FRAMES_DISABLE = 3 };
	const char * aNamesDisable[NUM_FRAMES_DISABLE] = {
		"DIO_EF_CLOCK0_ENABLE",
		DIO_EF_ENABLE_PLACEHOLDER,
		COUNTER_ENABLE_PLACEHOLDER
	};
	double aValuesDisable[NUM_FRAMES_DISABLE] = {
		0,
		0,
		0
	};

	// Configure the PWM output and counter.
	switch(GetDeviceType(handle)){
		// For the T4, use FIO6 (DIO6) for the PWM output
		// Use CIO2 (DIO18) for the high speed counter
		case LJM_dtT4:
			pwmDIO = 6;
			counterDIO = 18;
			break;
		// For the T7, use FIO0 (DIO0) for the PWM output
		// Use CIO2 (DIO18) for the high speed counter
		case LJM_dtT7:
			pwmDIO = 0;
			counterDIO = 18;
			break;
		// For the T8, use FIO7 (DIO7) for the PWM output
		// Use FIO6 (DIO6) for the high speed counter
		case LJM_dtT8:
			pwmDIO = 7;
			counterDIO = 6;
			break;
	};
	sprintf(DIO_EF_INDEX_PLACEHOLDER, "DIO%d_EF_INDEX", pwmDIO);
	sprintf(DIO_EF_CONFIG_A_PLACEHOLDER, "DIO%d_EF_CONFIG_A", pwmDIO);
	sprintf(DIO_EF_ENABLE_PLACEHOLDER, "DIO%d_EF_ENABLE", pwmDIO);
	sprintf(COUNTER_INDEX_PLACEHOLDER, "DIO%d_EF_INDEX", counterDIO);
	sprintf(COUNTER_ENABLE_PLACEHOLDER, "DIO%d_EF_ENABLE", counterDIO);
	sprintf(COUNTER_READ_PLACEHOLDER, "DIO%d_EF_READ_A", counterDIO);

	err = LJM_eWriteNames(handle, NUM_FRAMES_CONFIGURE, aNamesConfigure,
		aValuesConfigure, &errAddress);
	ErrorCheckWithAddress(err, errAddress, "LJM_eWriteNames - aNamesConfigure");

	// Wait 1 second.
	MillisecondSleep(1000);

	// Read from the counter.
	printf("\nCounter - ");
	GetAndPrint(handle, COUNTER_READ_PLACEHOLDER);

	// Turn off PWM output and counter
	err = LJM_eWriteNames(handle, NUM_FRAMES_DISABLE, aNamesDisable,
		aValuesDisable, &errAddress);
	ErrorCheckWithAddress(err, errAddress, "LJM_eWriteNames - aNamesDisable");
}
