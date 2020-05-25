cmake_minimum_required(VERSION ${TacentMinCMake})

# Setup configurations before specifying the project.
find_package("SetupTacentConfigurations")

# Give the project a name and specify explicitly the launguages used.
project(Scene VERSION ${TacentVersion} LANGUAGES CXX)

# Here are the source files. No globbing. We include the headers here so that some generators can include
# them properly, but they are not required. Currently SetupTacentProjectProperties requires a variable
# called src. We should probably make the module just expose some common functions so the structure of this
# CMakeLists looks a little more familiar.
list (APPEND src
    Src/tAttribute.cpp
    Src/tCamera.cpp
    Src/tInstance.cpp
    Src/tLight.cpp
    Src/tLodGroup.cpp
    Src/tMaterial.cpp
    Src/tMesh.cpp
    Src/tObject.cpp
    Src/tPath.cpp
    Src/tPolyModel.cpp
    Src/tSelection.cpp
    Src/tSkeleton.cpp
    Src/tWorld.cpp
    Inc/Scene/tAttribute.h
    Inc/Scene/tCamera.h
    Inc/Scene/tInstance.h
    Inc/Scene/tLight.h
    Inc/Scene/tLodGroup.h
    Inc/Scene/tMaterial.h
    Inc/Scene/tMesh.h
    Inc/Scene/tObject.h
    Inc/Scene/tPath.h
    Inc/Scene/tPolyModel.h
    Inc/Scene/tSelection.h
    Inc/Scene/tSkeleton.h
    Inc/Scene/tWorld.h
)

# Use a SetupTacentProjectProperties common file to call add_library with common build characteristics.
find_package("SetupTacentProjectProperties" REQUIRED)

# Public means dependent interfaces are propagated. Private means they are not. Here we are saying the
# Tacent System module depends-on/requires the Foundation and Math module.
# Note that since Math happens to also depend on Foundation, we could have left Foundation out and it would work.
target_link_libraries(Scene
	PUBLIC
		Foundation Math System
)
