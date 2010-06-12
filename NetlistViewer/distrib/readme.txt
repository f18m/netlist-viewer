Steps for releasing NetlistViewer on Windows
--------------------------------------------

1) Download and install NSIS.
2) Copy GetVersion.dll in your NSIS\Plugins folder
3) Compile UsbPicProg as a 32bit application and copy the final .exe file in the "x86" folder.
4) Compile UsbPicProg as a 64bit application and copy the final .exe file in the "amd64" folder.
5) Set ARCH=x86 in setup.nsi and compile it with NSIS; set ARCH=amd64 in setup.nsi and compile it with NSIS.
   Note that you can also use the build_installers.bat batch file to do this step automatically.
7) Upload the two installers on the release file server.
   They should install and work fine on both 32bit and 64bit versions of Windows XP, Windows Vista,
   Windows Server 2003, Windows 7 (and higher).

IMPORTANT: when compiling the 32bit versions of UsbPicProg with MS Visual Studio 2008 Express/Professional
           you may incur in the following bug (which makes the final .exe unable to run):
             https://connect.microsoft.com/VisualStudio/feedback/details/361682/vc9-sp1-generates-manifests-with-the-wrong-version-number?wa=wsignin1.0
           The solution is to tell MSVC to NOT embed the manifest XML file in the final EXE and then manually
           edit and fix the version in the .manifest file before running NSIS.
