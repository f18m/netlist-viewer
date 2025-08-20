# Netlist Viewer

Netlist Viewer is a tool capable of loading SPICE netlists and convert them in a schematic (i.e. graphical) format.
The graphical representations helps to understand the electrical/electronic circuit represented by the SPICE netlist, and save some tedious work.

# What is a netlist?

A [circuit diagram](https://en.wikipedia.org/wiki/Circuit_diagram) can be represented in a very compact form using a [netlist](https://en.wikipedia.org/wiki/Netlist). Wikipedia's definition of the netlist is "a list of the electronic components in a circuit and a list of the nodes they are connected to".
An example of how a netlist actually looks like is:

```
.SUBCKT test_misc1 IN OUT

V1 0 IN DC=4V
R1 IN 2 
Q1 3 2 0 NPNstd
M1 OUT 3 0 NMOSstd
D1 OUT 0 DIODEstd

.ENDS
```

# Screenshots

![1](https://github.com/f18m/netlist-viewer/assets/9748595/ff8c1017-f92b-4f33-b399-36a6affe25de)
![2](https://github.com/f18m/netlist-viewer/assets/9748595/9a7054e3-cc1b-469b-82e9-95874dc7773a)

# Binaries

You can download binaries from [Github releases](https://github.com/f18m/netlist-viewer/releases).
These binaries are not garantueed to work on your system. 

## Windows

Note that if NetListViewer fails to start after installing with the Windows Installer and complains
about DLL dependencies named `VCRUNTIME<something>.dll`, it means that your Windows installation
is lacking the VC++ redistributable package.
You can install the latest VC Redistributable package following [https://vcredist.com/quick/](https://vcredist.com/quick/).

## Linux

A better way to distribute applications for Linux would be using [Flatpak](https://github.com/f18m/netlist-viewer/issues/6).
If you are interested in such work, please open an issue/PR.

# Past versions of Netlist-viewer

Past versions, namely version 0.1 and version 0.2, were hosted in Sourceforge, see https://sourceforge.net/projects/netlistviewer/.
All new developments have been moved into this Github project.

# How to build from sources

NetListViewer is a pretty simple application with 2 main dependencies: [wxWidgets](https://www.wxwidgets.org/) and [Boost](https://www.boost.org/).
Some simple notes on how to build NetListViewer for [Windows](NetlistViewer/build/win/README.md) and for [Linux](NetlistViewer/build/linux/README.md) are available.

# Status

The software is usable even if it could be improved very much.
Since the creation of the project (2010) I have not been using this tool anymore (I'm not doing any HW design anymore) so this software will not be further improved unless someone else wants to step up to improve it.
Pull requests are of course welcome anyhow.
