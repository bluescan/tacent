# Tacent Input Module

The next module for Tacent is going to be for controller and gamepad input.

The Input Module handles retrieving input from various devices including gamepads that may be attached. A few definitions are helpful to describe the types and nomenclature used in this module. We'll start from the top-down.

## Controller
A controller is a hardware device attached to your computer that supports providing the computer with one or more forms of input. Usually the word controller is used in the context of gaming. An example controller is a 'gamepad'. We use gamepad to mean the canonical gaming input contoller with two joysticks, a d-pad, 4 face buttons, 2 triggers and 2 bumbers (essentially a 360 controller). There is a one-to-one mapping between the specific tController sub-class and a physical device. The tController base class provides common functionality for a sub-classed tController type. Currently the only supported controller is a gamepad (360 controller, 8BitDo, etc). A possible additinal controller would be a RacingWheel. All instances of a tController have a unique identifier in case multiple are plugged in simultaneously. Controllers contain one or more "components" described below.

## Component
A component is one of possibly many pieces of "independent" input hardware in a controller. For example, a gamepad controller has two joysticks, multiple buttons, triggers, and a d-pad. A component is basically a base class that contains a unique identifier/name for every part of a controller. Some components have multiple dependent parts. For example, a joystick has two axis units, but these are driven by a single physical 'stick' which means that, for example, dead-zone calculations need to take into account _both_ axes to be done correctly. Units are described below.

## Unit
The atom of the input world. Indivisible. An input unit is one of (usually many) pieces of hardware in a controller that reads a _single_ user input. A physical button, for example, is an input unit. So is one of the axes of a joystick. Note that a joystick is not a unit -- it is a component made with two axis units, one for each orthogonal direction. There is a one-to-one mapping between the tUnit class and a physical electro-mechanical device that reads a _single_ value. Units do not do any input processing so no dead-zone or anti-jitter functionality exists in units themselves -- they simply get updated/clamped and have accessors. The supported input units are described below.

## Summary
In short, a controller is made up of components, and components are made up of units.
* Controllers are uniquely identified and have names.
* Components are uniquely defined within a controller and have names.
* Units are uniquely defined within a component.
All three exist in the tInput namespace and have base classes: tController, tComponent, and tUnit.

Sub classes start with the following prefixes to identify them:
* tCont : Tacent Input Controller.
* tComp : Tacent Input Component.
* tUnit : Tacent Input Unit.

The input system (tInputSystem) pulls all this together and is the workhorse of the system. It is described below.

## Supported Units

* tUnitBoolState
A binary unit that is either on or off. Think buttons and switches. No intermediate states. The tUnitBoolState has a current state boolean member, a polling state boolean member, as well as an event queue of state changes to be processed which when applied will bring the current state 'up to date' with the polling (raw) state.

* tUnitMultiState  
An input unit with multiple input states. Things like a button that responds to different pressure levels. The tUnitMultiState has a current state enum member, a polling state enum member, as well as an event queue of state changes to be processed which when applied will bring the current state 'up to date' with the polling (raw) state.

* tUnitAxis  
An input unit that contains a value in [-1.0, 1.0]. Components like a joystick are made up of two of these. 

* tUnitDisp
An input unit that reports a value in [0.0, 1.0]. Displacement units may or may not return to any default/resting position if untouched. Components that use these may need anti-jitter and/or dead-zones to be implemented, but this depends on that particular component rather than tUnitDisp. Components that use a tUnitDisp include sliders, triggers (return), and dials.

## Supported Components

* tCompButton  
Contains a single tUnitBoolState. Buttons always return to their default off state when not physically held down.

* tCompSwitch  
Contains a single tUnitBoolState. Switches remain in the state they're set to last. On or off.

* tCompDirPad  
D-Pads are made of dependent tUnitBoolState units, one for each or the 4 cardinal directions. They are dependent because they are mechanically restricted from all being pressed at the same time. Typically only one or two of the 4 direction buttons may be simultaneously engaged (at 45 degrees some support 2). They all return to the off state if no physical input.

* tCompTrigger  
Made up of a single tUnitDisp. Dead-zone implemented for values close to zero, plus optional anti-jitter. Returns to 0 if no physical input applied.

* tCompPedal  
Made up of a single tUnitDisp. Dead-zone implemented for values close to zero, plus optional anti-jitter. Returns to 0 if no physical input applied.

* tCompJoystick  
A joystick contains one tUnitAxis for the horizontal (X) direction, and another for the vertical (Y) direction. The axes of a joystick are NOT independent since the same physical stick displaces both X and Y magnets for the hall effect transistors to pick up. This class implements dead-zone and anti-jitter correctly as it can read both axes. It's 'resting' position is (0,0) and is unstable due to the small physical forces (often springs) keeping it there. No physical input and a joystick returns to (0,0) for both X and Y axes.

* tCompDial  
Made up of a single tUnitDisp. Dials do not return to 0. Implement anti-jitter but no dead-zone. Used for things like volume control knobs.

* tCompSlider  
Made up of a single tUnitDisp. Sliders do not return to 0. Implement anti-jitter but no dead-zone.

## Input System
The input system (tInputSystem) is responsible for managing controllers. It allows the client to register callbacks when important things happen like the addition of a new controller or the removal of an existing one. Querying the attached controllers is done on a separate 'polling' thread.

### Polling Thread
The polling thread runs at a high frequency -- user configureable but hundreds or even 1000Hz is not unreasonable. In this polling thread the controller hardware is read using system APIs. In particular xinput on Windows, and the fcntl/joystick APIs on Linux. The polling thread then updates the units for the controller in question. For units with 'state' in their name, the polling (raw) state is updated and an event is added if the polling state has changed from the previous poll. This allows things like button presses and releases to 'not be missed' if they happen so quickly that they are within a single main thread update. Updates to the input units, including event queues, are protected by mutexes.

### Main Thread
Main thread updates are typically much slower -- in the order of 60Hz. The client of tInputSystem is responsible for registering callbacks that it cares about (like controller connection/disconnection) on construction (which starts up the polling thread), calling an 'Update' function periodically (usually at the current framerate), and reading any values from the controllers it cares about. The destructor will automatically perform any required shutdown including stopping the polling thread.  

Any connection/disconnection callbacks are called in the context of the main thead when the Update function is called. The update function performs the following operations:  
* Call and client callbacks that are registered for controller connection/disconnection.
* Process a single event on any units that have the word 'state' in them -- these units have event queues. It is important that only one is done at a time so the main thread update loop can respond to any changes. Over multiple frames it will 'catch up' to the polling state. This ensures the main thread has a chance to react to any 'twitchy' state changes that were quicker than 33 or 16 milliseconds in duration.
* Process any dead-zones for all tUnitAxis and tUnitDisp units.
* Process anti-jitter for all tUnitAxis and tUnitDisp units.
After the update call, the client is free to read any component of any controller it feels like directly. The read calls of all the components also use mutexes to ensure consistent data and no collisions with updates from the polling thread.

Note that we _could_ do things like dead-zone/anti-jitter calculations in the polling thread at the higher frequency but since there are no 'state changes' to manage, the result would be the same. It therefore makes sense to do these in the main thread update call. The polling thread should be as fast and efficient as possible. The main thread update also cares about 'components' and the dependencies between the different units. In the joystick component, for example, the update call knows that the two tUnitAxis units are dependent on each other and can make the dead-zone a proper circle rather than a square. A square is what you'd get if the axes were treated independently. The polling thread also cares about 'components'. It is important that all the units of a component get updated 'atomically'. This keeps things like the two axes perfectly in sync with each other when the main thread update does it's work.
