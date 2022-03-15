# Airborne-CPS
A Traffic Collision Avoidance System Mark II (TCAS-II) implementation for Laminar Research X-Plane

### Current Functionality

- Threat detection within a protection volume.
- Threat classifications:
  * Traffic Advisory(TA)
  * Resolution Advisory(RA)
- RA action consensus and action suggestion.
- Drawing of recommended action(vertical velocity) to gauge.
- Suport for at least 3 client connections.
- Strength-Based RA Selection Algorithm for 3+ aircraft RA scenarios.

### Description
This project is a plugin for the Laminar Research X-Plane flight simulator, and is an implementation of a [Traffic Collision Avoidance System](https://www.faa.gov/documentLibrary/media/Advisory_Circular/TCAS%20II%20V7.1%20Intro%20booklet.pdf), mark II. It is used in Cyber Physical Systems research at the State University of New York at Oswego; please see "Literature" for examples and related work.

- The plugin was written in C++ using Microsoft Visual Studio Community 2022.
- The SDK for the plugin is included in the codebase, and can also be found [here](http://www.xsquawkbox.net/xpsdk/mediawiki/Main_Page).

### Setup

To configure Visual Studio for development and deployment, follow our [TCAS Deployment Guide](
https://docs.google.com/document/d/1tbVyqh8PEF99yZbX0Qzxu0Va8STFWqr0/edit?usp=sharing&ouid=111114585063672702784&rtpof=true&sd=true)
