cmake_minimum_required(VERSION ${TacentMinCMake})

# Setup configurations before specifying the project.
find_package("SetupTacentConfigurations")

# Give the project a name and specify explicitly the launguages used.
project(Foundation VERSION ${TacentVersion} LANGUAGES CXX)

# Here are the source files. No globbing. We include the headers here so that some generators can include
# them properly, but they are not required. Currently SetupTacentProjectProperties requires a variable
# called src. We should probably make the module just expose some common functions so the structure of this
# CMakeLists looks a little more familiar.
list(
	APPEND src
	Src/tAssert.cpp
	Src/tBitArray.cpp
	Src/tMemory.cpp
	Src/tPlatform.cpp
	Src/tPool.cpp
	Src/tStandard.cpp
	Src/tString.cpp
	Src/tUnits.cpp
	Inc/Foundation/tArray.h
	Inc/Foundation/tAssert.h
	Inc/Foundation/tBitArray.h
	Inc/Foundation/tBitField.h
	Inc/Foundation/tFixInt.h
	Inc/Foundation/tList.h
	Inc/Foundation/tMemory.h
	Inc/Foundation/tPlatform.h
	Inc/Foundation/tPool.h
	Inc/Foundation/tPriorityQueue.h
	Inc/Foundation/tRingBuffer.h
	Inc/Foundation/tSort.h
	Inc/Foundation/tStandard.h
	Inc/Foundation/tString.h
	Inc/Foundation/tUnits.h
	Inc/Foundation/tVersion.h
)

# Use a SetupTacentProjectProperties common file to call add_library with common build characteristics.
find_package("SetupTacentProjectProperties" REQUIRED)

# If we had dependencies, we would use target_link_libraries
