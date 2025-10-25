# Movie Ticket Booking Backend (C++17)

Thread-safe, in-memory backend for booking movie tickets (no DB). Provides a clean C++ API, CLI demo, and unit tests.

## Technical Insights & Demo document:
BookingService_TechnicalBrief_and_DemoGuide.pdf

## Build & Run
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/bin/booking_cli 
```

## Run Unit Test cases
```bash
ctest --test-dir build -C Release --output-on-failure
./build/booking_tests --help
# Run all test cases
./build/booking_tests
# Run specific test case using filter
./build/booking_tests --filter=[filter]
# Run the given concurrency stress test using configurable number of threads
./build/booking_tests --threads=[threads] 
```

## Docker (optional)
```bash
docker build -t booking-cpp .
docker run --rm booking-cpp
```

## Conan (optional)
```bash
conan profile detect
conan install . -of build --build=missing
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
cmake --build build
ctest --test-dir build --output-on-failure
```

## Doxygen (optional)
```bash
doxygen Doxyfile
# open docs/html/index.html
```
