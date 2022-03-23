/**
 *	Name: c-r_speed_test.c
 *	Desc: Performs LabJack operations in a loop and reports the timing
 *		statistics for the operations.
 *	Note: Running this program via an IDE may reduce performance, causing a
 *		significant increase in round-trip communication times. Such is the
 *		case with Visual Studio - in both Debug and Release modes.
 *
 * Relevant Documentation:
 *
 * This file was used to generate the data given here:
 *		https://labjack.com/support/app-notes/maximum-command-response
 * Lua scripts for speed testing can be found here:
 *		hhttps://labjack.com/support/software/examples/lua-scripting/benchmarking-tests
 *
 * LJM Library:
 *	LJM Library Installer:
 *		https://labjack.com/support/software/installers/ljm
 *	LJM Users Guide:
 *		https://labjack.com/support/software/api/ljm
 *	Opening and Closing:
 *		https://labjack.com/support/software/api/ljm/function-reference/opening-and-closing
 *	Single Value Functions(such as eReadName):
 *		https://labjack.com/support/software/api/ljm/function-reference/single-value-functions
 *	Raw Byte Functions:
 *		https://labjack.com/support/software/api/ljm/function-reference/lowlevel-functions/raw-byte-functions
 *
 * T-Series and I/O:
 *	Modbus Map:
 *		https://labjack.com/support/software/api/modbus/modbus-map
 *	Digital I/O:
 *		https://labjack.com/support/datasheets/t-series/digital-io
 *	Analog Inputs:
 *		https://labjack.com/support/datasheets/t-series/ain
**/

// For printf
#include <stdio.h>
// For sorting data values
#include<stdlib.h>

#include <math.h>

// For the LabJackM Library
#include <LabJackM.h>

// For LabJackM helper functions, such as PrintDeviceInfoFromHandle
#include "../../LJM_Utilities.h"

// The first iteration consistently takes much longer to run, so toss it and
// set the number of iterations equal to the desired number of iterations + 1
enum{NUM_ITERATIONS = 10001};

typedef unsigned char bool; // Workaround for when stdbool.h is unavailable
#define false 0;
#define true 1;

typedef struct WhatToModify {
	bool digitalWrite;
	bool digitalRead;
	bool writeDACs;
	int numAIN;
} WhatToModify;

/**
 * Desc: Configures AIN settings on the device. On error, prints error, closes
 *       all devices, and exits the program.
 * Para: handle, the device to configure
 *       myTests, struct that holds information regarding how many AIN need to
 *                be configured as well as information for setting DIO_INHIBIT
 * Note: numAIN must be be greater than 0
**/
void ConfigureAIN(const int handle, const struct WhatToModify myTests);

/**
 * Desc: Sets up and runs a c-r speed test then prints data to file.
 * Para: DEVICE_TYPE, the device type to open and run tests on
 *       connectionType, the connection type to run the test over
 *       DEVICE_ID, the serial number or ip address of the device to connect to
 *       testFileName, the name of the output data file
 *       myTests, struct with booleans indicating some tests to run (read
 *                digital, write digital, write DACs) as well as the number of
 *                AIN to test
**/
void RunSpeedTest(
	const int DEVICE_TYPE,
	const int connectionType,
	const char * DEVICE_ID,
	const char * testFileName,
	const WhatToModify myTests
);

/**
 * Desc: Prints the tests to be ran
 * Para: myTests, struct with booleans indicating some tests to run (read
 *                digital, write digital, write DACs) as well as the number of
 *                AIN to test
**/
void PrintTests(const WhatToModify myTests);

int main() {
	const int DEVICE_TYPE = LJM_dtANY;
	const char * DEVICE_ID = "ANY";
	int connectionType;
	char * test1FileName = "filename.txt";
	struct WhatToModify myTests;
	// Set which tests you want to run
	myTests.digitalRead = true;
	myTests.digitalWrite = false;
	myTests.writeDACs = false;
	// Number of AIN to read in this test
	myTests.numAIN = 0;

	// Multiple connections can be tested at once. For example, if you want to
	// test USB and Ethernet at once you could simply uncomment the Ethernet
	// test below and the program would run the USB test then the Ethernet test.

	//*****Test for USB connection*****//

	connectionType = LJM_ctUSB;
	RunSpeedTest(
		DEVICE_TYPE,
		connectionType,
		DEVICE_ID,
		test1FileName,
		myTests
	);

	//*****Test for Ethernet connection*****//

	// connectionType = LJM_ctETHERNET;
	// char * test2FileName = "filename2.txt";
	// RunSpeedTest(
	//     DEVICE_TYPE,
	//     connectionType,
	//     DEVICE_ID,
	//     test2FileName,
	//     myTests
	// );

	//*****Test for WiFi connection*****//

	// connectionType = LJM_ctWIFI;
	// char * test3FileName = "filename3.txt";
	// RunSpeedTest(
	//     DEVICE_TYPE,
	//     connectionType,
	//     DEVICE_ID,
	//     test3FileName,
	//     myTests
	// );

	printf("Exiting program\n");
	WaitForUserIfWindows();
	return 0;
}

void RunSpeedTest(
	const int DEVICE_TYPE,
	const int connectionType,
	const char * DEVICE_ID,
	const char * testFileName,
	const struct WhatToModify myTests
){
	int handle;
	int errorAddress = INITIAL_ERR_ADDRESS;
	int err = 0;
	int frame = 0;
	int numFrames = 0;
	int i;
	char ** aNames;
	int * aWrites;
	int * aNumValues;
	double * aValues;
	double iterationTimes[NUM_ITERATIONS];
	double totalTime;
	double averageIterationTime = 0;
	FILE *speedTestData;

	handle = OpenOrDie(DEVICE_TYPE, connectionType, DEVICE_ID);
	PrintDeviceInfoFromHandle(handle);
	printf("\n");

	// the number of frames should equal the number of registers you want to
	// read / write
	if (myTests.writeDACs)
		numFrames += 2;
	if (myTests.digitalRead)
		++numFrames;
	if (myTests.digitalWrite)
		++numFrames;
	if (myTests.numAIN > 0) {
		ConfigureAIN(handle, myTests);
		numFrames += myTests.numAIN;
	}

	// Allocate memory based on how many frames we need
	aNames = malloc (numFrames * sizeof(char *));
	aWrites = malloc (numFrames * sizeof(int));
	aNumValues = malloc (numFrames * sizeof(int));
	aValues = malloc (numFrames * sizeof(double));

	// Add a frame for each register you want to read/write
	for (i = 0; i < myTests.numAIN; i++) {
		aNames[frame] = (char *) malloc (LJM_MAX_NAME_SIZE * sizeof(char));
		sprintf(aNames[frame], "AIN%d", i);
		aWrites[frame] = LJM_READ;
		aNumValues[frame] = 1;
		aValues[frame] = 0;
		++frame;
	}

	if (myTests.digitalRead) {
		aNames[frame] = malloc (LJM_MAX_NAME_SIZE * sizeof(char));
		sprintf(aNames[frame], "FIO_STATE");
		aWrites[frame] = LJM_READ;
		aNumValues[frame] = 1;
		aValues[frame] = 0;
		++frame;
	}

	if (myTests.digitalWrite) {
		aNames[frame] = malloc (LJM_MAX_NAME_SIZE * sizeof(char));
		sprintf(aNames[frame], "FIO_STATE");
		aWrites[frame] = LJM_WRITE;
		aNumValues[frame] = 1;
		aValues[frame] = 0; // output-low
		++frame;
	}

	if (myTests.writeDACs) {
		for (i=0; i<2; i++) {
			aNames[frame] = malloc (LJM_MAX_NAME_SIZE * sizeof(char));
			sprintf(aNames[frame], "DAC%d", i);
			aWrites[frame] = LJM_WRITE;
			aNumValues[frame] = 1;
			aValues[frame] = 0.0; // 0.0 V
			++frame;
		}
	}

    printf("Starting ");
    PrintTests(myTests);
	totalTime = LJM_GetHostTick();
	for (i = 0; i < NUM_ITERATIONS; i++) {
		if (i != 0) {
			iterationTimes[i-1] = LJM_GetHostTick();
		}
		err = LJM_eNames(
			handle,
			numFrames,
			(const char **)aNames,
			aWrites,
			aNumValues,
			aValues,
			&errorAddress
		);
		// The first iteration consistently takes much longer to run, so
		// toss it
		if (i != 0) {
			ErrorCheck(err, "problem in eNames");
			iterationTimes[i-1] = (LJM_GetHostTick() - iterationTimes[i-1]) / 1000;
			averageIterationTime += iterationTimes[i-1];
		}
		// Only check if there was an error while setting first data point
		else {
			ErrorCheck(err, "problem in eNames");
		}
	}
	totalTime = (LJM_GetHostTick() - totalTime) / 1000;
    printf("Testing done!\n");
    printf("Total time: %.3fms\n", totalTime);
    averageIterationTime = averageIterationTime / (NUM_ITERATIONS -1);
	printf("Avg iteration time: %.4fms\n\n", averageIterationTime);

    printf("Writing test data to %s...\n", testFileName);
	speedTestData = fopen(testFileName, "w");
	if (speedTestData != NULL) {
		for(i = 0; i < NUM_ITERATIONS-1; i++) {
			fprintf(speedTestData, " %.4f", iterationTimes[i]);
		}
		fprintf(speedTestData, "\n\n %.4f\n", averageIterationTime);
		fclose(speedTestData);
		printf("Closing %s\n\n", testFileName);
	}

	free(aNames);
	CloseOrDie(handle);
}

void ConfigureAIN(const int handle, const struct WhatToModify myTests)
{
	int i = 0;
	enum { NUM_FRAMES = 3 };
	char range[LJM_MAX_NAME_SIZE];
	char resolutionIndex[LJM_MAX_NAME_SIZE];
	char settling[LJM_MAX_NAME_SIZE];
	char * aNames[NUM_FRAMES] = {range, resolutionIndex, settling};
	double aValues[NUM_FRAMES];
	int errorAddress = INITIAL_ERR_ADDRESS;
	int err = 0;

	// T4 analog input configuration
	double T4RangeAIN_HV = 10.0; // HV channels range
	double T4RangeAIN_LV = 2.4; // LV channels range
	double dioInhibit, dioAnalogEnable;

	// T7 and T8 analog input range
	double rangeAIN = 10.0;

	double resolutionAIN = 1;
	double settlingIndexAIN = 0;

	printf("ConfigureAIN:\n");
	printf("  resolutionAIN: %f\n", resolutionAIN);
	printf("  settlingIndexAIN: %f\n", settlingIndexAIN);
	printf("\n");

	if (myTests.numAIN < 1) {
		printf("ConfigureAIN: numAIN must be greater than 0\n");
		LJM_CloseAll();
		exit(1);
	}

	if (GetDeviceType(handle) == LJM_dtT4) {
		// Configure the channels to analog input or digital I/O
		// Update all digital I/O channels. b1 = Ignored. b0 = Affected.
		dioInhibit = 0x00000; // (b00000000000000000000)
		// Set AIN 0 to numAIN-1 as analog inputs (b1), the rest as digital I/O
		// (b0).
		dioAnalogEnable = (1 << myTests.numAIN) - 1;
		WriteNameOrDie(handle, "DIO_INHIBIT", dioInhibit);
		WriteNameOrDie(handle, "DIO_ANALOG_ENABLE", dioAnalogEnable);
		if (myTests.digitalWrite) {
			// Update only digital I/O channels in future digital write calls.
			// b1 = Ignored. b0 = Affected.
			dioInhibit = dioAnalogEnable;
			WriteNameOrDie(handle, "DIO_INHIBIT", dioInhibit);
		}
	}

	for (i=0; i<myTests.numAIN; i++) {
		sprintf(range, "AIN%d_RANGE", i);
		if (GetDeviceType(handle) == LJM_dtT4) {
			if (i < 4) {
				aValues[0] = T4RangeAIN_HV;
			}
			else {
				aValues[0] = T4RangeAIN_LV;
			}
		}
		else {
			aValues[0] = rangeAIN;
		}

		sprintf(resolutionIndex, "AIN%d_RESOLUTION_INDEX", i);
		aValues[1] = resolutionAIN;

		sprintf(settling, "AIN%d_SETTLING_US", i);
		aValues[2] = settlingIndexAIN;

		err = LJM_eWriteNames(handle, NUM_FRAMES, (const char **)aNames,
			aValues, &errorAddress);
		ErrorCheckWithAddress(err, errorAddress,
			"ConfigureAIN: LJM_eWriteNames");
	}
}

void PrintTests(const WhatToModify myTests)
{
	printf("test(s) for:\n");
	if (myTests.digitalWrite) {
		printf("digital write\n");
	}
	if (myTests.digitalRead) {
		printf("digital read\n");
	}
	if (myTests.writeDACs) {
		printf("writing DACs\n");
	}
	if (myTests.numAIN > 0) {
		printf("reading %d AIN\n", myTests.numAIN);
	}
	printf("\n");
}
