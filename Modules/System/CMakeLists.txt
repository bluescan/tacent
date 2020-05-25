cmake_minimum_required(VERSION ${TacentMinCMake})

# Setup configurations before specifying the project.
find_package("SetupTacentConfigurations")

# Give the project a name and specify explicitly the launguages used.
project(System VERSION ${TacentVersion} LANGUAGES CXX)

# Here are the source files. No globbing. We include the headers here so that some generators can include
# them properly, but they are not required. Currently SetupTacentProjectProperties requires a variable
# called src. We should probably make the module just expose some common functions so the structure of this
# CMakeLists looks a little more familiar.
list (APPEND src
    Src/tChunk.cpp
    Src/tCommand.cpp
    Src/tFile.cpp
    Src/tMachine.cpp
    Src/tPrint.cpp
    Src/tRegex.cpp
    Src/tScript.cpp
    Src/tTask.cpp
    Src/tThrow.cpp
    Src/tTime.cpp
    Inc/System/tChunk.h
    Inc/System/tCommand.h
    Inc/System/tFile.h
    Inc/System/tMachine.h
    Inc/System/tPrint.h
    Inc/System/tRegex.h
    Inc/System/tScript.h
    Inc/System/tStream.h
    Inc/System/tTask.h
    Inc/System/tThrow.h
    Inc/System/tTime.h
)

# Use a SetupTacentProjectProperties common file to call add_library with common build characteristics.
find_package("SetupTacentProjectProperties" REQUIRED)

# Public means dependent interfaces are propagated. Private means they are not. Here we are saying the
# Tacent System module depends-on/requires the Foundation and Math module.
# Note that since Math happens to also depend on Foundation, we could have left Foundation out and it would work.
target_link_libraries(System
	PUBLIC
		Foundation Math
)
