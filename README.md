## libmmd

MikuMikuDance importer

## Building

    mkdir target && cd target                # - create build target directory
    cmake -DCMAKE_INSTALL_PREFIX=build ..    # - run CMake, set install directory
    make                                     # - compile

## TODO
* Add tests
* Support importing animations
* Figure out what most of the imported data is and organize them more sanely
* Hide structs from public API
* Provide API to read from memory

## License

zlib, see LICENSE file for more information
