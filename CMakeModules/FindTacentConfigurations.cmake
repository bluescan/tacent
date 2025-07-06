# set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

# We want control of the available configurations. Here, before project, we define
# Debug, Develop, Profile, Release, and Ship.
if (NOT SETUP_CONFIGURATIONS_COMPLETE)

	# No reason to set CMAKE_CONFIGURATION_TYPES if it's not a multiconfig generator
	# Also no reason mess with CMAKE_BUILD_TYPE if it's a multiconfig generator.
	get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

	if (isMultiConfig)
		set(CMAKE_CONFIGURATION_TYPES "Debug;Develop;Profile;Release;Ship" CACHE STRING "" FORCE) 
	else()
		if (NOT DEFINED CMAKE_BUILD_TYPE)
			message(STATUS "Tacent -- Default to Release build.")
			set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose Build Type" FORCE)
		endif()
		message(STATUS "Tacent -- Build type set to: ${CMAKE_BUILD_TYPE}")
		set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose Build Type")

		# Set the valid options for cmake-gui drop-down list. CMake tools for vscode does not (but should) respect this.
		set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Develop" "Profile" "Release" "Ship")	
	endif()

	set(SETUP_CONFIGURATIONS_COMPLETE TRUE)
endif()
