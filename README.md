# ShowPDFast
A simple, fast open-source PDF rendering library.

> **WARNING:** This is a work in progress, and it is not meant to be used in any project yet.

I started this project by being unable to find a decent MIT-compliant lib for rendering pdf via C/C++.
This is just a pet project to try some stuff and to have fun. It is not meant to be a complete library for the moment
and is just the start of an experiment.

## Download and build

Dont forget to
```
git submodule udpate --init --recursive
```

### Dependencies

ShowPDFast uses PoDoFo as a backend pdf parser. And PoDoFo depends on several libs. You can provide those
dependencies as you wish but the quickest way to quickstart this repo is to use Vcpkg as a package manager
and install dependencies through it.

To do so, you may be willing to read their [quickstart](https://vcpkg.io/en/getting-started.html). You may also
want to set the environment variable `VCPKG_DEFAULT_TRIPLET` to `x64-windows` and define `VCPKG_INSTALLATION_ROOT`.

From the repo root run
```
vcpkg install fontconfig freetype libxml2 openssl libjpeg-turbo libpng tiff zlib
```

We also use `raylib` for demos
```
vcpkg install raylib
```

### Build

Then config the build using CMake
```
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Debug
```

And compile
```
cmake --build build/ --config Debug
```

**Note:** The CMake list currently shows the `BUILD_SHARED_LIBS` option, but it doesn't work correctly yet and
the lib should only be built as a static library.