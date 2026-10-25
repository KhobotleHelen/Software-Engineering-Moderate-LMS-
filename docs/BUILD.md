# Build and run guide

## Requirements
C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+), CMake 3.16+, SQLite3 dev files, libsodium dev files,
GoogleTest (tests only; CMake downloads it automatically if it is not installed).

| OS | Install dependencies |
|---|---|
| Ubuntu/Debian | `sudo apt install build-essential cmake libsqlite3-dev libsodium-dev libgtest-dev` |
| macOS (Homebrew) | `brew install cmake sqlite libsodium googletest` |
| Windows (vcpkg) | `vcpkg install sqlite3 libsodium gtest` then add `-DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake` |

## Build
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```
Options: `-DLMS_BUILD_TESTS=OFF` (skip tests), `-DLMS_COVERAGE=ON` (gcov), `-DLMS_WERROR=ON`.

## Run
```
./build/lms                # uses ./lms.db (created on first run; you are asked to create the administrator)
./build/lms /path/to/school.db
```
Demo data (800 students, ~45 s because every password is hashed): `./build/lms_seed demo.db 800`, then
`./build/lms demo.db`. Accounts: admin, lecturer1..5, hod_sci, hod_hum, student1..800, password `Password123`.

## Test, coverage, static analysis
```
ctest --test-dir build --output-on-failure
cmake -B build-cov -DLMS_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug && cmake --build build-cov
./build-cov/lms_tests
gcovr -r . build-cov --filter 'src/services/' --filter 'src/strategy/' --filter 'src/domain/' --filter 'src/common/' --filter 'src/security/' --txt
cppcheck --enable=all --std=c++17 -Isrc src tools
```
Errors that are not shown on screen (database problems) are appended to `lms_error.log` in the working folder.
