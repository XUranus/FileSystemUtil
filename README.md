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
cmake .. -A x64
cmake --build . --config=Release
```

## Demo Usage
```
fsutil -ls <directory path>   ----  list subdirectory/file of a directory
fsutil -stat <path>           ----  print the detail info of directory/file
fsutil -mkdir <path>          ----  create directory recursively
fsutil -getsd <path>          ----  list security descriptor string of win32 path
fsutil -copysd <path>         ----  copy security descriptor from src to target
fsutil -sparse <path>         ----  query sparse file allocate ranges
fsutil --drivers              ----  list drivers
fsutil --volumes              ----  list volumes
```