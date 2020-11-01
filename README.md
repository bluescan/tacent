![Unit Tests](https://github.com/bluescan/tacent/workflows/Unit%20Tests/badge.svg)

# Tacent
Tacent is a C++ library designed to be the foundation of a game engine or other interactive project. Originally the code was the backbone of the Tactile 3D project.

Modern CMake (target-based) is used to generate the build files. Generators for makefiles, ninja, NMake, and Visual Studio solutions have been tested. Tacent may be compiled with gcc, clang, or msvc.

Thanks to Woboq, the source may be viewed in a web browser [here](http://upperboundsinteractive.com/Tacent/Modules/index.html).

### Overview

Tacent is divided into a number of separate modules. Each module is a collection of related source files. Some modules depend on others.

* __Foundation__
The base set of classes, functions, and types. Includes container types (lists, arrays, priority queues, ring buffers), a fast memory pool, big integer types, bitfields, units, a string class. Depends on nothing but platform libs.

* __Math__
Vectors, matrices, quaternions, projections, linear algebra, hash functions, random number generation, splines, analytic functions, geometric primitives, mathematical constants, and colour space conversions. Depends on Foundation.

* __System__
File IO, path and file string parsing functions, chunk-based binary format, configuration file parsing, a light task system, a timer class, formatted printing, regular expression parser, a command-line parser with proper separation of concerns, and other utility functions. Depends on Foundation and Math.

* __Image__
Image loading, saving, manipulation, mipmapping, texture generation. Depends on Foundation, Math, and System.

* __Pipeline__
Process launching with captured output and exit codes, dependency rule checking. Depends on Foundation, Math, and System.

* __Scene__
A scene library that can contain cameras, materials, models, splines, skeletons, lights, materials, attributes, and LOD groups. This is not really a full-on scene graph, but rather a tool to allow an exporter to be built and a geometry pipeline to process exported files. Depends on Foundation, Math, and System.


### Design

Tacent source code is largely self-documenting. The goal has been to make the headers easily human-readable.

Implementations in Tacent are intended to be both readable and efficient. There are ample comments but no 'comment-litter'. eg. An 'in-only' comment is redundant if const is being used. eg. A function argument declared as tList<Item> does not need a dummy variable called 'items' afterwards.

The dependencies are reasonable and the code should be easy to port to a different environments. eg. if you need a formatted print function that supports custom types (perhaps vectors), or output channels, it should be fairly straightforward to take tPrintf.h and tPrintf.cpp and adapt them to your needs.


### Examples

The unit tests are a good reference. There may be more information in the comments [on the homepage](http://upperboundsinteractive.com/tacent.php).

__Example: Image Loading__
This is how you'd load a png file and save it to a targa. If the png has transparency, so will the tga:

```C++
tPicture pngPic("ThoseEyes.png");
pngPic.Save("ThoseEyes.tga");
```

And loading a cubemap direct draw surface and saving it to a binary chunk format is done like this:

```C++
tTexture cubemap("Cubemap.dds");
tChunkWriter writer("Cubemap.tac");
cubemap.Save(writer);
```

__Example: Command Line Parsing__

With tCommand you specify which options and parameters you care about only in the cpp file you are working in. Tacent can parse the parameters, options, and flags.

```
mycopy.exe -R --overwrite fileA.txt -pat fileB.txt --log log.txt
```

The fileA.txt and fileB.txt in the above example are parameters. The order in which parameters are specified is important. fileA.txt is the first parameter, and fileB.txt is the second. Options on the other hand can be specified in any order. All options take a specific number of arguments. If an option takes zero arguments it is a flag (present or not).

The '--log log.txt' is an option with a single argument, log.txt. Single character flags specified with a single hyphen may be combined. The -pat in the example expands to -p -a -t. You may list the same option more than once. Eg. -i filea.txt -i fileb.txt etc is fine. To use the command line class, you start by registering your options and parameters. This is done using the tOption and tParam types to create static objects. After main calls the parse function, your objects get populated appropriately.

```C++
// FileA.cpp:
tParam FromFile(1, "FromFile");             // The 1 means this is the first parameter. The description is optional.
tParam ToFile(2, "ToFile");                 // The 2 means this is the second parameter. The description is optional.
tOption("log", 'l', 1, "Specify log file"); // The 1 means there is one option argument to --log or -l.

// FileB.cpp:
tOption ProgramOption('p', 0, "Program mode.");
tOption AllOption('a', "ALL", 0, "Include all widgets.");
tOption TimeOption("time", 't', 0, "Print timestamp.");

// Main.cpp:
tParse(argc, argv);
```

### Building

The suggested build methods are to eitehr use VSCode with the CMake Tools extension or from the command line. Both methods work in either Windows or Linux. There are two build-types (AKA configurations) with Tacent: Debug and Release. If you want to build debug versions pass -DCMAKE_BUILD_TYPE=Debug to the cmake command.

The 'install' target creates a directory called 'Install' that has all the built libraries (.a or .lib), exported headers, cmake target files, and the unit test executable.

#### Windows
* Install Visual Studio Community 2019 (this gives you CMake, Ninja, and the MSVC compiler)
* Install VS Code (optional)
* Open 64bit Command Prompt for VS2019 and cd into the the 'tacent' directory. Do an out-of-source build.
```
mkdir buildninja
cd buildninja
cmake .. -GNinja
ninja install
```    

#### Ubuntu
* Install ninja and either Clang or GCC
* Install VS Code from the snap store (optional)
```
sudo apt-get install ninja-build
```
* Open a terminal window and cd into the tacent directory.
```
mkdir buildninja
cd buildninja
cmake .. -GNinja
ninja install
```

#### Visual Studio Code
Using the VS Code editor along with the CMake Tools extension works surprisingly well. The editor is cross platform so you get the same experience on Ubuntu as well as Windows.

On either platform open up VSCode. Choose File->Open Folder and open the 'tacent' directory. It will automatically detect the CMakeFiles.txt files, suggest installing CMake Tools, ask permission to generate an intellisence database for code completion, etc.

On Windows choose the 'Visual Studio 2019 Release -amd64' compiler kit (on the bottom info bar). The build-type to the left can be set to either Debug or Release (tacent ships with a cmake-variants.yaml file since that's one thing CMake Tools doesn't read from CMake). To the right select 'install' for the build type. Hit F7.

The instructions for Ubuntu are nearly identical. The kit can be Clang 9 or 10 or GCC 9.3.


### Credits and Thanks

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
* Omar Cornut for Dear ImGui.


### Legal

Any 3rd party credits and licences may be found directly in the code at the top of the file and/or the licence may be found in the Docs directory. The image-loading module contains the most 3rd-party code. tImageHDR.cpp, that loads high-dynamic-range (hdr) images, includes Radiance software (http://radsite.lbl.gov/) developed by the Lawrence Berkeley National Laboratory (http://www.lbl.gov/). If the hdr code is incuded in your project, attribution is required either in your end-product or its documentation and the word "Radiance" is not to appear in the product name. tImageJPG.cpp, that loads and saves JPeg images, is based in part on the work of the Independent JPEG Group. Loading jpg images uses libjpeg-turbo - Neither the name of the libjpeg-turbo Project nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. Woboq is used under the ShareAlike License with attribution links remaining in tact.
