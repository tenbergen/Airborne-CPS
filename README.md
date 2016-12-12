# Airborne-CPS
Building a Software Simulator for Airborne Cyber Physical Systems using X-Plane

### Description / Setup
This project is a plugin for the Microsoft X-Plane flight simulator, and is an implementation of a [Traffic Collision Avoidance System](https://www.faa.gov/documentLibrary/media/Advisory_Circular/TCAS%20II%20V7.1%20Intro%20booklet.pdf).

- The plugin was written in C++ in Microsoft Visual Studio 15 Community.

- The SDK for the plugin can be found [here](http://www.xsquawkbox.net/xpsdk/mediawiki/Main_Page).

- On Windows, you will need to add these to the linker/input/AdditionalDependencies settings:
  * glu32.lib
  * glaux.lib

- You will also need to get Google's serialization protocol buffers from [here](https://github.com/google/protobuf/releases/tag/v3.0.0).
  * For this project you'll need either protobuf-cpp-3.0.0.tar.gz or protobuf-cpp-3.0.0.zip
  * To build this library you'll also need [CMake](https://cmake.org/).
  * You'll have to run the CMake on the CMake file, which will result in a Visual Studio project you can build.
  

