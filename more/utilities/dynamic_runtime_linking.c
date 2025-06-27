/**
 * Name: dynamic_runtime_linking.c
 * Desc: Shows how to load LJM at run-time rather than at load-time for Windows,
 * Linux, and macOS.
 * 
 * Windows
 *
 *   To dynamically link LJM during run-time, use the function LoadLibrary to
 *   load LabJackM.dll, then use the function GetProcAddress to get the
 *   addressses of given LabJackM functions as needed. See the Microsoft
 *   article "Using Run-Time Dynamic Linking" for more information:
 *
 *   https://msdn.microsoft.com/en-us/library/windows/desktop/ms686944%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
 *
 * Linux
 *
 *   To dynamically link LJM during run-time, use the function dlopen to load
 *   LabJackM.so, then use the function dlsym to get the addresses of given
 *   LabJackM functions as needed. See `man dlopen` and `man dlsym` for more
 *   information, or see this article:
 *
 *   http://tldp.org/HOWTO/Program-Library-HOWTO/dl-libraries.html
 *
 * macOS
 *
 *   To dynamically link LJM during run-time, use the function dlopen to load
 *   LabJackM.dylib, then use the function dlsym to get the addresses of given
 *   LabJackM functions as needed. See the Apple Developer documentation for
 *   "Using Runtime-Loaded Libraries":
 *
 *   https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/DynamicLibraries/100-Articles/UsingDynamicLibraries.html#//apple_ref/doc/uid/TP40002182-SW13
 *
**/

#include <stdio.h>
#include <stdlib.h> // For exit

#ifdef _WIN32
    //Windows specific
    #include "Windows.h"

    #define LJM_LIB_NAME "LabJackM.dll"

    #define LOAD_LIBRARY(libname) LoadLibrary(TEXT(libname))
    #define GET_PROC_ADDRESS(lib_handle, func_name) GetProcAddress((HMODULE)lib_handle, func_name)
    #define FREE_LIBRARY(lib_handle) FreeLibrary((HMODULE)lib_handle)

    typedef HMODULE LIB_HANDLE;
#else
    //Linux and macOS specific
    #include <dlfcn.h>

    #ifdef __APPLE__
        #define LJM_LIB_NAME "LabJackM.dylib"
    #else
        #define LJM_LIB_NAME "LabJackM.so"
    #endif

    #define LOAD_LIBRARY(libname) dlopen(libname, RTLD_LAZY)
    #define GET_PROC_ADDRESS(lib_handle, func_name) dlsym(lib_handle, func_name)
    #define FREE_LIBRARY(lib_handle) dlclose(lib_handle)

    typedef void* LIB_HANDLE;
#endif

// Function pointer types for LJM functions
typedef int (*OPENSFUNCTIONTYPE)(
    const char * DeviceType,
    const char * ConnectionType,
    const char * Identifier,
    int * Handle
);

typedef int (*EREADNAMEFUNCTIONTYPE)(
    int Handle,
    const char * Name,
    double * Value
);

typedef int (*CLOSEFUNCTIONTYPE) (int Handle);

void ErrorCheck(int err, const char * function)
{
    if (err != 0) {
        printf("Error on %s:  %d\n", function, err);
        printf("Press enter to continue\n");
        getchar();
        exit(err);
    }
}

int main()
{
    LIB_HANDLE hinstLib;
    OPENSFUNCTIONTYPE OpenSAddress;
    EREADNAMEFUNCTIONTYPE EReadNameAddress;
    CLOSEFUNCTIONTYPE CloseAddress;
    int err = 0;
    int handle = 0;
    const char * function;
    const char * NAME = "SERIAL_NUMBER";
    double value = 0.0;
    int fRunTimeLinkSuccess = 1;

    // Get a handle to the LJM library.
    hinstLib = LOAD_LIBRARY(LJM_LIB_NAME);

    // If the handle is valid, continue to get the function addresses.
    if (hinstLib != NULL) {
        // Get the LJM function addresses and check if they are valid.
        OpenSAddress = (OPENSFUNCTIONTYPE) GET_PROC_ADDRESS(hinstLib, "LJM_OpenS");
        EReadNameAddress = (EREADNAMEFUNCTIONTYPE) GET_PROC_ADDRESS(hinstLib, "LJM_eReadName");
        CloseAddress = (CLOSEFUNCTIONTYPE) GET_PROC_ADDRESS(hinstLib, "LJM_Close");
        if (NULL == OpenSAddress || NULL == EReadNameAddress || NULL == CloseAddress) {
            fRunTimeLinkSuccess = 0;
        }

        if(fRunTimeLinkSuccess) {
            // Open first found LabJack.
            function = "LJM_OpenS";
            err = (OpenSAddress) ("LJM_dtANY", "LJM_ctANY", "LJM_idANY", &handle);
            ErrorCheck(err, function);

            // Read the serial number from the LabJack.
            function = "LJM_eReadName";
            err = (EReadNameAddress) (handle, NAME, &value);
            ErrorCheck(err, function);
            printf("\nLJM_eReadName result - %s: %f\n", NAME, value);

            // Close the LabJack handle.
            function = "LJM_Close";
            err = (CloseAddress) (handle);
            ErrorCheck(err, function);
        }

        // Free the LJM library.
        FREE_LIBRARY(hinstLib);

        // If unable to call the LJM functions, print an error message.
        if (! fRunTimeLinkSuccess) {
            printf("Error finding at least one LJM function.\nPlease install LJM "
                   "at https://labjack.com/support/software/installers/ljm \n");
        }
    }
    else {
        printf("Error loading the LJM library.\nPlease install LJM "
               "at https://labjack.com/support/software/installers/ljm \n");
    }

    printf("Press enter to continue\n");
    getchar();

    return 0;
}
