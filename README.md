# VirusRushTheater's fork of AddMusicKFF

This fork was made by VirusRushTheater in order to modify some aspects of
KungFuFurby's AddMusicKFF v1.0.8:

* Remove Visual Studio dependencies and use a CMake build based system.
* Tidy up program dependencies, moving them to the `deploy` folder.
* Make it multiplatform with minor modifications.
* Static Linkage with Asar, which would allow a more seamless integration.
* Compiling AddMusicKFF as a library, which would allow for more complex
  SMW hacking tools to integrate it easily.

## Cloning ##

Please clone this repository by using:

```
git clone --recursive https://github.com/VirusRushTheater/AddMusicKFF.git
```

## Building the project with CMake ##

First of all, create a folder named `build` in the repositor root.

```
mkdir build
```

Then, configure the project. Enter the `build` directory and call `cmake ..`
from there.

```
cd build
cmake ..
```

And compile it. This will copy the executable into the `deploy` folder. The
project is still dependent on an external `libasar.so`/`asar.dll` provided
manually by you.

```
cmake --build .
```
