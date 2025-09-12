/**
 * Name: dio_ef_config_counter.c
 * Desc: Enables an Interrupt Counter measurement to rising edges and a 10 Hz
 * square wave on DAC1. To measure the rising edges on DAC1, connect a jumper
 * between DAC1 and FIO0 on T7/T8 or FIO4 on T4.
 *
 * The Interrupt Counter counts the rising edge of pulses on the associated IO
 * line. This interrupt-based digital I/O extended feature (DIO-EF) is not
 * purely implemented in hardware, but rather firmware must service each edge.
 *
 * This example will read the DAC1 rising edge count at 1 second intervals 5
 * times. Then the count will be read and reset, and after 1 second, the count
 * is read again.
 *
 * For more information on the Interrupt Counter DIO_EF mode see section 13.2.9 of
 * the T-Series Datasheet.
 * https://support.labjack.com/docs/13-2-9-interrupt-counter-t-series-datasheet
**/

// For printf
#include <stdio.h>

// For the LabJackM Library
#include <LabJackM.h>

// For LabJackM helper functions, such as OpenOrDie, PrintDeviceInfoFromHandle,
// ErrorCheck, etc.
#include "../../LJM_Utilities.h"

void dio_ef_config_counter(int handle);

int main()
{
	int handle;

	// Open first found LabJack
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
	// handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	PrintDeviceInfoFromHandle(handle);

	dio_ef_config_counter(handle);

	CloseOrDie(handle);

	WaitForUserIfWindows();

	return LJME_NOERROR;
}

void dio_ef_config_counter(int handle)
{
	// --- Interrupt Counter ---
	int counterDIO = 0;  // DIO Pin that will measure the signal, T7/T8 use FIO0, T4 use FIO4.

	int deviceType = 0;
	char nameStr[LJM_MAX_NAME_SIZE];
	int i = 0;
	int err = 0;
	double numRisingEdges = 0;
	double numRisingEdgesBeforeReset = 0;
	double numRisingEdgesAfterReset = 0;


	// Selecting a specific DIO# Pin is necessary for each T-Series Device, only specific DIO# pins can do an Interrupt Counter measurement.
	// For detailed T-Series Device DIO_EF pin mapping tables see section 13.2 of the T-Series Datasheet:
	// https://support.labjack.com/docs/13-2-dio-extended-features-t-series-datasheet
	deviceType = GetDeviceType(handle);
	if (deviceType == LJM_dtT4) {
		// For the T4, use FIO4/DIO4 for the Interrupt Counter measurement.
		counterDIO = 4;
	}

	// --- How to Configure Interrupt Counter Measurment ---
	// See Datasheet reference for DIO_EF Interrupt Counter:
	// https://support.labjack.com/docs/13-2-9-interrupt-counter-t-series-datasheet
	//
	// -- Registers used for configuring DAC1 Frequency Out ---
	// "DAC1_FREQUENCY_OUT_ENABLE": 0 = off, 1 = output 10 Hz signal on DAC1. The signal will be a square wave with peaks of 0 and 3.3V.
	//
	// --- Registers used for configuring Interrupt Counter ---
	// "DIO#_EF_INDEX":            Sets desired DIO_EF feature, Interrupt Counter is DIO_EF Index 8.
	// "DIO#_EF_ENABLE":           Enables/Disables the DIO_EF mode.
	//
	// Interrupt Counter counts the rising edge of pulses on the associated IO line.
	// This interrupt-based digital I/O extended feature (DIO-EF) is not purely implemented in hardware, but rather firmware must service each edge.
	//
	// For a more detailed walkthrough see Configuring and Reading a Counter:
	// https://support.labjack.com/docs/configuring-reading-a-counter
	//
	// For a more accurate measurement for counting Rising edges, use the hardware clocked High-Speed Counter mode.
	// See the docs for High-Speed Counter here:
	// https://support.labjack.com/docs/13-2-8-high-speed-counter-t-series-datasheet


	// Configure Interrupt Counter Registers
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_ENABLE", counterDIO);
	WriteNameOrDie(handle, nameStr, 0);                        // Disable the DIO#_EF before configuration.
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_INDEX", counterDIO);
	WriteNameOrDie(handle, nameStr, 8);                        // Set DIO#_EF_INDEX to 8 for Interrupt Counter.
	WriteNameOrDie(handle, "DAC1_FREQUENCY_OUT_ENABLE", 1);    // Enable 10 Hz square wave on DAC1.
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_ENABLE", counterDIO);
	WriteNameOrDie(handle, nameStr, 1);                        // Enable the DIO#_EF Mode.

	printf("\n--- Outputting a 10 Hz signal on DAC1, measuring signal on FIO%d ---\n\n", counterDIO);

	// --- How to read the measured count of rising edges? ---
	// To read the count of Rising Edges, use the register below.
	// DIO#_EF_READ_A: Returns the current Count.
	//
	// To read and reset the count:
	// DIO#_EF_READ_A_AND_RESET: Reads the current count then clears the counter.
	//
	// Note that there is a brief period of time between reading and clearing during which edges can be missed.
	// During normal operation this time period is 10-30 microseconds.
	// If missed edges at this point can not be tolerated then reset should not be used.

	// If measuring at 1 second intervals, you should expect to see ~10 rising edges per second on the 10 Hz DAC1_FREQUENCY_OUT signal.

	numRisingEdges = 0;
	numRisingEdgesBeforeReset = 0;
	numRisingEdgesAfterReset  = 0;

	// Read all of the measured values.
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_READ_A", counterDIO);
	for (i = 0; i < 5; i++) {
		MillisecondSleep(1000);  // Sleep for 1 Second
		err = LJM_eReadName(handle, nameStr, &numRisingEdges);
		ErrorCheck(err, "LJM_eReadName");

		printf("DIO_EF Measured Values - Rising Edges: %.1f\n", numRisingEdges);
	}

	printf("\n--- Reading and Resetting the count of DIO%d ---\n\n", counterDIO);

	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_READ_A_AND_RESET", counterDIO);
	err = LJM_eReadName(handle, nameStr, &numRisingEdgesBeforeReset);
	ErrorCheck(err, "LJM_eReadName");
	MillisecondSleep(1000);  // Sleep for 1 Second
	err = LJM_eReadName(handle, nameStr, &numRisingEdgesAfterReset);
	ErrorCheck(err, "LJM_eReadName");

	printf("DIO_EF Edges Before Read and Reset: %.1f\n", numRisingEdgesBeforeReset);
	printf("DIO_EF Edges After Read and Reset + 1 sec sleep: %.1f\n", numRisingEdgesAfterReset);


	// Disable Counter and DAC1 Frequency Out.
	printf("\n--- Disabling Interrupt Counter and DAC1_FREQUENCY_OUT ---\n\n");
	snprintf(nameStr, LJM_MAX_NAME_SIZE, "DIO%d_EF_ENABLE", counterDIO);
	WriteNameOrDie(handle, nameStr, 0);
	WriteNameOrDie(handle, "DAC1_FREQUENCY_OUT_ENABLE", 0);
}
