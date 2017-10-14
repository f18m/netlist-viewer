Steps for releasing NetlistViewer on Windows
--------------------------------------------

1) Download and install NSIS.
2) Copy GetVersion.dll in your NSIS\Plugins folder
3) Compile NetlistViewer as a 32bit application against wxWidgets RELEASE and copy the final .exe file in the "x86" folder.
5) Set ARCH=x86 in setup.nsi and compile it with NSIS
   Note that you can also use the build_installers.bat batch file to do this step automatically.
7) Upload the installer on the release file server.
