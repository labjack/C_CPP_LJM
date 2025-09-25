/**
 * Name: dio_ef_config_freq.c
 * Desc: Enables a Frequency In measurement and a 10 Hz square wave on DAC1. To
 * measure the DAC1 Frequency and Period, connect a jumper between DAC1 and FIO0
 * on T7/T8 or FIO4 on T4.
 *
 * Frequency In will measure the period or frequency of a digital input signal
 * by counting the number of clock source ticks between two edges:
 * rising-to-rising (DIO_EF index = 3) or falling-to-falling (DIO_EF index = 4).
 *
 * This example will read the DAC1 frequency and period 5 times at 1 second
 * intervals.
 *
 * For more information on the Frequency In DIO_EF mode see section 13.2.5 of
 * the T-Series Datasheet:
 * https://support.labjack.com/docs/13-2-5-frequency-in-t-series-datasheet
**/

// For printf
#include <stdio.h>

// For the LabJackM Library
#include <LabJackM.h>

// For LabJackM helper functions, such as OpenOrDie, PrintDeviceInfoFromHandle,
// ErrorCheck, etc.
#include "../../LJM_Utilities.h"

void dio_ef_config_freq(int handle);

int main()
{
	int handle;

	// Open first found LabJack
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
	// handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	PrintDeviceInfoFromHandle(handle);

	dio_ef_config_freq(handle);

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}

void dio_ef_config_freq(int handle)
{
	// --- Configure Clock and Frequency In ---
	int freqDIO = 0;  // DIO Pin that will measure the signal, T7/T8 use FIO0, T4 use FIO4.

	int deviceType = 0;
	char nameStr[LJM_MAX_NAME_SIZE];
	double clockDivisor = 0;
	double clockRollValue = 0;
	double freqIndex = 0;
	double freqConfigA = 0;
	double periodTicks = 0;
	double periodSec = 0;
	double freqHz = 0;
	int intervalHandle = 0;
	int skippedIntervals = 0;
	int i = 0;
	int err = 0;
	int errAddress = 0;

	enum { NUM_FRAMES_AIN = 3 };
	double aValuesEFRead[NUM_FRAMES_AIN] = {0};
	char *aNamesEFRead[NUM_FRAMES_AIN];

	// Selecting a specific DIO# Pin is necessary for each T-Series Device, only specific DIO# pins can measure a Frequency In signal.
	// For detailed T-Series Device DIO_EF pin mapping tables see section 13.2 of the T-Series Datasheet:
	// https://support.labjack.com/docs/13-2-dio-extended-features-t-series-datasheet
	deviceType = GetDeviceType(handle);
	if (deviceType == LJM_dtT4) {
		// For the T4, use FIO4/DIO4 for the Frequency In measurement.
		freqDIO = 4;
	}

	// --- How to Configure Frequency In Measurment ---
	// To configure a DIO_EF Frequency In measurement mode, you need first to configure the clock used by the DIO_EF mode,
	// then configure the DIO_EF mode itself.
	//
	// See Datasheet reference for DIO_EF Clocks:
	// https://support.labjack.com/docs/13-2-1-ef-clock-source-t-series-datasheet
	//
	// See Datasheet reference for DIO_EF Frequency In:
	// https://support.labjack.com/docs/13-2-5-frequency-in-t-series-datasheet
	//
	// -- Registers used for configuring DAC1 Frequency Out ---
	// "DAC1_FREQUENCY_OUT_ENABLE": 0 = off, 1 = output 10 Hz signal on DAC1. The signal will be a square wave with peaks of 0 and 3.3V.
	//
	// --- Registers used for configuring DIO_EF Clock ---
	// "DIO_FE_CLOCK#_DIVISOR":    Divides the core clock. Valid options: 1, 2, 4, 8, 16, 32, 64, 256.
	// "DIO_EF_CLOCK#_ROLL_VALUE": The clock count will increment continuously and then start over at zero as it reaches the roll value.
	// "DIO_EF_CLOCK#_ENABLE":     Enables/Disables the Clock.
	//
	// --- Registers used for configuring Frequency In ---
	// "DIO#_EF_INDEX":            Sets desired DIO_EF feature, DIO_EF Frequency In uses index 3 for rising edges, 4 for falling edges.
	// "DIO#_EF_CLOCK_SOURCE":     (Formerly DIO#_EF_OPTIONS). Specify which clock source to use.
	// "DIO#_EF_CONFIG_A":         Measurement mode Default = 0. One-Shot measurement = 2, Continuous measurement = 0. See below for more info on measurement modes.
	// "DIO#_EF_ENABLE":           Enables/Disables the DIO_EF mode.
	//
	// Frequency In will measure the period or frequency of a digital input signal by counting the number of clock source ticks between two edges:
	// rising-to-rising (index=3) or falling-to-falling (index=4).
	//
	// DIO_EF Config A - Measurement Modes - One-Shot vs. Continuous:
	// - One-Shot
	//     When one-shot mode is enabled, the DIO_EF will complete a measurement then go idle.
	//     No more measurements will be made until the DIO_EF has been read or reset.
	//
	// - Continuous
	//     When continuous mode is enabled, the DIO_EF will repeatedly make measurements.
	//     If a new reading is completed before the old one has been read the old one will be discarded.
	//
	// Continuous Measurements are the default mode and recommended for most use cases.
	// For more info on measurement modes see:
	// https://support.labjack.com/docs/13-2-5-frequency-in-t-series-datasheet#id-13.2.5FrequencyIn[T-SeriesDatasheet]-Configure

	// --- Clock Configuration Values ---
	clockDivisor = 1;
	clockRollValue = 0;  // A value of 0 sets the clock to roll at its max value, allowing the maximum measurable period.

	// --- Frequency In Configuration Values ---
	freqIndex = 3;      // Measuring between rising-to-rising edges.
	//freqIndex = 4;    // Measuring between falling-to-falling edges.
	freqConfigA = 0;    // Measurement mode set to continuous, the default value.
	//freqConfigA = 2;  // Measurement mode set to one-shot.

	// --- Configure and write values to connected device ---
	// Configure Clock Registers, use 32-bit Clock0.
	WriteNameOrDie(handle, "DIO_EF_CLOCK0_DIVISOR", clockDivisor);       // Set Clock Divisor.
	WriteNameOrDie(handle, "DIO_EF_CLOCK0_ROLL_VALUE", clockRollValue);  // Set Clock Roll Value

	// Configure DIO_EF Frequency Registers.
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_ENABLE", freqDIO);
	WriteNameOrDie(handle, nameStr, 0);            // Enable the DIO#_EF Mode, DIO will not start measurement until DIO and Clock are enabled.
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_INDEX", freqDIO);
	WriteNameOrDie(handle, nameStr, freqIndex);    // Set DIO#_EF_INDEX to 3 for rising-to-rising, 4 for falling-to-falling
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_CLOCK_SOURCE", freqDIO);
	WriteNameOrDie(handle, nameStr, 0);            // Set DIO#_EF to use clock 0. Formerly DIO#_EF_OPTIONS, you may need to switch to this name on older LJM versions.
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_CONFIG_A", freqDIO);
	WriteNameOrDie(handle, nameStr, freqConfigA);  // Set DIO#_EF_CONFIG_A to set measurement mode.
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_ENABLE", freqDIO);
	WriteNameOrDie(handle, nameStr, 1);            // Enable the DIO#_EF Mode, DIO will not start measurement until DIO and Clock are enabled.

	WriteNameOrDie(handle, "DIO_EF_CLOCK0_ENABLE", 1);       // Enable Clock0, this will start the measurements.

	WriteNameOrDie(handle, "DAC1_FREQUENCY_OUT_ENABLE", 1);  // Enable 10 Hz square wave on DAC1.

	printf("\n--- Outputting a 10 Hz signal on DAC1, measuring signal on FIO%d ---\n\n", freqDIO);

	// --- How to read the measured frequency ---
	// There are multiple registers available to read results from for this DIO_EF mode.
	// DIO#_EF_READ_A: Returns the period in ticks. If a full period has not yet been observed this value will be zero.
	// DIO#_EF_READ_B: Returns the same value as READ_A.
	// DIO#_EF_READ_A_F: Returns the period in seconds. If a full period has not yet been observed this value will be zero.
	// DIO#_EF_READ_B_F: Returns the frequency in Hz. If a full period has not yet been observed this value will be zero.
	//
	// Note that all "READ_B" registers are capture registers.
	// All "READ_B" registers are only updated when any "READ_A" register is read.
	// Thus it would be unusual to read any B registers without first reading at least one A register.
	//
	// To Reset the READ registers:
	// DIO#_EF_READ_A_AND_RESET: Returns the same data as DIO#_EF_READ_A and then clears the result
	// so that zero is returned by subsequent reads until another full period is measured (2 new edges).
	//
	// DIO#_EF_READ_A_AND_RESET_F: Returns the same data as DIO#_EF_READ_A_F and then clears the result
	// so that zero is returned by subsequent reads until another full period is measured (2 new edges).
	//
	// Note that when One-Shot mode is enabled, there is conflicting behavior
	// between one-shot and READ_A_AND_RESET registers which can lead to unexpected results.
	// For more info see:
	// https://support.labjack.com/docs/13-2-12-interrupt-frequency-in-t-series-datasheet#id-13.2.12InterruptFrequencyIn[T-SeriesDatasheet]-One-shot,Continuous,Read,ReadandReset

	periodTicks = 0;
	periodSec = 0;
	freqHz = 0;

	// Start a 1 second interval.
	intervalHandle = 1;
	err = LJM_StartInterval(intervalHandle, 1000000);
	ErrorCheck(err, "LJM_StartInterval");

	// Read all of the measured values.
	aNamesEFRead[0] = (char *)malloc(sizeof(char) * LJM_MAX_NAME_SIZE);
	aNamesEFRead[1] = (char *)malloc(sizeof(char) * LJM_MAX_NAME_SIZE);
	aNamesEFRead[2] = (char *)malloc(sizeof(char) * LJM_MAX_NAME_SIZE);
	snprintf(aNamesEFRead[0], LJM_MAX_NAME_SIZE, "DIO%d_EF_READ_A", freqDIO);
	snprintf(aNamesEFRead[1], LJM_MAX_NAME_SIZE, "DIO%d_EF_READ_A_F", freqDIO);
	snprintf(aNamesEFRead[2], LJM_MAX_NAME_SIZE, "DIO%d_EF_READ_B_F", freqDIO);

	for (i = 0; i < 5; i++) {
		err = LJM_WaitForNextInterval(intervalHandle, &skippedIntervals);  // Wait for 1 Second = 1000 ms.
		PrintErrorIfError(err, "LJM_WaitForNextInterval");

		// Get the period ticks, period seconds and frequency.
		err = LJM_eReadNames(handle, NUM_FRAMES_AIN, (const char **)aNamesEFRead, aValuesEFRead, &errAddress);
		PrintErrorWithAddressIfError(err, errAddress, "LJM_eReadNames");
		periodTicks = aValuesEFRead[0];
		periodSec = aValuesEFRead[1];
		freqHz = aValuesEFRead[2];

		printf("DIO_EF Measured Values - Frequency: %f Hz | Period: %f sec %.1f ticks.\n",
			  freqHz, periodSec, periodTicks);
	}

	free(aNamesEFRead[0]);
	free(aNamesEFRead[1]);
	free(aNamesEFRead[2]);

	// Clean up the memory for the interval handle.
	err = LJM_CleanInterval(intervalHandle);
	ErrorCheck(err, "LJM_CleanInterval");

	// Disable Clock, Frequency Measurement, and DAC1 Frequency Out.
	printf("\n--- Disabling Clock0, Frequency In, and DAC1_FREQUENCY_OUT ---\n");
	WriteNameOrDie(handle, "DIO_EF_CLOCK0_ENABLE", 0);
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_ENABLE", freqDIO);
	WriteNameOrDie(handle, nameStr, 0);
	WriteNameOrDie(handle, "DAC1_FREQUENCY_OUT_ENABLE", 0);
}
