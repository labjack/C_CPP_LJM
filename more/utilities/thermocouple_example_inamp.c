/**
 * Name: thermocouple_example_inamp.c
 * Desc: Demonstrates thermocouple configuration and measurement using the
 *      LJTick-InAmp (commonly used with the T4). If you are using a device
 *      such as the T7 or T8 which supports the thermocouple AIN_EF, see
 *      'thermocouple_example_ain_ef.c' instead.
 *
 * Relevant Documentation:
 *
 * Thermocouple App-Note:
 *      https://labjack.com/support/app-notes/thermocouples
 *
 * LJM Library:
 *  LJM Library Installer:
 *      https://labjack.com/support/software/installers/ljm
 *  LJM Users Guide:
 *      https://labjack.com/support/software/api/ljm
 *  Opening and Closing:
 *      https://labjack.com/support/software/api/ljm/function-reference/opening-and-closing
 *  Single Value Functions(such as eReadName):
 *      https://labjack.com/support/software/api/ljm/function-reference/single-value-functions
 *  TCVoltsToTemp:
 *      https://labjack.com/support/software/api/ud/function-reference/tcvoltstotemp
 *
 * T-Series and I/O:
 *  Modbus Map:
 *      https://labjack.com/support/software/api/modbus/modbus-map
 *  Analog Inputs:
 *      https://labjack.com/support/datasheets/t-series/ain
 *  Thermocouple AIN_EF:
 *      https://labjack.com/support/datasheets/t-series/ain/extended-features/thermocouple
**/

// For printf.
#include <stdio.h>

// For the LabJackM Library.
#include <LabJackM.h>

// For LabJackM helper functions, such as OpenOrDie, PrintDeviceInfoFromHandle,
// ErrorCheck, etc.
#include "../../LJM_Utilities.h"

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
    long tcType;

    // AIN channel to read the thermocouple/InAmp voltage.
    int ainNumber;

    // Modbus address to read the CJC sensor at.
    int CJCAddress;

    // Slope of CJC voltage to Kelvin conversion (K/volt). TEMPERATURE_DEVICE_K
    // returns temp in K, so this would be set to 1 if using it for CJC. If
    // using an LM34 on some AIN for CJC, this config should be 55.56.
    float CJCSlope;

    // Offset for CJC temp (in Kelvin).This would normally be 0 if reading the
    // register TEMPERATURE_DEVICE_K for CJC. If using an InAmp or expansion
    // board, the CJ might be a bit cooler than the internal temp sensor, so
    // you might adjust the offset down a few degrees. If using an LM34 on some
    // AIN for CJC, this config should be 255.37.
    float CJCOffset;

    // Gain of the LJTick-InAmp.
    float InAmpGain;

    // Offset (V) of the LJTick-InAmp. Usually 1.25 V.
    float InAmpOffset;

    const TempUnits tempUnits;
} TCData;

void GetReadingsInAmp(int handle, TCData tcData) {
    int err;
    double TCTemp, TCVolts, CJTemp;
    enum{ NUM_FRAMES = 2 };
    int aAddresses[NUM_FRAMES];
    int aTypes[NUM_FRAMES];
    double aValues[NUM_FRAMES] = {0,0};
    int errorAddress = INITIAL_ERR_ADDRESS;

    // For reading TC Volts.
    aAddresses[0] = 2*tcData.ainNumber;
    aTypes[0] = LJM_FLOAT32;

    // For reading CJC Temperature.
    aAddresses[1] = tcData.CJCAddress;
    aTypes[1] = LJM_FLOAT32;

    err = LJM_eReadAddresses(
        handle,
        NUM_FRAMES,
        aAddresses,
        aTypes,
        aValues,
        &errorAddress
    );
    ErrorCheckWithAddress(
        err,
        errorAddress,
        "GetReadingsAIN_EF"
    );
    TCVolts = aValues[0];
    CJTemp = aValues[1];

    // Account for LJTick-InAmp scaling.
    TCVolts = (TCVolts - tcData.InAmpOffset) / tcData.InAmpGain;

    // Apply scaling to CJC reading if necessary.
    // At this point, the reading must be in units Kelvin.
    CJTemp = CJTemp * tcData.CJCSlope + tcData.CJCOffset;

    // Convert voltage reading to the thermocouple temperature.
    err = LJM_TCVoltsToTemp(tcData.tcType, TCVolts, CJTemp, &TCTemp);
    ErrorCheck(err, "GetReadingsInAmp: Calculating TCTemp(K)");

    // Convert to temp units for display.
    switch(tcData.tempUnits){
        case DEGK:
            // Nothing to do.
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
    printf(
        "TCTemp: %lf %c,\t TCVolts: %lf,\tCJTemp: %lf %c\n",
        TCTemp,
        (char)tcData.tempUnits,
        TCVolts,
        CJTemp,
        (char)tcData.tempUnits
    );
}

int main()
{
    int err, handle, i, deviceType, connectionType, serialNumber;
    int ipAddress, portOrPipe, packetMaxBytes;
    // Initialize tcData struct to valid values.
    // Note that the CJC address can be acquired from the register name using
    // LJM_NameToAddress. Ex (address and type are int values):
    // LJM_NameToAddress("TEMPERATURE_AIR_K", &address, &type);
    TCData tcData = {
        LJM_ttK,    // Type K thermocouple.
        6,          // Thermocouple/InAmp connected to AIN6.
        60052,      // Use TEMPERATURE_DEVICE_K for CJC.
        1,          // CJC Slope. Use 55.56 for LM34.
        0,          // CJC Offset. Use 255.37 for LM34.
        51,         // Assuming x51 InAmp gain.
        1.25,       // Assuming 1.25 V InAmp Offset.
        DEGC        // Output temperature units.
    };
    const int RESOLUTION_INDEX_BASE_ADDR = 41500;

    // Open first found LabJack
    handle = OpenOrDie(LJM_dtANY, LJM_ctANY, "LJM_idANY");
    // handle = OpenSOrDie("LJM_dtANY", "LJM_ctANY", "LJM_idANY");

    // Get device info.
    err = LJM_GetHandleInfo(
        handle,
        &deviceType,
        &connectionType,
        &serialNumber,
        &ipAddress,
        &portOrPipe,
        &packetMaxBytes
    );
    ErrorCheck(err, "LJM_GetHandleInfo");

    PrintDeviceInfo(
        deviceType,
        connectionType,
        serialNumber,
        ipAddress,
        portOrPipe,
        packetMaxBytes
    );

    // Set the resolution index to the default setting (value=0).
    err = LJM_eWriteAddress(
        handle,
        RESOLUTION_INDEX_BASE_ADDR+tcData.ainNumber,
        LJM_UINT16,
        0
    );
    ErrorCheck(err, "Setting AIN resolution index");

    printf("\nPress ctrl + c to stop\n");
    for (i=0; ; i++) {
        GetReadingsInAmp(handle, tcData);
        MillisecondSleep(1000);
    }
    WaitForUserIfWindows();

    return LJME_NOERROR;
}
