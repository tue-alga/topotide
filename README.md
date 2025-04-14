TopoTide: Topological Tools for Network Extraction
=======================================================

![The TopoTide logo](https://github.com/user-attachments/assets/cc91b284-6fbd-4738-bae2-4e5e02057930)

[![DOI](https://zenodo.org/badge/204456296.svg)](https://zenodo.org/badge/latestdoi/204456296)

TopoTide is a tool which helps the analysis of river systems, in particular, braided rivers and estuaries. The focus of the tool is the computation of river networks from a digital elevation model (DEM) of the river bed. Such a river network can then be used as input for further analyses, such as computing the length or average elevation of channels.

TopoTide is available for Windows and Linux systems, and is free software licensed under the GNU General Public License version 3.

In the `lib` directory, the C++ implementation of the algorithms can be found. The directory `gui` contains a GUI for visualizing the output, written in Qt, which can also be run in batch mode using a CLI. The directory `test` contains unit tests for essential parts of the implementation. More documentation on how to use TopoTide can be found in the manual in the `manual` directory.


## Dependencies

TopoTide depends on the following build tools:

* g++ (11.4.0) / clang++ (14.0.0)
* CMake (3.12)

And it depends on the following libraries:

* Qt (6.4) – for the interactive GUI
* GDAL (3.8.4) – for reading raster files

The version numbers listed are the ones we're testing with. Newer (and possibly somewhat older) versions will most likely work as well.


### Windows (MSYS2 / MINGW64)

<details>
  <summary><b>Installing dependencies on Windows (MSYS2 / MINGW64)</b></summary>

In case your machine does not have MSYS2 installed yet, you can download it from [here](https://www.msys2.org/). Then install the dependencies from the repository:

```sh
pacman -S base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
pacman -S mingw-w64-x86_64-qt6
```
</details>


### Linux

<details>
  <summary><b>Installing dependencies on Linux</b></summary>

On Ubuntu, install the dependencies from the repository:

```sh
sudo apt install build-essential cmake
sudo apt install qt6-base-dev
sudo apt install libgdal-dev
```
</details>


## Compiling

TopoTide uses CMake as its build system and can therefore be built like any other CMake application, for example:

```sh
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build build
cmake --install build
```

There are various CMake flags that can be used in the `cmake` invocation.

| Option     | Description    |
| ---------- | -------------- |
| `BUILD_TESTS` | Builds the unit tests (on by default). |
| `DISABLE_SLOW_ASSERTS` | Removes the slowest assertions, even when compiling in debug mode. For example the assertions that check if each component of the network stays connected (by doing a complete BFS after every operation of the algorithm) are removed. This makes the program much faster in debug mode. |
| `EXPERIMENTAL_FINGERS_SUPPORT` | Enables support for finger detection (off by default). This is very experimental. Running finger detection may be buggy and consumes a lot of memory even for fairly small datasets. This will be improved in the future. |

For example:

```shell
cmake -DBUILD_TESTS=OFF -S . -B build
```

Usage
-----
Execute by running

```shell
$ build/gui/topotide               # run GUI
$ build/gui/topotide --help        # run batch mode
$ build/test/topotide_test         # run unit tests
```
