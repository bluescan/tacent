# This is the first module cmake file. It has more comments than subsequent cmakelists.

# Include the utilities module.
find_package("TacentProjectUtilities" REQUIRED)

# Give the project a name and specify explicitly the launguages used.
project(Foundation VERSION ${TACENT_VERSION} LANGUAGES CXX)

# Here are the source files. No globbing. We include the headers here so that some generators can include
# them properly, but they are not required. Currently SetupTacentProjectProperties requires a variable
# called src. We should probably make the module just expose some common functions so the structure of this
# CMakeLists looks a little more familiar.
add_library(
	${PROJECT_NAME}
	Src/tVersion.cpp
	Src/tAssert.cpp
	Src/tBitArray.cpp
	Src/tHash.cpp
	Src/tMemory.cpp
	Src/tPlatform.cpp
	Src/tPool.cpp
	Src/tStandard.cpp
	Src/tString.cpp
	Src/tUnits.cpp
	Inc/Foundation/tVersion.cmake.h
	Inc/Foundation/tArray.h
	Inc/Foundation/tAssert.h
	Inc/Foundation/tBitArray.h
	Inc/Foundation/tBitField.h
	Inc/Foundation/tConstants.h
	Inc/Foundation/tFixInt.h
	Inc/Foundation/tFundamentals.h
	Inc/Foundation/tHash.h
	Inc/Foundation/tList.h
	Inc/Foundation/tMap.h
	Inc/Foundation/tMemory.h
	Inc/Foundation/tPlatform.h
	Inc/Foundation/tPool.h
	Inc/Foundation/tPriorityQueue.h
	Inc/Foundation/tRingBuffer.h
	Inc/Foundation/tSmallFloat.h
	Inc/Foundation/tSort.h
	Inc/Foundation/tStandard.h
	Inc/Foundation/tString.h
	Inc/Foundation/tUnits.h
)

tacent_target_include_directories(${PROJECT_NAME})
tacent_target_compile_definitions(${PROJECT_NAME})
tacent_target_compile_options(${PROJECT_NAME})
tacent_target_compile_features(${PROJECT_NAME})
tacent_set_target_properties(${PROJECT_NAME})

# Library dependencies.
tacent_is_arch_arm(IsArm)
target_link_libraries(${PROJECT_NAME} PUBLIC
	# I'm a little surprised cmake doesn't do these for me on Linux. The odd thing is they are
	# unnecessary if using VSCode/Cmake Tools, even in Ubuntu. They are, however, needed when
	# building from bash.
	$<$<PLATFORM_ID:Linux>:m>
	$<$<PLATFORM_ID:Linux>:stdc++>
	$<$<AND:$<BOOL:${IsArm}>,$<PLATFORM_ID:Linux>>:stdc++fs>
)

tacent_install(${PROJECT_NAME})
