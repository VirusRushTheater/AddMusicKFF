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

## Overview of the refactoring ##

The code has been turned from a CLI-centered program into a library-centered
program where the CLI is just an interface.

The easier way to work with this code as a library is to include the `AddmusicK.h`
header, and remember the `AddMusic` namespace. You can use the directive
`using namespace AddMusic;` if you don't want to deal with it.

The `SPCEnvironment` class is the heart of the program, where general options
and behavior are set in order to compile and adapt the SPC driver to generate
playable SPC files from MML files and optionally a set of BRR samples.

`ROMEnvironment` extends its functionality and gives it ROM patching
capabilities, but requires a patcheable Super Mario World ROM with enough
free space to put songs in.

You can use a convenience struct `SPCEnvironmentOptions` to initialize each one of
these structs. It holds the command-line options that used to be in the original
program, such as turning off checks or duplicate sample optimization, among
others.

A default set of AMK lists and the default SPC driver will be embedded in
compile time, making this program easier to distribute and more straightforward
for the user. These are generated scanning the `src/package/asm` and 
`src/package/boilerplate` folders and generating a Package object for each,
which you can extract by taking the static objects `asm_package` and
`boilerplate_package` and run their `extract` method into the path of your
choice.