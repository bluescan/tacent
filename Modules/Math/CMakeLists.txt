cmake_minimum_required(VERSION ${TacentMinCMake})

# Setup configurations before specifying the project.
find_package("SetupTacentConfigurations")

# Give the project a name and specify explicitly the launguages used.
project(Math VERSION ${TacentVersion} LANGUAGES CXX)

# Here are the source files. No globbing. We include the headers here so that some generators can include
# them properly, but they are not required. Currently SetupTacentProjectProperties requires a variable
# called src. We should probably make the module just expose some common functions so the structure of this
# CMakeLists looks a little more familiar.
list (APPEND src
	Src/tColour.cpp
	Src/tGeometry.cpp
	Src/tHash.cpp
	Src/tLinearAlgebra.cpp
	Src/tRandom.cpp
	Src/tSpline.cpp
	Inc/Math/tColour.h
	Inc/Math/tConstants.h
	Inc/Math/tFundamentals.h
	Inc/Math/tGeometry.h
	Inc/Math/tHash.h
	Inc/Math/tLinearAlgebra.h
	Inc/Math/tMatrix2.h
	Inc/Math/tMatrix4.h
	Inc/Math/tQuaternion.h
	Inc/Math/tRandom.h
	Inc/Math/tSpline.h
	Inc/Math/tVector2.h
	Inc/Math/tVector3.h
	Inc/Math/tVector4.h
)

# Use a SetupTacentProjectProperties common file to call add_library with common build characteristics.
find_package("SetupTacentProjectProperties" REQUIRED)

# Public means dependent interfaces are propagated. Private means they are not. Here we are saying the
# Tacent Math module depends-on/requires the Foundation module.
target_link_libraries(Math
	PUBLIC
		Foundation
)
