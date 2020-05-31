find_package("TacentProjectUtilities" REQUIRED)
project(Pipeline VERSION ${TACENT_VERSION} LANGUAGES CXX)

add_library(
	${PROJECT_NAME}
	Src/tProcess.cpp
	Src/tRule.cpp
	$<$<PLATFORM_ID:Windows>:Src/tSolution.cpp>
	Inc/Pipeline/tProcess.h
	Inc/Pipeline/tRule.h
	$<$<PLATFORM_ID:Windows>:Inc/Pipeline/tSolution.h>
)

tacent_target_include_directories(${PROJECT_NAME})
tacent_target_compile_definitions(${PROJECT_NAME})
tacent_target_compile_options(${PROJECT_NAME})
tacent_target_compile_features(${PROJECT_NAME})
tacent_set_target_properties(${PROJECT_NAME})

# Public means dependent interfaces are propagated. Private means they are not.
target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
		Foundation Math System
)

tacent_install(${PROJECT_NAME})
