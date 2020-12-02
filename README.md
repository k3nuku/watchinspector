# watchinspector
A simple Apple Watch internal directory extraction tool

## Coverage
- [x] Pairing
- [x] AFC Service
- [x] Inode information
- [x] Directory walk
- [ ] Single file read (TODO)
- [ ] Directory extraction (TODO)

## Prerequisites
First, you need to build a latest libimobiledevice from their repository if you wanna do this from latest WatchOS.
```bash
[using homebrew]
$ brew install -v --HEAD --build-from-source usbmuxd libimobiledevice
```

## Build arguments
you can build this project using g++, clang++ or etc.
```
$ g++ --std=c++11 -g afcapi.cpp main.cpp -o watchinspector -limobiledevice-1.0
```

## Usage
Just execute built one. It automatically find and pair with connected watch and extract directories.
