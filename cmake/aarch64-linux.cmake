# Cross-compiling to aarch64 Linux (ROCKNIX on the RG DS / RG DS Plus, whose
# RK3568 has Cortex-A55 cores). Uses aarch64-linux-gnu-gcc/g++ (g++ only for SDL2) from the PATH, or
# from $AARCH64_TOOLCHAIN_BIN.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
if(DEFINED ENV{AARCH64_TOOLCHAIN_BIN})
	set(CMAKE_C_COMPILER $ENV{AARCH64_TOOLCHAIN_BIN}/aarch64-linux-gnu-gcc)
	set(CMAKE_CXX_COMPILER $ENV{AARCH64_TOOLCHAIN_BIN}/aarch64-linux-gnu-g++)
else()
	set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
	set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
endif()
set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-a55")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-a55")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
