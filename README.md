# MoReV2X - A Cellular Vehicle-to-Vehicle (C-V2V) Communication Module for ns-3
MoReV2X is a simulator of sub-6 GHz NR-V2X communications based on ns-3.  
The implementation of MoReV2X is focused on NR-V2X Mode 2, the distributed access strategy allowing the direct data exchange between vehicles.
The MoReV2X module is backward compatible and implements LTE-V2X Mode 4.  

The first version of the code (v1.0) has been released and presented in 2021 during the IEEE Vehicular Technology Conference (VTC-Fall 2021). If you use MoReV2X in your research, please cite [this work](https://ieeexplore.ieee.org/document/9625478).  
With respect to v1.0, the MoReV2X simulator has been significantly extended over the last years and the current version of MoReV2X (v4.0) includes the following new features:
* Support of variable Packet Delay Budget (PDB) requirements
* Re-evaluation mechanism
* Novel metrics to assess the operation of the re-evaluation mechanism
* Blind re-transmissions 
* Encoding and decoding of the 2nd-stage SCI
* Platooning with unicast and groupcast communications

Should you need any information about the code and how to run it please feel free to contact: luca.lusvarghi5@unimore.it  

The upgrade of MoReV2X to the latest version of ns-3 is currently underway, stay tuned!

# Getting Started
First, you need to download the MoReV2X simulator on your computer. To do so, run the following command:   
`git clone https://github.com/LLusvarghi/MoReV2X.git `

## Building with waf
Before running one of the scripts from the `scratch` folder, configure the build with the following command:   
`CXXFLAGS="-Wall -g -O0" ./waf configure --disable-python --enable-examples --disable-tests --build-profile=debug`

and then build ns-3 typing   
`./waf`

# About
List of people who contributed to this project: 
* Luca Lusvarghi (luca.lusvarghi5@unimore.it), Università degli Studi di Modena e Reggio Emilia, Italy
* Lorenzo Gibellini, Università degli Studi di Modena e Reggio Emilia, Italy
* Maria Luisa Merani, Università degli Studi di Modena e Reggio Emilia, Italy
* Alejandro Molina-Galan, Universidad Miguel Hernandez de Elche (UMH), Spain
* Baldomero Coll-Perales, Universidad Miguel Hernandez de Elche (UMH), Spain
* Javier Gozalvez, Universidad Miguel Hernandez de Elche (UMH), Spain

# License
The MoReV2X module is licensed under the GNU GPLv2 license.
