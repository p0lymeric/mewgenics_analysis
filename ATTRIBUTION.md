### Attribution

This repository gratefully uses the following libraries.

### C/C++

#### Linked
* MinHook
  * Optionally used for function hooking.
  * `Copyright (C) 2009-2017 Tsuda Kageyu. All rights reserved.`
  * BSD 2-Clause License
  * https://github.com/TsudaKageyu/minhook/blob/master/LICENSE.txt
* Detours
  * Used for function hooking and DLL injection.
  * `Copyright (c) Microsoft Corporation.`
  * MIT License
  * https://github.com/microsoft/Detours/blob/main/LICENSE
* LZ4
  * Used to decompress savefile cats and stream compress Amoeba transaction logs.
  * `Copyright (c) Yann Collet. All rights reserved.`
  * BSD 2-Clause License
  * https://github.com/lz4/lz4/blob/dev/lib/LICENSE
* Dear ImGui
  * Used to define debugging GUIs.
  * `Copyright (c) 2014-2026 Omar Cornut`
  * MIT License
  * https://github.com/ocornut/imgui/blob/master/LICENSE.txt
* SDL3
  * Used to interface with Mewgenics' game engine.
  * `Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>`
  * zlib License
  * https://github.com/libsdl-org/SDL/blob/main/LICENSE.txt
* sqlite3
  * Used to access Mewgenics saves.
  * `The author disclaims copyright to this source code.`
  * Public Domain
  * https://sqlite.org/copyright.html
* LibTomCrypt
  * Used to hash Mewgenics.exe to detect version mismatches.
  * `LibTomCrypt, modular cryptographic library -- Tom St Denis`
  * Unlicense
  * https://github.com/libtom/libtomcrypt/blob/develop/LICENSE

#### Integrated
* GON
  * Used to interpret GON objects taken from game memory (types/gon.hpp).
  * `Copyright (c) 2018 Tyler Glaiel`
  * MIT License
  * https://github.com/TylerGlaiel/GON/blob/master/LICENSE

#### Referenced
* STL
  * Referenced to write types/msvc.hpp
  * `Copyright (c) Microsoft Corporation.`
  * Apache License v2.0 with LLVM Exception
  * https://github.com/microsoft/STL/blob/main/LICENSE.txt
* parallel-hashmap
  * Referenced to write types/phmap.hpp
  * `Copyright (c) 2019, Gregory Popovitch - greg7mdp@gmail.com`
  * Apache License 2.0
  * https://github.com/greg7mdp/parallel-hashmap/blob/master/LICENSE

### Python

* lz4
  * Used to decompress savefile cats and Amoeba transaction logs.
  * `Copyright (c) 2012-2013, Steeve Morin All rights reserved.`
  * BSD 3-Clause License
  * https://github.com/python-lz4/python-lz4/blob/master/LICENSE
* networkx
  * Used by `1_fishnet.py` to convert the game's pedigree map to GraphML format.
  * `Copyright (c) 2004-2025, NetworkX Developers Aric Hagberg <hagberg@lanl.gov> Dan Schult <dschult@colgate.edu> Pieter Swart <swart@lanl.gov> All rights reserved.`
  * BSD 3-Clause License
  * https://github.com/networkx/networkx/blob/main/LICENSE.txt
