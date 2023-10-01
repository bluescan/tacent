find_package("TacentProjectUtilities" REQUIRED)
project(System VERSION ${TACENT_VERSION} LANGUAGES CXX)

add_library(
	${PROJECT_NAME}
	Src/tChunk.cpp
	Src/tCmdLine.cpp
	Src/tFile.cpp
	Src/tMachine.cpp
	Src/tPrint.cpp
	Src/tRegex.cpp
	Src/tScript.cpp
	Src/tTask.cpp
	Src/tThrow.cpp
	Src/tTime.cpp
	Inc/System/tChunk.h
	Inc/System/tCmdLine.h
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

tacent_target_include_directories(${PROJECT_NAME})
tacent_target_compile_definitions(${PROJECT_NAME})
tacent_target_compile_options(${PROJECT_NAME})
tacent_target_compile_features(${PROJECT_NAME})
tacent_set_target_properties(${PROJECT_NAME})

# Note that since Math happens to also depend on Foundation, we could have left Foundation out and it would work.
target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
		Foundation Math
)

tacent_install(${PROJECT_NAME})
