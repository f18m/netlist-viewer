# Building NetlistViewer on Linux

Please install wxWidgets 3.0 and Boost >= 1.58 using your Linux package manager (apt, yum, dnf, etc).
E.g. on Ubuntu:

```
   $ sudo apt install -y libwxgtk3.0-gtk3-dev libboost-graph1.74.0 libboost-serialization1.74.0 libboost-all-dev
```

The `wx-config` global utility will be used to discover location of wxWidgets header/lib files.
Boost libs/headers are assumed to be in standard sytem-wide paths.

Then just run

```
    $ cd <your-path-to-NetlistViewer-sources>/build/linux 
	$ make
    $ sudo make install
    $ NetlistViewer
```

