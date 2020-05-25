cmake_minimum_required(VERSION ${TacentMinCMake})

# Setup configurations before specifying the project.
find_package("SetupTacentConfigurations")

# Give the project a name and specify explicitly the launguages used.
project(Pipeline VERSION ${TacentVersion} LANGUAGES CXX)

# Here are the source files. No globbing. We include the headers here so that some generators can include
# them properly, but they are not required. Currently SetupTacentProjectProperties requires a variable
# called src. We should probably make the module just expose some common functions so the structure of this
# CMakeLists looks a little more familiar.
list (APPEND src
	Src/tProcess.cpp
	Src/tRule.cpp
	Src/tSolution.cpp
	Inc/Pipeline/tProcess.h
	Inc/Pipeline/tRule.h
	Inc/Pipeline/tSolution.h
)

# Use a SetupTacentProjectProperties common file to call add_library with common build characteristics.
find_package("SetupTacentProjectProperties" REQUIRED)

# Public means dependent interfaces are propagated. Private means they are not.
target_link_libraries(Pipeline
	PUBLIC
		Foundation Math System
)
