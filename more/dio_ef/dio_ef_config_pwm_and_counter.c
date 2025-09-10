/**
 * Name: dio_ef_config_pwm_and_counter.c
 * Desc: Enables a 10 kHz PWM output and high speed counter, and reads the
 * counter every 1 second in a loop. If you jumper the counter to PWM, it should
 * increment about 10000 counts each loop iteration.
 *
 * To configure PWM to user desired frequency and duty cycle, modify the
 * "desiredFrequency" and "desiredDutyCyclePercent" variables.
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
	// ------------- USER INPUT VALUES -------------
	double desiredFrequency = 10000;      // Set this value to your desired PWM Frequency Hz. Default 10000 Hz
	double desiredDutyCyclePercent = 50;  // Set this value to your desired PWM Duty Cycle percentage. Default 50%
	// ---------------------------------------------

	int pwmDIO = 0;
	int counterDIO = 0;
	double coreFrequency = 0;
	double clockDivisor = 0;
	double clockTickRate = 0;
	double clockRollValue = 0;
	double pwmConfigA = 0;
	double counterVal = 0;
	int deviceType = 0;
	int intervalHandle = 0;
	int skippedIntervals = 0;
	int i = 0;
	char nameStr[LJM_MAX_NAME_SIZE];
	int err = 0;

	// -- High-Speed Counter Info and Hardware Conflicts ---
	// The DIO_EF High-Speed Counter uses a hardware counter to achieve high count rates up to 5 MHz.
	// The hardware counters are also used by the EF clocks which can cause potential hardware conflicts.
	// Enabling a High-Speed Counter on a DIO pin which conflicts with the EF clocks will cause an error.
	// The conflicts between hardware are outlined below.
	//
	// T4 and T7 devices support up to 4 high-speed rising-edge counters, these counters are shared with other resources as follows:
	// CounterA (DIO16/CIO0): Used by EF Clock0 and Clock1.
	// CounterB (DIO17/CIO1): Used by EF Clock0 and Clock2.
	// CounterC (DIO18/CIO2): Used by the asynchronous serial communication feature on the T4. Always available on the T7.
	// CounterD (DIO19/CIO3): Used by stream mode.
	//
	// The T8 supports up to 7 high-speed rising-edge counters,these counters are shared with other resources as follows:
	// DIO14/EIO6: Used by EF Clock0 and Clock1.
	// DIO15/EIO7: Used by EF Clock0 and Clock2.
	//
	// Selecting a specific DIO# Pin is necessary for each T-Series Device, only specific DIO# pins can measure with a High-Speed Counter.
	// For detailed T-Series Device DIO_EF pin mapping tables see section 13.2 of the T-Series Datasheet:
	// https://support.labjack.com/docs/13-2-dio-extended-features-t-series-datasheet
	deviceType = GetDeviceType(handle);
	if (deviceType == LJM_dtT4) {
		// For the T4, use FIO6 (DIO6) for the PWM output.
		// Use CIO2 (DIO18) for the high speed counter.
		// T4 Core Clock Speed is 80 MHz.
		pwmDIO = 6;
		counterDIO = 18;
		coreFrequency = 80000000;
	}
	else if (deviceType == LJM_dtT7) {
		// For the T7, use FIO2 (DIO2) for the PWM output.
		// Use CIO2 (DIO18) for the high speed counter.
		// T7 Core Clock Speed is 80 MHz.
		pwmDIO = 2;
		counterDIO = 18;
		coreFrequency = 80000000;
	}
	else if (deviceType == LJM_dtT8) {
		// For the T8, use FIO2 (DIO2) for the PWM output.
		// Use FIO6 (DIO6) for the high speed counter.
		// T8 Core Clock Speed is 100 MHz.
		pwmDIO = 2;
		counterDIO = 6;
		coreFrequency = 100000000;
	}
	else {
		printf("Unknown LabJack device type %d.\n", deviceType);
		return;
	}

	// --- How to Configure PWM and Counter ---
	// To configure a DIO_EF PWM out signal, you first need to configure the clock used by the DIO_EF mode,
	// Then you can configure the PWM and the High-Speed Counter.
	//
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
	// --- Registers used for configuring High-Speed Counter ---
	// "DIO#_EF_INDEX":            Sets desired DIO_EF feature, DIO_EF High-Speed Counter is index 7.
	// "DIO#_EF_ENABLE":           Enables/Disables the DIO_EF mode.
	//
	// For more info on the DIO_EF Clocks see section 13.2.1 of the T-Series Datasheet:
	// https://support.labjack.com/docs/13-2-1-ef-clock-source-t-series-datasheet
	//
	// For a more detailed walkthrough see Configuring a PWM Output:
	// https://support.labjack.com/docs/configuring-a-pwm-output

	// --- Calculate Clock Values from user defined values ---
	clockDivisor = 1;
	clockTickRate = coreFrequency / clockDivisor;
	clockRollValue = clockTickRate / desiredFrequency;  // clockRollValue should be written to "DIO_EF_CLOCK0_ROLL_VALUE"

	// Below is a single equation which calculates the same value as the above equations
	//clockRollValue = coreFrequency / clockDivisor / desiredFrequency;

	// --- Calculate PWM Values ---
	// Calculate the clock tick value where the line will transition from high to low based on user defined duty cycle percentage, rounded to the nearest integer.
	pwmConfigA = (int)(clockRollValue * (desiredDutyCyclePercent / 100.0));

	// --- Configure and write values to connected device ---
	// Configure Clock Registers, use 32-bit Clock0 for this example.
	WriteNameOrDie(handle, "DIO_EF_CLOCK0_DIVISOR", clockDivisor);     // Set Clock Divisor.
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

	// Configure High-Speed Counter Registers
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_INDEX", counterDIO);
	WriteNameOrDie(handle, nameStr, 7);                 // Set DIO#_EF_INDEX to 7 - High-Speed Counter.
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_ENABLE", counterDIO);
	WriteNameOrDie(handle, nameStr, 1);                 // Enable the High-Speed Counter.
	WriteNameOrDie(handle, "DIO_EF_CLOCK0_ENABLE", 1);  // Enable Clock0, this will start the PWM signal.


	// Start a 1 second interval
	intervalHandle = 1;
	err = LJM_StartInterval(intervalHandle, 1000000);
	ErrorCheck(err, "LJM_StartInterval");

	// Reading from the counter in a loop
	for (i = 0; i < 5; i++) {
		// Wait until the 1 second interval is complete
		err = LJM_WaitForNextInterval(intervalHandle, &skippedIntervals);
		ErrorCheck(err, "LJM_WaitForNextInterval");

		// Read from the counter
		snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_READ_A", counterDIO);
		err = LJM_eReadName(handle, nameStr, &counterVal);
		ErrorCheck(err, "LJM_WaitForNextInterval");

		printf("\nCounter - %.0f\n", counterVal);
		if (skippedIntervals > 0) {
			printf("SkippedIntervals: %d\n", skippedIntervals);
		}
	}

	// Clean up the memory for the interval handle.
	err = LJM_CleanInterval(intervalHandle);
	ErrorCheck(err, "LJM_CleanInterval");

	// Turn off PWM output and counter
	WriteNameOrDie(handle, "DIO_EF_CLOCK0_ENABLE", 0);
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_ENABLE", pwmDIO);
	WriteNameOrDie(handle, nameStr, 0);
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_ENABLE", counterDIO);
	WriteNameOrDie(handle, nameStr, 0);
}
