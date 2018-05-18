# Airborne-CPS
Building a Software Simulator for Airborne Cyber Physical Systems using X-Plane

### Description / Setup
This project is a plugin for the Microsoft X-Plane flight simulator, and is an implementation of a [Traffic Collision Avoidance System](https://www.faa.gov/documentLibrary/media/Advisory_Circular/TCAS%20II%20V7.1%20Intro%20booklet.pdf).

- The plugin was written in C++ in Microsoft Visual Studio 15 Community.

- The SDK for the plugin is included in the codebase, and can also be found [here](http://www.xsquawkbox.net/xpsdk/mediawiki/Main_Page).

- On Windows, you will need these linker/input/AdditionalDependencies settings:
  * glu32.lib
  * glaux.lib
  They should already be included.

- You need Google's serialization protocol buffers from [here](https://github.com/google/protobuf/releases/tag/v3.0.0).
  * These are included in the codebase.
  
- Open the Visual Studio project ExampleGauge, and change the project's C++ Linker folder locations, Dependency folder locations, and build   path to reflect your project's path structure.

- Copy the included Images folder from the repository to the Plugins folder of X-Plane.

- Build plugin.
  
  
### Current Functionality

- Threat detection within a protection volume.
- Threat classifications:
  * Traffic Advisory(TA)
  * Resolution Advisory(RA)
- RA action consensus and action suggestion.
- Drawing of recommended action(vertical velocity) to gauge.
- Suport for at least 3 client connections.
- Strength-Based RA Selection Algorithm for 3+ aircraft RA scenarios.


### Builds

Extract to Plugins folder of X-Plane 10

-x32 https://github.com/tenbergen/Airborne-CPS/raw/master/AirborneCPS-x32.zip
