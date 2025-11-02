# Building NetlistViewer on MacOS arm 

Please install wxWidgets 3.0 and Boost >= 1.58 

```
   $ brew install boost
   $ brew install wxwidgets

```
Boost libs/headers are assumed to be in standard sytem-wide paths.
Then just run

```
    $ cd <your-path-to-NetlistViewer-sources>/build/macos 
	$ make
    $ sudo make install
    $ NetlistViewer
```