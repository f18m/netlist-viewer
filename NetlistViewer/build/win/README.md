# Building NetListViewer on Windows

NetListViewer uses [vcpkg](https://vcpkg.io/en/getting-started) to install its 2 dependencies:
1. wxWidgets >= 3.0.0
1. Boost >= 1.58

Step by step procedure to get them installed:

1. Install vcpkg following the [vcpkg getting started guide](https://vcpkg.io/en/getting-started)
1. From a "cmd.exe" window run:

```
   cd <netlist-viewer-git-repo>
   vcpkg install                 # this will read the dependencies from vcpkg.json and download and build them
   vcpkg integrate install       # this allows the dependencies to be usable from msbuild
```

Note that this step will take a while: it will rebuild all wxWidgets and Boost libraries and all their
dependencies as well.

1. Open the netlist_viewer_vs2022.sln project with VisualStudio 2022 or newer and you should be able to build it. 
Alternatively the build can be launched from a console window as well:

```
   cd <netlist-viewer-git-repo>
   msbuild NetlistViewer\build\win\netlist_viewer_vs2022.vcxproj -t:rebuild -property:Configuration=Release -property:Platform=x64
```

NOTE: as of Nov 2023, the installation through vcpkg of the "expat" library (one of wxWidgets dependencies) can
fail due to https://github.com/libexpat/libexpat/issues/418 if you have a localized version of VisualStudio.
Check that URL for the workaround (i.e. installing the English pack in VisualStudio)
