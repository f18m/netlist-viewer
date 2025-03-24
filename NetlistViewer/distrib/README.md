# Steps for packaging NetlistViewer on Windows

1. Download and install [NSIS](https://nsis.sourceforge.io/Download).
1. Compile NetlistViewer against wxWidgets RELEASE and copy the final .exe file in the "build\win" folder.
1. Compile the distrib\setup.nsi with NSIS
   Note that you can also use the build_installers.bat batch file to do this step automatically.
1. Attach the installer to the new Github release

NOTE: these steps run also in the CI pipeline so you can find the NSIS Windows installer attached to any run of the CI.