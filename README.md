# Android Unreal Engine Dumper / UE Dumper

Generate sdk and functions script for unreal engine games on android.

The dumper is based on [UE4Dumper-4.25](https://github.com/guttir14/UnrealDumper-4.25)
project.

## Features

* Supported ABI ARM64, ARM, x86 and x86_64
* Dump UE offsets, classes, structs, enums and functions
* Generate function names json script to use with IDA & Ghidra etc
* Symbol and pattern scanning to find GUObjectArray, GNames and NamePoolData addresses automatically
* Find GEngine and GWorld in Data segment
* Find ProcessEvent index and offset (64bit only for now)
* Dump UE library from memory

## Currently Supported Games

* Arena Breakout Mobile
* Delta Force Mobile
* Farlight 84 Mobile
* PUBG Mobile
* Valorant Mobile

## Usage

Simply load or inject the library with whichever method and let it do it's thing.
Run logcat with tag filter "UEDump3r" for dump logs.
The dump output will be at the game's external data folder (/sdcard/Android/data/\<game\>/files) to avoid external storage permission.

## Output Files

### AIOHeader.hpp

* An all-in-one dump file header

### Offsets.hpp

* Header containing UE Offsets

### Logs.txt

* Log file containing dump process logs

### Objects.txt

* ObjObjects dump

### script.json

* If you are familiar with Il2cppDumper script.json, this is similar. It contains a json array of function names and addresses

## Adding a new game to the Dumper

Follow the prototype in [GameProfiles](AndUEDumper/src/UE/UEGameProfiles)

You can also use the provided patterns to find GUObjectArray, GNames or NamePoolData.

## Building

You need to have 'make' installed on your OS and NDK_HOME env variable should be set.

```bash
git clone --recursive https://github.com/MJx0/AndUEDumper
cd AndUEDumper/AndUEDumper
make clean && make
```

## TODO

* Sort Generated Packages & Solve Dependencies
* [Dumper-7](https://github.com/Encryqed/Dumper-7) Auto Find Offsets

## Credits & Thanks

* [UE4Dumper-4.25](https://github.com/guttir14/UnrealDumper-4.25)
* [Il2cppDumper](https://github.com/Perfare/Il2CppDumper)
* [Dumper-7](https://github.com/Encryqed/Dumper-7)
* [UEDumper](https://github.com/Spuckwaffel/UEDumper)
