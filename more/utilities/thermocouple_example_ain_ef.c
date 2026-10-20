/**
 * Name: thermocouple_example_ain_ef.c
 * Desc: Demonstrates thermocouple configuration and measurement using the
 *      thermocouple AIN_EF features. This example is for devices such as
 *      the T7 and T8 that support built-in amplification and the thermocouple
 *      AIN_EF. For devices without built-in amplification, such as the T4,
 *      see 'thermocouple_example_inamp.c' instead.
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

// Enum representing the thermocouple type (AIN#_EF_INDEX).
// Note that these values are different from the LJM library thermocouple type
// constants, f.e. LJM_ttK. Those are intended for the TCVoltsToTemp function.
typedef enum {
    ETYPE=20,
    JTYPE=21,
    KTYPE=22,
    RTYPE=23,
    TTYPE=24,
    STYPE=25,
    NTYPE=27,
    BTYPE=28,
    CTYPE=30
} TCType;

typedef struct {
    TCType tcType;

    // AIN number to get the thermocouple reading, f.e. 0 for AIN0.
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

    const TempUnits tempUnits;
} TCData;

void SetupAIN_EF(int handle, TCData tcData) {
    const int CONFIG_A_BASE_ADDR = 9300;
    const int CONFIG_B_BASE_ADDR = 9600;
    const int CONFIG_D_BASE_ADDR = 10200;
    const int CONFIG_E_BASE_ADDR = 10500;
    int err;
    enum{ NUM_FRAMES = 5 };
    int aAddresses[NUM_FRAMES];
    int aTypes[NUM_FRAMES];
    double aValues[NUM_FRAMES];
    int errorAddress = INITIAL_ERR_ADDRESS;

    // For setting up the AIN#_EF_INDEX (thermocouple type).
    aAddresses[0] = 9000+2*tcData.ainNumber;
    aTypes[0] = LJM_UINT32;
    aValues[0] = tcData.tcType;

    // For setting up the AIN#_EF_CONFIG_A (temperature units).
    aAddresses[1] = CONFIG_A_BASE_ADDR+2*tcData.ainNumber;
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

    // For setting up the AIN#_EF_CONFIG_B (CJC address).
    aAddresses[2] = CONFIG_B_BASE_ADDR+2*tcData.ainNumber;
    aTypes[2] = LJM_UINT32;
    aValues[2] = tcData.CJCAddress;

    // For setting up the AIN#_EF_CONFIG_D (CJC slope).
    aAddresses[3] = CONFIG_D_BASE_ADDR+2*tcData.ainNumber;
    aTypes[3] = LJM_FLOAT32;
    aValues[3] = tcData.CJCSlope;

    // For setting up the AIN#_EF_CONFIG_E (CJC offset).
    aAddresses[4] = CONFIG_E_BASE_ADDR+2*tcData.ainNumber;
    aTypes[4] = LJM_FLOAT32;
    aValues[4] = tcData.CJCOffset;

    err = LJM_eWriteAddresses(
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
        "SetupAIN_EF"
    );
}

void GetReadingsAIN_EF(int handle, TCData tcData) {
    const int READ_A_BASE_ADDR = 7000;
    const int READ_B_BASE_ADDR = 7300;
    const int READ_C_BASE_ADDR = 7600;
    int err;
    double TCTemp, TCVolts, CJTemp;
    enum{ NUM_FRAMES = 3 };
    int aAddresses[NUM_FRAMES];
    int aTypes[NUM_FRAMES];
    double aValues[NUM_FRAMES] = {0,0,0};
    int errorAddress = INITIAL_ERR_ADDRESS;

    // For reading TC Temperature.
    aAddresses[0] = READ_A_BASE_ADDR+2*tcData.ainNumber;
    aTypes[0] = LJM_FLOAT32;

    // For reading TC Volts.
    aAddresses[1] = READ_B_BASE_ADDR+2*tcData.ainNumber;
    aTypes[1] = LJM_FLOAT32;

    // For reading CJC Temperature.
    aAddresses[2] = READ_C_BASE_ADDR+2*tcData.ainNumber;
    aTypes[2] = LJM_FLOAT32;

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
    TCTemp = aValues[0];
    TCVolts = aValues[1];
    CJTemp = aValues[2];

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
    const int AIN_RANGE_BASE_ADDR = 40000;
    const int RESOLUTION_INDEX_BASE_ADDR = 41500;
    const int NEGATIVE_CH_BASE_ADDR = 41000;
    int err, handle, i, deviceType, connectionType, serialNumber;
    int ipAddress, portOrPipe, packetMaxBytes;
    // Initialize tcData struct to valid values.
    // Note that CJC address can be acquired from the register name using
    // LJM_NameToAddress. Address and type are int values:
    // LJM_NameToAddress("TEMPERATURE_AIR_K", &address, &type);
    TCData tcData = {
        KTYPE,      // Type K thermocouple.
        0,          // Thermocouple connected to AIN0.
        60052,      // Use TEMPERATURE_DEVICE_K for CJC.
        1,          // CJC Slope. Use 55.56 for LM34.
        0,          // CJC Offset. Use 255.37 for LM34.
        DEGC // Output temperature units.
    };

    // Open first found LabJack.
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

    SetupAIN_EF(handle, tcData);

    // If you set range according to the max TC voltage amplitude,
    // the device will use the best/closest setting available.
    err = LJM_eWriteAddress(
        handle,
        AIN_RANGE_BASE_ADDR+tcData.ainNumber,
        LJM_FLOAT32,
        0.07 // Will use ±0.1V on T7, ±0.075V on T8.
    );
    ErrorCheck(err, "Setting AIN range");
    // Set the resolution index to the default setting (value=0).
    err = LJM_eWriteAddress(
        handle,
        RESOLUTION_INDEX_BASE_ADDR+tcData.ainNumber,
        LJM_UINT16,
        0
    );
    ErrorCheck(err, "Setting AIN resolution index");

    // Only set up the negative channel config if using a T7.
    if(deviceType == LJM_dtT7){
        err = LJM_eWriteAddress(
            handle,
            NEGATIVE_CH_BASE_ADDR+tcData.ainNumber,
            LJM_UINT16,
            LJM_GND // 199, single ended measurement
        );
        ErrorCheck(err, "Setting T7 negChannel");
    }

    printf("\nPress ctrl + c to stop\n");
    for (i=0; ; i++) {
        GetReadingsAIN_EF(handle, tcData);
        MillisecondSleep(1000);
    }
    WaitForUserIfWindows();

    return LJME_NOERROR;
}
