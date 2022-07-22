
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


function(tacent_is_arch_arm retval)
	message(STATUS "Tacent -- ArchitectureHost   ${CMAKE_HOST_SYSTEM_PROCESSOR}")
	message(STATUS "Tacent -- ArchitectureTarget ${CMAKE_SYSTEM_PROCESSOR}")
	SET(${retval} False PARENT_SCOPE)

	string(FIND "${CMAKE_SYSTEM_PROCESSOR}" "arm" SUBSTRINDEXVAR)
	if (NOT SUBSTRINDEXVAR EQUAL -1)
		SET(${retval} True PARENT_SCOPE)
	endif()

	string(FIND "${CMAKE_SYSTEM_PROCESSOR}" "aarch64" SUBSTRINDEXVAR)
	if (NOT SUBSTRINDEXVAR EQUAL -1)
		SET(${retval} True PARENT_SCOPE)
	endif()
endfunction()


function(tacent_target_compile_definitions PROJNAME)
	# Set -D defines based on configuration and platform.
	target_compile_definitions(
		${PROJNAME}
		PUBLIC
			# ARCHITECTURE_X64
			$<$<CONFIG:Debug>:CONFIG_DEBUG>
			$<$<CONFIG:Release>:CONFIG_RELEASE>
			$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_DEPRECATE _LIB>
			# $<$<PLATFORM_ID:Windows>:_ITERATOR_DEBUG_LEVEL=0>
			$<$<PLATFORM_ID:Windows>:PLATFORM_WINDOWS>
			$<$<AND:$<PLATFORM_ID:Windows>,$<BOOL:${TACENT_UTF16_API_CALLS}>>:UNICODE>	# C++	UFF-16
			$<$<AND:$<PLATFORM_ID:Windows>,$<BOOL:${TACENT_UTF16_API_CALLS}>>:_UNICODE>	# C 	UTF-16
			$<$<AND:$<PLATFORM_ID:Windows>,$<BOOL:${TACENT_UTF16_API_CALLS}>>:TACENT_UTF16_API_CALLS>

			# Uncomment to force UTF-16 API calls.
			#$<$<PLATFORM_ID:Windows>:UNICODE>	# C++	UFF-16
			#$<$<PLATFORM_ID:Windows>:_UNICODE>	# C 	UTF-16
			#$<$<PLATFORM_ID:Windows>:TACENT_UTF16_API_CALLS>

			$<$<PLATFORM_ID:Linux>:PLATFORM_LINUX>
	)
endfunction(tacent_target_compile_definitions)


function(tacent_target_compile_options PROJNAME)
	# Set compiler option flags based on specific compiler and configuration.
	target_compile_options(
		${PROJNAME}
		PRIVATE
			# MSVC compiler. Warning /utf-8 is needed for MSVC to support UTF-8 source files. Basically u8 string literals won't work without it.
			$<$<CXX_COMPILER_ID:MSVC>:/utf-8 /W2 /GS /Gy /Zc:wchar_t /Gm- /Zc:inline /fp:precise /WX- /Zc:forScope /Gd /FC>
			$<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:/Od>
			$<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>:/O2>

			# Clang compiler.

			# GNU compiler. -std=c++20
			$<$<CXX_COMPILER_ID:GNU>:-Wno-unused-result>
			$<$<CXX_COMPILER_ID:GNU>:-Wno-stringop-overflow>

			# Clang and GNU.
			$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wno-multichar>
			$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wno-switch>

			$<$<AND:$<CONFIG:Debug>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-O0>
			$<$<AND:$<CONFIG:Release>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:-O2>
	)
endfunction(tacent_target_compile_options)

function(tacent_target_compile_features PROJNAME)
	target_compile_features(
		${PROJNAME}
		PRIVATE
			cxx_std_20
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

function(tacent_target_link_options PROJNAME)
	target_link_options(
		${PROJNAME}
		INTERFACE
			# We only supply release 3rd party precompiled libs.
			$<$<PLATFORM_ID:Windows>:/ignore:4099>
	)
endfunction(tacent_target_link_options)

function(tacent_install PROJNAME)
	# This path is relative to CMAKE_INSTALL_PREFIX. Do not use an absolute path if you want
	# the exports to work properly (the will have bad absolute paths if you do).
	set(TACENT_INSTALL_RELDIR "TacentInstall")
	message(STATUS "Tacent -- ${PROJNAME} will be installed to ${TACENT_INSTALL_RELDIR}")

	install(
		TARGETS ${PROJNAME}
		EXPORT ${PROJNAME}-targets
		LIBRARY DESTINATION ${TACENT_INSTALL_RELDIR}
		ARCHIVE DESTINATION ${TACENT_INSTALL_RELDIR}
	)

	install(DIRECTORY Inc/ DESTINATION "${TACENT_INSTALL_RELDIR}/Inc")

	install(
		EXPORT ${PROJNAME}-targets
		FILE
			${PROJNAME}Targets.cmake
		NAMESPACE
			Tacent::
		DESTINATION
			${TACENT_INSTALL_RELDIR}
	)
endfunction(tacent_install)
