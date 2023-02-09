# FileSystemUtil
 > cross-platform(Linux/Windows) C++ filesystem API wrapper

# Require
 - C++17
 - MSVC/GCC

## Build
```
git clone https://github.com/XUranus/FileSystemUtil.git
cd FileSystemUtil
mkdir build
cd build
cmake ..
cmake --build .
```

## Demo Usage
```
fsutil -ls <directory path>   ----  list subdirectory/file of a directory
fsutil -stat <path>           ----  print the detail info of directory/file
fsutil -mkdir <path>          ----  create directory recursively
fsutil --drivers              ----  list drivers
fsutil --volumes              ----  list volumes
```