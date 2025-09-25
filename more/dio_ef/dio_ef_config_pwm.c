/**
 * Name: dio_ef_config_pwm.c
 * Desc: Enables a 10 kHz PWM output for 10 seconds.
 *
 * To configure PWM to user desired frequency and duty cycle, modify the
 * "desiredFrequency" and "desiredDutyCycle" variables.
 *
 * For more information on the PWM DIO_EF mode see section 13.2.2 of the T-Series
 * Datasheet:
 * https://support.labjack.com/docs/13-2-2-pwm-out-t-series-datasheet
**/

// For printf
#include <stdio.h>

// For the LabJackM Library
#include <LabJackM.h>

// For LabJackM helper functions, such as OpenOrDie, PrintDeviceInfoFromHandle,
// ErrorCheck, etc.
#include "../../LJM_Utilities.h"

void dio_ef_config_pwm(int handle);

int main()
{
	int handle;

	// Open first found LabJack
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
	// handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	PrintDeviceInfoFromHandle(handle);

	dio_ef_config_pwm(handle);

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}

void dio_ef_config_pwm(int handle)
{
	// ------------- USER INPUT VALUES -------------
	double desiredFrequency = 10000;  // Set this value to your desired PWM Frequency Hz. Defualt 10000 Hz
	double desiredDutyCycle = 50;     // Set this value to your desired PWM Duty Cycle percentage. Default 50%
	// ---------------------------------------------

	int deviceType = 0;
	double clockTickRate = 0;
	double clockRollValue = 0;
	double pwmConfigA = 0;
	int pwmDIO = 0;
	double coreFrequency = 0;
	double clockDivisor = 0;
	char nameStr[LJM_MAX_NAME_SIZE];

	// --- Configure device specific values ---
	// Selecting a specific DIO# Pin is necessary for each T-Series Device, only specific DIO# pins can output a PWM signal.
	// For detailed T-Series Device DIO_EF pin mapping tables see section 13.2 of the T-Series Datasheet:
	// https://support.labjack.com/docs/13-2-dio-extended-features-t-series-datasheet
	deviceType = GetDeviceType(handle);
	if (deviceType == LJM_dtT4) {
		// For the T4, use FIO6 (DIO6) for the PWM output. T4 Core Clock Speed is 80 MHz.
		pwmDIO = 6;
		coreFrequency = 80000000;
	}
	else if (deviceType == LJM_dtT7) {
		// For the T7, use FIO2 (DIO2) for the PWM output. T7 Core Clock Speed is 80 MHz.
		pwmDIO = 2;
		coreFrequency = 80000000;
	}
	else if (deviceType == LJM_dtT8) {
		// For the T8, use FIO2 (DIO2) for the PWM output. T8 Core Clock Speed is 100 MHz.
		pwmDIO = 2;
		coreFrequency = 100000000;
	}
	else {
		printf("Unknown LabJack device type %d.\n", deviceType);
		return;
	}

	// --- How to Configure a Clock and PWM Signal? ---
	// See Datasheet reference for DIO_EF Clocks:
	// https://support.labjack.com/docs/13-2-1-ef-clock-source-t-series-datasheet
	//
	// To configure a DIO_EF PWM out signal, you first need to configure the clock used by the DIO_EF mode.
	//
	// --- Registers used for configuring Clocks ---
	// "DIO_FE_CLOCK#_DIVISOR":    Divides the core clock. Valid options: 1, 2, 4, 8, 16, 32, 64, 256.
	// "DIO_EF_CLOCK#_ROLL_VALUE": The clock count will increment continuously and then start over at zero as it reaches the roll value.
	// "DIO_EF_CLOCK#_ENABLE":     Enables/Disables the Clock.
	//
	// --- Registers used for configuring PWM ---
	// "DIO#_EF_INDEX":            Sets desired DIO_EF feature, DIO_EF PWM mode is index 0.
	// "DIO#_EF_CLOCK_SOURCE":     (Formerly DIO#_EF_OPTIONS). Specify which clock source to use.
	// "DIO#_EF_CONFIG_A":         When the clocks count matches this value, the line will transition from high to low.
	// "DIO#_EF_ENABLE":           Enables/Disables the DIO_EF mode.
	//
	// To configure a DIO_EF clock to any desired frequency, you need to calculate the Clock Tick Rate and then the Clock Roll Value.
	// Clock Tick Rate = Core Frequency / DIO_EF_CLOCK#_DIVISOR
	// Clock Roll Value = Clock Tick Rate / Desired Frequency
	//
	// In general, a slower Clock#Frequency will increase the maximum measurable period,
	// and a faster Clock#Frequency will increase measurement resolution.
	//
	// For more information on DIO_EF Clocks see section 13.2.1 - EF Clock Source of the T-Series Datasheet:
	// https://support.labjack.com/docs/13-2-1-ef-clock-source-t-series-datasheet
	//
	// For a more detailed walkthrough see Configuring a PWM Output:
	// https://support.labjack.com/docs/configuring-a-pwm-output

	// Calculate Clock Values
	clockDivisor = 1;
	clockTickRate = coreFrequency / clockDivisor;
	clockRollValue = clockTickRate / desiredFrequency;  // clockRollValue should be written to "DIO_EF_CLOCK0_ROLL_VALUE"

	// Below is a single equation which calculates the same value as the above equations
	//clockRollValue = coreFrequency / clockDivisor / desiredFrequency;


	// --- Calculate PWM Values ---
	// Calculate the clock tick value where the line will transition from high to low based on user defined duty cycle percentage, rounded to the nearest integer.
	pwmConfigA = (int)(clockRollValue * (desiredDutyCycle / 100.0));

	// What the PWM signal will look like based on Clock0 Count for a 50% Duty Cycle.
	// PWM will go high when Clock Count = 0, and then go low halfway to the Clock Roll Value thus a 50% duty cycle.
	//  __________            __________
	// |          |          |          |          |
	// |          |__________|          |__________|
	// 0        Roll/2      Roll      Roll/2      Roll
	// 0          50%       100%       50%        100%

	// --- Configure and write values to connected device ---
	// Configure Clock Registers, use 32-bit Clock0 for this example.
	WriteNameOrDie(handle, "DIO_EF_CLOCK0_DIVISOR", clockDivisor);       // Set Clock Divisor.
	WriteNameOrDie(handle, "DIO_EF_CLOCK0_ROLL_VALUE", clockRollValue);  // Set calculated Clock Roll Value.

	// Configure PWM Registers
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_INDEX", pwmDIO);
	WriteNameOrDie(handle, nameStr, 0);           // Set DIO#_EF_INDEX to 0 - PWM Out.
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_CLOCK_SOURCE", pwmDIO);
	WriteNameOrDie(handle, nameStr, 0);           // Set DIO#_EF to use clock 0. Formerly DIO#_EF_OPTIONS, you may need to switch to this name on older LJM versions.
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_CONFIG_A", pwmDIO);
	WriteNameOrDie(handle, nameStr, pwmConfigA);  // Set DIO#_EF_CONFIG_A to the calculated value.
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_ENABLE", pwmDIO);
	WriteNameOrDie(handle, nameStr, 1);           // Enable the DIO#_EF Mode, PWM signal will not start until DIO_EF and CLOCK are enabled.

	WriteNameOrDie(handle, "DIO_EF_CLOCK0_ENABLE", 1);  // Enable Clock0, this will start the PWM signal.


	printf("A PWM Signal at %.1f Hz with a duty cycle of %.1f %% is now being output on DIO%d for 10 seconds.\n",
		desiredFrequency, desiredDutyCycle, pwmDIO);

	MillisecondSleep(10000);  // Sleep for 10 Seconds = 10000 ms, remove this line to allow PWM to run until stopped.

	// Turn off Clock and PWM output.
	WriteNameOrDie(handle, "DIO_EF_CLOCK0_ENABLE", 0);
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_ENABLE", pwmDIO);
	WriteNameOrDie(handle, nameStr, 0);
}