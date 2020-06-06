# add_library(${PROJECT_NAME} ${src})


function(tacent_target_include_directories PROJNAME)
	# Set properties on the target. Include files here. Note:
	# PRIVATE is for building only. Populates INCLUDE_DIRECTORIES
	# INTERFACE is for users of the library. Populates INTERFACE_INCLUDE_DIRECTORIES
	# PUBLIC means both building and for users of the lib. Populates both.
	target_include_directories(
		"${PROJNAME}"
		PUBLIC
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Inc>
			$<INSTALL_INTERFACE:Inc>
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/Inc
	)
endfunction(tacent_target_include_directories)

function(tacent_target_compile_definitions PROJNAME)
	# Set -D defines based on configuration and platform.
	target_compile_definitions(
		${PROJNAME}
		PUBLIC
			ARCHITECTURE_X64
			$<$<CONFIG:Debug>:CONFIG_DEBUG>
			$<$<CONFIG:Release>:CONFIG_RELEASE>
			$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_DEPRECATE _LIB>
			$<$<PLATFORM_ID:Windows>:PLATFORM_WINDOWS>
			$<$<PLATFORM_ID:Linux>:PLATFORM_LINUX>
	)
endfunction(tacent_target_compile_definitions)

function(tacent_target_compile_options PROJNAME)
	# Set compiler option flags based on specific compiler and configuration.
	target_compile_options(
		${PROJNAME}
		PRIVATE
			# MSVC compiler.
			$<$<CXX_COMPILER_ID:MSVC>:/W2 /GS /Gy /Zc:wchar_t /Gm- /Zc:inline /fp:precise /WX- /Zc:forScope /Gd /FC>

			# Clang compiler.
			$<$<CXX_COMPILER_ID:Clang>:-Wno-switch>

			# GNU compiler.
			$<$<CXX_COMPILER_ID:GNU>:-Wunused-result>
			$<$<CXX_COMPILER_ID:GNU>:-Wno-multichar>
			$<$<CXX_COMPILER_ID:GNU>:-Wunused-result>

			$<$<AND:$<CONFIG:Debug>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-O0>
			$<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:/Od>
			$<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-O2>
			$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2>
	)
endfunction(tacent_target_compile_options)

function(tacent_target_compile_features PROJNAME)
	target_compile_features(
		${PROJNAME}
		PRIVATE
			cxx_std_17
	)
endfunction(tacent_target_compile_features)

function(tacent_set_target_properties PROJNAME)
	# This is how you set things like CMAKE_DEBUG_POSTFIX for a target.
	set_target_properties(
		${PROJNAME}
		PROPERTIES
		DEBUG_POSTFIX "d"												# Add a 'd' before the extension for debug builds.
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"	# Use multithreaded or multithreaded-debug runtime on windows.
		# More prop-value pairs here.
	)
endfunction(tacent_set_target_properties)

function(tacent_install PROJNAME)
	set(TACENT_INSTALL_DIR "${CMAKE_BINARY_DIR}/TacentInstall")
	message(STATUS "Tacent -- ${PROJECT_NAME} will be installed to ${TACENT_INSTALL_DIR}")

	install(
		TARGETS ${PROJNAME}
		EXPORT ${PROJECT_NAME}-targets
		LIBRARY DESTINATION ${TACENT_INSTALL_DIR}
		ARCHIVE DESTINATION ${TACENT_INSTALL_DIR}
	)

	install(DIRECTORY Inc/ DESTINATION "${TACENT_INSTALL_DIR}/Inc")

	install(
		EXPORT ${PROJNAME}-targets
		FILE
			${PROJNAME}Targets.cmake
		NAMESPACE
			Tacent::
		DESTINATION
			${TACENT_INSTALL_DIR}
	)
endfunction(tacent_install)
