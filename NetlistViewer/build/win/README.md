# Building NetListViewer on Windows

NetListViewer uses [vcpkg](https://vcpkg.io/en/getting-started) to install its 2 dependencies:
1. wxWidgets >= 3.0.0
1. Boost >= 1.58

Step by step procedure to get them installed:

1. Install vcpkg following the [vcpkg getting started](https://vcpkg.io/en/getting-started)
1. From a console window run:

```
   cd <your-vcpkg-repo-dir>
   vcpkg install wxwidgets boost
   vcpkg integrate install
```

Note that this step will take a while: it will rebuild all wxWidgets and Boost libraries and all their
dependencies as well.

1. Open the netlist_viewer_vs2022.sln project with VisualStudio 2022 or newer and you should be able to build it.



NOTE: as of Nov 2023, the installation throug vcpkg of the "expat" library (one of wxWidgets dependencies)
fails due to https://github.com/libexpat/libexpat/issues/418. Check that URL for the workaround (i.e. installing the English pack in VisualStudio)
