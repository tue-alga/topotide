TTGA: Topological Tools for Geomorphological Analysis
=======================================================

[![DOI](https://zenodo.org/badge/204456296.svg)](https://zenodo.org/badge/latestdoi/204456296)

TTGA is a tool which helps the analysis of river systems, in particular, braided rivers and estuaries. The focus of the tool is the computation of river networks from a digital elevation model (DEM) of the river bed. Such a river network can then be used as input for further analyses, such as computing the length or average elevation of channels.

TTGA is available for Windows and Linux systems, and is free software licensed under the GNU General Public License version 3.

_Note: the current version of TTGA is an alpha version: it is not finished and is missing some features. These will be added in the future._

In the `src` directory, the actual C++ implementation can be found. The directory `src/gui` contains a GUI for visualizing the output, written in Qt, which can also be run in batch mode using a CLI. The directory `src/test` contains unit tests for essential parts of the implementation. More documentation on how to use TTGA can be found in the manual in the `manual` directory.

Compilation
-----

Compile by running

```shell
$ mkdir build
$ cd build
$ cmake ..
$ make
```

There are various CMake flags that can be used in the `cmake` invocation.

| Option     | Description    |
| ---------- | -------------- |
| `BUILD_TESTS` | Builds the unit tests (on by default). |
| `DISABLE_SLOW_ASSERTS` | Removes the slowest assertions, even when compiling in debug mode. For example the assertions that check if each component of the network stays connected (by doing a complete BFS after every operation of the algorithm) are removed. This makes the program much faster in debug mode. |
| `WITH_IPELIB` | Enables Ipe output. Only tested on Linux, depends on Ipelib being installed. |
| `WITH_KCRASH` | Enables KCrash (Dr. Konqi) integration: whenever the application crashes, Dr. Konqi will pop up with a nice stack trace. Very useful when debugging. Only tested on Linux, depends on KCrash being installed. |

For example:

```shell
cmake -DWITH_IPELIB=ON -DWITH_KCRASH=ON ..
```

Usage
-----
Execute by running

```shell
$ ./src/gui/ttga               # run GUI
$ ./src/gui/ttga --help        # run batch mode
$ ./src/test/ttga-test         # run unit tests
```
