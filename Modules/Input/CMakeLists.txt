find_package("TacentProjectUtilities" REQUIRED)
project(Input VERSION ${TACENT_VERSION} LANGUAGES CXX)

add_library(
	${PROJECT_NAME}
	Src/tUnit.cpp
	Src/tUnitContinuousAxis.cpp
	Src/tUnitContinuousDisp.cpp
	Src/tUnitContinuousWind.cpp
	Src/tUnitDiscreteBool.cpp
	Src/tUnitDiscreteMulti.cpp
	Src/tComp.cpp
	Src/tCompButton.cpp
	Src/tCompSelector.cpp
	Src/tCompDial.cpp
	Src/tCompWheel.cpp
	Src/tCompScrubber.cpp
	Src/tCompDirPad.cpp
	Src/tCompJoystick.cpp
	Src/tCompPedal.cpp
	Src/tCompSlider.cpp
	Src/tCompSwitch.cpp
	Src/tCompTrigger.cpp
	Src/tCont.cpp
	Src/tContGamepad.cpp
	Src/tContRacingWheel.cpp
	Src/tControllerSystem.cpp
	Inc/Input/tUnit.h
	Inc/Input/tUnitContinuousAxis.h
	Inc/Input/tUnitContinuousDisp.h
	Inc/Input/tUnitContinuousWind.h
	Inc/Input/tUnitDiscreteBool.h
	Inc/Input/tUnitDiscreteMulti.h
	Inc/Input/tComp.h
	Inc/Input/tCompButton.h
	Inc/Input/tCompSelector.h
	Inc/Input/tCompDial.h
	Inc/Input/tCompWheel.h
	Inc/Input/tCompScrubber.h
	Inc/Input/tCompDirPad.h
	Inc/Input/tCompJoystick.h
	Inc/Input/tCompPedal.h
	Inc/Input/tCompSlider.h
	Inc/Input/tCompSwitch.h
	Inc/Input/tCompTrigger.h
	Inc/Input/tCont.h
	Inc/Input/tContGamepad.h
	Inc/Input/tContRacingWheel.h
	Inc/Input/tControllerSystem.h
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
		Foundation Math System
		$<$<PLATFORM_ID:Windows>:XInput.lib>
)

tacent_install(${PROJECT_NAME})
