/**
 * Name: thermocouple_example.c
 * Desc: Demonstrates thermocouple configuration and  measurement
 *		This example demonstrates usage of the thermocouple AIN_EF (T7/T8 only)
 *		and a solution using our LJTick-InAmp (commonly used with the T4)
 *
 * Relevant Documentation:
 *
 * Thermocouple App-Note:
 *		https://labjack.com/support/app-notes/thermocouples
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
 *	TCVoltsToTemp:
 *		https://labjack.com/support/software/api/ud/function-reference/tcvoltstotemp
 *
 * T-Series and I/O:
 *	Modbus Map:
 *		https://labjack.com/support/software/api/modbus/modbus-map
 *	Analog Inputs:
 *		https://labjack.com/support/datasheets/t-series/ain
 *	Thermocouple AIN_EF:
 *		https://labjack.com/support/datasheets/t-series/ain/extended-features/thermocouple
**/

// For printf
#include <stdio.h>

// For the LabJackM Library
#include <LabJackM.h>

// For LabJackM helper functions, such as OpenOrDie, PrintDeviceInfoFromHandle,
// ErrorCheck, etc.
#include "../../LJM_Utilities.h"

// Gain set for Tick-InAmp (for T4).
const int INAMP_GAIN = 51;
// Offset voltage setting on Tick-InAmp (for T4).
const double INAMP_OFFSET = 0.4;

typedef enum {DEGK='K', DEGC='C', DEGF='F'} TempUnits;

typedef struct {
	// Supported TC types are:
	//     LJM_ttB (val=6001)
	//     LJM_ttE (val=6002)
	//     LJM_ttJ (val=6003)
	//     LJM_ttK (val=6004)
	//     LJM_ttN (val=6005)
	//     LJM_ttR (val=6006)
	//     LJM_ttS (val=6007)
	//     LJM_ttT (val=6008)
	//     LJM_ttC (val=6009)
	// Note that the values above do not align with the AIN_EF index values
	// or order. In this example, we demonstrate a lookup table to convert
	// our thermocouple constant to the correct index when using the AIN_EF
	long tcType;

	// If taking a differential reading on a T7, posChannel should be an even
    // numbered AIN connecting signal+, and signal- should be connected to
    // the positive AIN channel plus one.
    // Example: signal+=posChannel=0 (AIN0), signal-=negChannel=1 (AIN1)
	int posChannel;

	// negChannel value of 199 indicates single ended measurement
	// This config does nothing for the T4 and T8
	int negChannel;

	// Modbus address to read the CJC sensor at.
	int CJCAddress;

	// Slope of CJC voltage to Kelvin conversion (K/volt). TEMPERATURE_DEVICE_K
	// returns temp in K, so this would be set to 1 if using it for CJC. If
	// using an LM34 on some AIN for CJC, this config should be 55.56
	float CJCSlope;

	// Offset for CJC temp (in Kelvin).This would normally be 0 if reading the
	// register TEMPERATURE_DEVICE_K for CJC. If using an InAmp or expansion
	// board, the CJ might be a bit cooler than the internal temp sensor, so
	// you might adjust the offset down a few degrees. If using an LM34 on some
	// AIN for CJC, this config should be 255.37
	float CJCOffset;


	const TempUnits tempUnits;
}TCData;

void SetupAIN_EF(int handle, TCData tcData) {
	int err;
	// For converting LJM TC type constant to TC AIN_EF index
	// Thermocouple type:		 B  E  J  K  N  R  S  T  C
	const int TC_INDEX_LUT[9] = {28,20,21,22,27,23,25,24,30};
	enum{ NUM_FRAMES = 5 };
	int aAddresses[NUM_FRAMES];
	int aTypes[NUM_FRAMES];
	double aValues[NUM_FRAMES];
	int errorAddress = INITIAL_ERR_ADDRESS;

	// For setting up the AIN#_EF_INDEX (thermocouple type)
	aAddresses[0] = 9000+2*tcData.posChannel;
	aTypes[0] = LJM_UINT32;
	aValues[0] = TC_INDEX_LUT[tcData.tcType - 6001];

	// For setting up the AIN#_EF_CONFIG_A (temperature units)
	aAddresses[1] = 9300+2*tcData.posChannel;
	aTypes[1] = LJM_UINT32;
	switch(tcData.tempUnits) {
		case DEGK:
			aValues[1] = 0;
			break;
		case DEGC:
			aValues[1] = 1;
			break;
		case DEGF:
			aValues[1] = 2;
			break;
	}

	// For setting up the AIN#_EF_CONFIG_B (CJC address)
	aAddresses[2] = 9600+2*tcData.posChannel;
	aTypes[2] = LJM_UINT32;
	aValues[2] = tcData.CJCAddress;

	// For setting up the AIN#_EF_CONFIG_D (CJC slope)
	aAddresses[3] = 10200+2*tcData.posChannel;
	aTypes[3] = LJM_FLOAT32;
	aValues[3] = tcData.CJCSlope;

	// For setting up the AIN#_EF_CONFIG_E (CJC offset)
	aAddresses[4] = 10500+2*tcData.posChannel;
	aTypes[4] = LJM_FLOAT32;
	aValues[4] = tcData.CJCOffset;

	err = LJM_eWriteAddresses(handle, NUM_FRAMES, aAddresses, aTypes,
		aValues, &errorAddress);
	ErrorCheckWithAddress(err, errorAddress,
		"SetupAIN_EF");
}

void GetReadingsInAmp(int handle, TCData tcData) {
	int err;
	double TCTemp, TCVolts, CJTemp;

	err = LJM_eReadAddress(handle, 2*tcData.posChannel, LJM_FLOAT32, &TCVolts);
	ErrorCheck(err, "GetReadingsInAmp: Reading TC AIN");

	// Account for LJTick-InAmp scaling
	TCVolts = (TCVolts - INAMP_OFFSET) / INAMP_GAIN;

	err = LJM_eReadAddress(handle, tcData.CJCAddress, LJM_FLOAT32, &CJTemp);
	ErrorCheck(err, "GetReadingsInAmp: Reading CJC sensor");

	// Apply scaling to CJC reading if necessary
	// At this point, the reading must be in units Kelvin
	CJTemp = CJTemp * tcData.CJCSlope + tcData.CJCOffset;

	// Convert voltage reading to the thermocouple temperature.
	err = LJM_TCVoltsToTemp(tcData.tcType, TCVolts, CJTemp, &TCTemp);
	ErrorCheck(err, "GetReadingsInAmp: Calculating TCTemp(K)");

	// Convert to temp units for display
	switch(tcData.tempUnits){
		case DEGK:
			// Nothing to do
			break;

		case DEGC:
			TCTemp = TCTemp-273.15;
			CJTemp = CJTemp-273.15;
			break;

		case DEGF:
			TCTemp = (1.8*TCTemp)-459.67;
			CJTemp = (1.8*CJTemp)-459.67;
			break;
	}
	printf("TCTemp: %lf %c,\t TCVolts: %lf,\tCJTemp: %lf %c\n",
		TCTemp, (char)tcData.tempUnits, TCVolts, CJTemp, (char)tcData.tempUnits);
}

void GetReadingsAIN_EF(int handle, TCData tcData) {
	int err;
	double TCTemp, TCVolts, CJTemp;

	err = LJM_eReadAddress(handle, 7300+2*tcData.posChannel, LJM_FLOAT32, &TCVolts);
	ErrorCheck(err, "GetReadingsAIN_EF: Reading TC Volts");

	err = LJM_eReadAddress(handle, 7600+2*tcData.posChannel, LJM_FLOAT32, &CJTemp);
	ErrorCheck(err, "GetReadingsAIN_EF: Reading CJC temperature");

	err = LJM_eReadAddress(handle, 7000+2*tcData.posChannel, LJM_FLOAT32, &TCTemp);
	ErrorCheck(err, "GetReadingsAIN_EF: Reading TC Temperature");

	printf("TCTemp: %lf %c,\t TCVolts: %lf,\tCJTemp: %lf %c\n",
		TCTemp, (char)tcData.tempUnits, TCVolts, CJTemp, (char)tcData.tempUnits);
}

int main()
{
	int err, handle, i, deviceType, connectionType, serialNumber;
	int ipAddress, portOrPipe, packetMaxBytes;
	// initialize tcData struct to valid values.
	TCData tcData = {
		LJM_ttK,	// Type K thermocouple
		0, 			// Connected to AIN0
		199, 		// GND for negChannel (should be ignored for T4/T8)
		60052,		// Use TEMPERATURE_DEVICE_K for CJC
		1,			// CJC Slope associated to TEMPERATURE_DEVICE_K
		0, 			// CJC Offset associated to TEMPERATURE_DEVICE_K,
		DEGC		// Temperature units
	};

	// Open first found LabJack
	handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
	// handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

	// Get device info
	err = LJM_GetHandleInfo(handle, &deviceType, &connectionType, &serialNumber, &ipAddress,
		&portOrPipe, &packetMaxBytes);
	ErrorCheck(err, "LJM_GetHandleInfo");

	PrintDeviceInfo(deviceType, connectionType, serialNumber, ipAddress, portOrPipe, packetMaxBytes);

	// Set the resolution index to the default setting (value=0)
	// Default setting has different meanings depending on the device.
	// See our AIN documentation (link above) for more information
	err = LJM_eWriteAddress(handle, 41500+tcData.posChannel, LJM_UINT16, 0);
	ErrorCheck(err, "Setting AIN resolution index");

	// Only set up the negative channel config if using a T7
	if(deviceType == LJM_dtT7){
		err = LJM_eWriteAddress(handle, 41000+tcData.posChannel,
			LJM_UINT16, tcData.negChannel);
		ErrorCheck(err, "Setting T7 negChannel");
	}
	// Set up the AIN_EF if using a T7/T8
	if(deviceType != LJM_dtT4) {
		SetupAIN_EF(handle, tcData);
	}


	printf("\nPress ctrl + c to stop\n");
	for (i=0; ; i++) {
		if(deviceType == LJM_dtT4) { // Assumed that the InAmp is used with T4
			GetReadingsInAmp(handle, tcData);
		}
		else { // Otherwise use AIN_EF
			GetReadingsAIN_EF(handle, tcData);
		}
		MillisecondSleep(1000);
	}
	WaitForUserIfWindows();

	return LJME_NOERROR;
}
