
## 0. hi

MOSTLY unmodified src from the limewire link in the discord server  
only modifications i did is *hopefully* fix sdl3

> [!CAUTION]
EXPECT CRASHES OR BUGS, ORIGINAL DEVS HAVE STATED THAT THIS PROJECT  
IS MOSTLY UNFINISHED AND VERY UNSTABLE

### 0.1. build the thing

> [!WARNING]
> THIS GUIDE IS MADE ONLY FOR **LINUX**, OTHER PLATFORMS HAVE NOT BEEN TESTED  
mainly bc i have no fucking clue how to do ts in windows i barely figured out
this on my own

requirements:
* clang
* cmake
* qt shit:
```
# Arch: pacman -S qt6-base qt6-webengine
# Ubuntu: apt install qt6-base-dev qt6-webengine-dev qt6-webengine-dev-tools libqt6webenginecore6-bin
```
* [vcpkg](https://github.com/microsoft/vcpkg)

ok steps:
* clone this repo
* clone the vcpkg repo in the root of the project (should be like this: [project]/vcpkg/...)
* cd into vcpkg and run `./bootstrap-vcpkg.sh`
* go back to the root folder and run:
```
mkdir build
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++
```
* wait til it compiles the zobillion dependencies and configures shit,
* then cd into `build` and run `make`, this will compile the project

enjoy your mess, if you didnt understand my instructions uhhh sorry not sorry lmao  

below is the original README.txt (now with markdown !)

## 1. Overview

Aya is a robust, customizable, and otherwise heavily modified fork of the
Roblox client codebase which was leaked to the public internet around March
2016. Relative to the original codebase, Aya contains numerous improvements,
enhancements, and other features, all of which are listed in the CHANGELOG.

Aya was created with the aspiration to foster positive change and further
innovation in the legacy Roblox community. It is our hope that every use of
Aya aligns with this vision.

Please read all the provided documentation files to get a full grasp on how
to use Aya. This file contains a glossary of all relevant information,
whereas INSTALL contains information how to fully set up and use Aya.
You should definitely read LICENSE, as it describes the relevant terms for
using Aya, as well as key legal information you need to know if you choose
to use Aya.

## 2. Documentation

### 2.1. Overall documentation

    README.md                 This file
    docs/API.md               Aya custom API reference
    docs/INSTALL.md           Installation/compile instructions for Aya
    docs/CHANGELOG.md         Full list of changes from the original 2016
                              Roblox source code

### 2.2. Project structure

    client                   Implements the Aya game, delivered to end-users
    client/app               Universal Svelte app UI for Player/Studio/Server
    client/core              Foundational code used across the client apps
    client/player            Aya Player, used to connect to multiplayer games
    client/studio            Aya Studio, used to edit and test levels
    client/server            Aya Server, used to host multiplayer game sessions
    client/web-helper        Aya CEF integration
    client/thumbnail-helper  Call with regsvr32 on Windows for .ayal thumbnails
                             in the Windows File Explorer
    client/updater           Aya auto-updater for instances
    
    engine                   The underlying engine that the client applications
                             rely on. This is a heavily modified version of the
                             Roblox game engine source code leak, dated 3/16/2016
    engine/app               The heart of the engine
    engine/core              Fundamental components used throughout the engine
    engine/gfx               3D rendering
    engine/network           Multiplayer component of the game engine
    
    docs                     Documentation on how to use and install Aya
    resources                Static resources such as images that are used in Aya
    
    third-party              Third party dependencies accessible as Git submodules


## 3. Noteworthy changes

Aya contains many significant changes relative to the original codebase.
Here is a list of some of the more noteworthy ones that are important to
know when using Aya.

  - Most of Roblox's anti-cheat measures have been removed or stripped
    out of the codebase entirely, including:

      * The real-time program memory checker (NetPmc) as well as the
        golden hash checker. The justification for this is because it is
        too difficult to port to non-x86 Windows systems as a lot of the 
        functionality relies on fundamental properties of the Microsoft 
        Visual C++ compiler and the PE file structure.

      * VMProtect integration has been removed entirely. Unlike other
        proprietary libraries, there is no alternative put in its place.
        However, you may want to tackle obfuscating code during the
        compilation stage with projects such as obfuscated-llvm so that
        the control flow of the application is obfuscated to the point that
        it is near-impossible for malicious actors to exploit.

  - All proprietary or otherwise closed-source dependencies have been
    removed entirely and replaced with open source alternatives.

  - The client applications have been almost entirely rewritten with the
    goal of being as lightweight as possible as well as having equivalent
    functionality across all platforms (in particular, Windows and Linux).

  - All dependencies have been updated to their newest versions.
