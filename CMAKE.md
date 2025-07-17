# Building with CMake
```
mkdir build
cd build
cmake ..
cmake --build .
```

# Testing with CMake
```
cd build
ctest -C Debug -V .
```

# Installing with CMake
```
cmake --install .
```

or

```
cmake --install . --config Debug
```

# Changing the CMake Generator
If you want to build with a Generator (C/C++ build system) other than CMake
chosen default, you can change it by selecting a different generator with
command:
```
cmake -G <generator-name>
```

cmake -G is documented here:
https://cmake.org/cmake/help/latest/manual/cmake.1.html#cmdoption-cmake-G

# macOS issue with finding libLabJackM.dylib
After building the examples, when running the example binaries/executables they
may be unable to find the LJM driver (libLabJackM.dylib) in /usr/local/lib .
macOS 14 has introduced a breaking change to rpaths. A fix for the time being is
to add "/usr/local/lib" to your "DYLD_LIBRARY_PATH" environment variable.

This can be done by adding this line to your ~/.zprofile file:
```
export DYLD_LIBRARY_PATH=/usr/local/lib:$DYLD_LIBRARY_PATH
```
Note to reboot your computer for the new DYLD_LIBRARY_PATH setting to take
effect.
