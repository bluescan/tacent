---
title: Homepage
---

![Build](https://github.com/bluescan/tacent/workflows/Build/badge.svg) [![latest](https://img.shields.io/github/v/release/bluescan/tacent.svg)](https://github.com/bluescan/tacent/releases)

## Introduction

_Tacent_ is a C++ library designed to be the foundation for a game pipeline or other interactive project. It implements support for linear algebra, text and file IO, UTF-N conversions, containers, image loading/saving, image quantization/filtering, command-line parsing, etc. Modern CMake (target-based) is used to generate the build files. Generators for makefiles, ninja, NMake, and Visual Studio solutions have been tested. Tacent may be compiled with gcc, clang, or msvc.

---
## Source Browser

_Tacent_ source code can be browsed online.

---
## Overview

Tacent is divided into a number of separate modules. Each module is a collection of related source files. Some modules depend on others.

* __Foundation__
The base set of classes, functions, and types. Includes container types (lists, arrays, priority queue, map/dictionary, ring buffer), a fast memory pool, big integer types, bitfields, units, a string class, UTF conversion functions. Depends on nothing but platform libs.

* __Math__
Vectors, matrices, quaternions, projections, linear algebra, hash functions, random number generation, splines, analytic functions, geometric primitives, mathematical constants, and colour space conversions. Depends on Foundation.

* __System__
File IO, path and file string parsing functions, chunk-based binary format, configuration file parsing, a light task system, a timer class, formatted printing, regular expression parser, a command-line parser with proper separation of concerns, and other utility functions. Depends on Foundation and Math.

* __Image__
Image loading, saving, manipulation, mipmapping, texture generation, and colour quantization. Depends on Foundation, Math, and System.

* __Pipeline__
Process launching with captured output and exit codes, dependency rule checking. Depends on Foundation, Math, and System.

* __Scene__
A scene library that can contain cameras, materials, models, splines, skeletons, lights, materials, attributes, and LOD groups. This is not really a full-on scene graph, but rather a tool to allow an exporter to be built and a geometry pipeline to process exported files. Depends on Foundation, Math, and System.
