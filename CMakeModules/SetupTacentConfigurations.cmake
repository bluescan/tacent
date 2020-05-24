# set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

# We want control of the available configurations. Here, before project, we allow only Debug and Release.
#set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE TYPE INTERNAL FORCE)
if (NOT SETUP_CONFIGURATIONS_COMPLETE)
	message(STATUS "Tacent -- CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

	# No reason to set CMAKE_CONFIGURATION_TYPES if it's not a multiconfig generator
	# Also no reason mess with CMAKE_BUILD_TYPE if it's a multiconfig generator.
	get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
	if (isMultiConfig)
		set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE) 
	else()
		if (NOT DEFINED CMAKE_BUILD_TYPE)
			message(STATUS "Tacent -- Default to Release build.")
			set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type" FORCE)
		endif()
	endif()

	if (NOT isMultiConfig)
		message(STATUS "Tacent -- Build type set to: ${CMAKE_BUILD_TYPE}")
	endif()

	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build")

	# set the valid options for cmake-gui drop-down list
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release")
	
	# now set up the configurations.
	#set(CMAKE_C_FLAGS_PROFILE "...")
	#set(CMAKE_CXX_FLAGS_PROFILE "...")
	set(CMAKE_CXX_FLAGS_RELEASE "-DCONFIG_RELEASE")
	#set(CMAKE_EXE_LINKER_FLAGS_PROFILE "...")
	
	set(SETUP_CONFIGURATIONS_COMPLETE TRUE)
endif()
