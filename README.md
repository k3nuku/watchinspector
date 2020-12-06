# watchinspector [![Build Status](https://travis-ci.com/sokdak/watchinspector.svg?branch=master)](https://travis-ci.com/sokdak/watchinspector)
A simple Apple Watch internal directory extraction tool

## Coverage
- [x] Pairing
- [x] AFC Service
- [x] Inode information
- [x] Directory walk
- [x] Single file read
- [x] Directory extraction

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
After build process, You can see usages when execute binary with ```-h``` option.
