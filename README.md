# VirusRushTheater's fork of AddMusicKFF

This fork was made by VirusRushTheater in order to modify some aspects of
KungFuFurby's AddMusicKFF v1.0.9 RC:

* Remove Visual Studio dependencies and use a CMake build based system.
* Tidy up program dependencies, moving them to the `deploy` folder.
* Make it multiplatform with minor modifications.
* Including Asar repository into the building system, allowing a more straightforward usage of it as a library.
* Merge the music hacking base directory system (in other words, the clean `deploy` folder) into the program, so it can be restored to a clean slate with a single call, relieving the user of the burden of making backup directories.
* Compiling AddMusicKFF as a library, which would allow for more complex SMW hacking tools to integrate it easily.
* Removing as many boilerplate dependencies as possible (Addmusic_list.txt, etc.) if generating SPC files.

## Cloning ##

Please clone this repository by using:

```
git clone --recursive https://github.com/VirusRushTheater/AddMusicKFF.git
```

## Building the project with CMake ##

First of all, create a folder named `build` in the repository root.

```
mkdir build
```

Then, configure the project. Enter the `build` directory and call `cmake ..`
from there.

```
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

And compile it.
```
cmake --build .
```

The results will be in a `deploy` folder inside your `build` folder, where the
program, together with the SPC drivers will be copied.

Please, be aware that, if you want to build this project with debug symbols you
will need to install the library `asan` (Address Sanitizer) in your system.

In Ubuntu it would be:

```
sudo apt install libasan8
```