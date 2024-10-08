Megasource is a CMake-buildable collection of all [Jenjin's](https://github.com/JenjinEngine/Jenjin) dependencies from [LÖVE](https://love2d.org).

Prerequisites
-------------
 - [CMake 3.5][cmake].
 - [Visual Studio 2022][Visual Studio] (or another CMake-compatible compiler).

Building
--------
To build just the dependencies for 64 bit x86 using Visual Studio 2022:
```sh
$ git clone https://github.com/JenjinEngine/megasource megasource
$ cd megasource
$ cmake -G Ninja -A x64 -S . -B build
$ cmake --build build --config Release
```

[Jenjin]: https://github.com/JenjinEngine
[Love2D]: http://love2d.org
[CMake]: http://www.cmake.org/
[NSIS]: http://nsis.sourceforge.net
[Visual Studio]: https://visualstudio.microsoft.com/downloads/
[Generators]: https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#visual-studio-generators
[Architectures]: https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2016%202019.html#platform-selection
