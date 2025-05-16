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
