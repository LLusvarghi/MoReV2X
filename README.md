# MoReV2X - A New Radio (NR) Vehicular Communication Module for ns-3
MoReV2X is a ns-3 module for the simulation of sub-6 GHz NR-V2X communications.  
The implementation of MoReV2X is focused on NR-V2X Mode 2, the distributed access strategy allowing the direct data exchange between vehicles.  

The MoReV2X module is backward compatible and implements LTE-V2X Mode 4.

Should you need any information about the code and how to run it please feel free to contact: (luca.lusvarghi5@unimore.it)

As of today, MoReV2X has been tested with . Its upgrade to the latest version of ns-3 is currently underway, stay tuned!

# Getting Started
After installing [ns-3.22](https://www.nsnam.org/releases/ns-3-22/), you just need to clone this repository inside the `src` ns-3 folder in order to use MoReV2X.
`git clone `

## Builiding with waf
Before running one of the scripts from the `examples` folder, configure the build with the following command:
`CXXFLAGS="-Wall -g -O0" ./waf configure --disable-python --enable-examples --disable-tests --build-profile=optimized`

and then build the optimized ns-3 programs typing
`./waf`

# About
Hats off to the people who contributed to this project: 
* Luca Lusvarghi (luca.lusvarghi5@unimore.it)  
* Lorenzo Gibellini 
* Maria Luisa Merani
* Alejandro Molina-Galan
* Baldomero Coll-Perales
* Javier Gozalvez

# License
The MoReV2X module is licensed under the GNU GPLv2 license.
