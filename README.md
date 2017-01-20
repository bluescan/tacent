# Tacent
Tacent is collection of C++ source files designed to be the basis for a game engine or other interactive project. Tacent is being released under the permissive MIT-style ISC licence. Originally the code was the backbone of the Tactile 3D project. 

Visual Studio 2015 Community Edition is being used to compile and run unit tests. Some (selective) improvements to take advantage of C++11 features are being made.

__Browse the Source__

The files that are currently available can be browsed using Woboq, a Clang-based tool that marks-up C++ to web-ready HTML. This tool is being used under the ShareAlike License and the 'What is this' text has been removed. Attribution links remain in tact. Woboq is being run on Windows 10 using the new Linux Subsystem.

[Browse the source here.](http://upperboundsinteractive.com/Tacent/Modules/index.html)

__Download the Source__

The Tacent source and solution/project files may be downloaded as a 7-zip archive. The instructions are simple: Unzip the archive somewhere, open Tools/UnitTests/UnitTests.sln, press F7 in Visual Studio to build, and press F5 to run.

[Download the source here.](http://upperboundsinteractive.com/Tacent.7z)

###Overview

Tacent is divided into a number of separate packages called modules. Each module is a collection of related source files. The code requires a C++11 compiler. Some modules depend on others. The current set of modules is:

* Foundation
The base set of classes, functions, and types. Includes container types (lists, arrays, priority queues, ring buffers), a fast memory pool, big integer types, bitfields, units, a string class. Depends on nothing but platform libs.

* Math
Vectors, matrices, quaternions, projections, linear algebra, hash functions, random number generation, splines, analytic functions, geometric primitives, mathematical constants, and colour space conversions. Depends on Foundation.

* System
File IO, path and file string parsing functions, chunk-based binary format, configuration file parsing, a light task system, a timer class, formatted printing, regular expression parser, a command-line parser with proper separation of concerns, and other utility functions. Depends on Foundation and Math.

* Image
Image loading, saving, manipulation, mipmapping, texture generation, and compression to various pixel formats including BC (like dxt1/3/5). Depends on Foundation, Math, and System.

* Build
Process launching with captured output and exit codes, dependency rule checking. Depends on Foundation, Math, and System.

* Scene
A scene library that can contain cameras, materials, models, splines, skeletons, lights, materials, attributes, and LOD groups. This is not really a full-on scene graph, but rather a tool to allow an exporter to be built and a geometry pipeline to process exported files. Depends on Foundation, Math, and System.


###Design

Tacent source code should be mostly self-documenting. Some C++ interfaces use obscure language features to control the valid ways in which the objects and functions can be used, but the cost is high — these interfaces cannot easily be inspected for intended use. In Tacent the goal has been to make the headers easily human-readable.

The implementations in Tacent are intended to be both clean (easily understood) and efficient. Efficient code does not need to be obscure code. Comments in plain English elucidate the complex parts. However, the reader is expected to know the language, so saying a variable is 'const' in a comment is clearly redundant if the const keyword is being used. There is no 'comment litter' in this regard.

This lack of redundancy and clutter just makes it all easier to absorb. You will notice in headers, for example, that a function argument declared as a tList<Item> will not have a dummy variable called 'items' afterwards, as the type already infers it's a list of items.

The dependencies are reasonable and well-understood. The code should be easy to port to a different environment for this reason. For example, if you need a printf function that supports custom types beyond floats, ints, and strings, as well as supporting different output channels, go ahead and take the tPrintf.h and tPrintf.cpp files in the System module and adapt them to your environment and needs. The dependencies on Tacent types and framework are not overbearing.


###Credits and Thanks

Credits are found directly in the code where appropriate. Here is a list of some of the helpful contributors:
* Gregory G. Slabaugh for a paper on Euler angles that was referenced to implement the ExtractRotationEulerXYZ function.
* Robert J. Jenkins Jr. for his hash function circa 1997. See burtleburtle.net
* M Phillips for his BigInt number class that helped structure the tFixInt implementation. The homepage link in tFixInt.h no longer works.
* Erik B. Dam, Martin Koch, and Martin Lillholm for their paper "Quaternions, Interpolation and Animation."
* Alberto Demichelis for his TRex regular expression class.
* Woboq for their code browser.
* Microsoft for their Community Edition version of Visual Studio.
* Simon Brown for his SquishLib texture block compression library.
* Ignacio Castano and nVidia for nVidia Texture Tools and placing the code under the MIT license.
* Davide Pizzolato for CxImage and for placing it under the zlib license.
