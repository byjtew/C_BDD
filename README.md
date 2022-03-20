# C_BDD: C Byjtew Dumb Debugger

[![CMake](https://github.com/byjtew/C_BDD/actions/workflows/cmake.yml/badge.svg)](https://github.com/byjtew/C_BDD/actions/workflows/cmake.yml)

C_BDD is a free and open source program analysis tool for C programs on UNIX systems only.

## Supported features

- [X] **Program execution**: Run - Pause - Stop
- [X] **ELF**: Display headers and sections
- [X] **Segmentation faults**: Catch and analyse
- [X] **Stack**: Display
- [X] **Register values**: Display
- [X] **Program assembly**: Display entirely or a restricted section
- [X] **Breakpoint**: Stop the program before a function or elf-address

## Quick start

Please read the [requirements](#Requirements) specific notes before running this script.

```bash
git clone https://github.com/byjtew/C_BDD.git
mkdir build
cd build
cmake ..
make -j
./bin/c_bdd ../samples/stable_program
```

## Documentation & Usage

### Program arguments commands

- `-h`/`--help`: Show help message
- `-v`/`--version`: Show bugger version

### CLI commands

Before typing any fo theses commands, you should run the debugger:\
`./bin/C_BDD <traced-program-path>`

- `r`/`run <parameters>`: Run the traced program
- `restart`: Restart the traced program from the beginning
- `s`/`step`: Run one assembly instruction in the traced-program
- `stop`: Try to stop the traced program
- `kill`: Force the traced program to stop (a memory leak issue may occur)
- `status`: Display the overall traced program status
- `functions <full>`: Display every functions
- `reg`/`registers`: Display every registers values (as %llu only)
- `d`/`dump <n>`: Display the program (assembly + C) with the next *n* lines at the current location
- `bp <address|function-name|line>`: Creates a breakpoint at the specified location
- `bp off <address|function-name>`: Removes a breakpoint from the specified location
- `bp show`: Display every breakpoints
- `bt`/`backtrace`: Show the current stack.
- `elf`: Show elf information about the traced program.
- `help`: Show help message
- `version`: Show bugger version

## Branches

The project is currently setup in two main branches:

- `develop` - This branch has often new features, but might also contain breaking changes. Might be unstable
- `main` - This branch contains the latest stable release.

## Support

### Help / Discord

For any questions not covered by the documentation or for further information about the debugger, or to simply engage
with like-minded individuals, I encourage you to contact me directly on GitHub or Discord.

### Bugs / Issues

If you discover a bug in the debugger, please
[search the issue tracker](https://github.com/byjtew/C_BDD/issues?q=is%3Aopen+is%3Aissue)
first. If it hasn't been reported, please
[create a new issue](https://github.com/byjtew/C_BDD/issues/new/choose) and ensure to provide as much information as
possible so that I can assist you as quickly as possible.

## Requirements

### Software requirements

- **UNIX-based OS**
- CMake
- TBB
- git
- [libunwind-dev](https://github.com/libunwind/libunwind)
- [liblzma-dev](https://github.com/kobolabs/liblzma)
- [libfmt-dev](https://github.com/fmtlib/fmt)
- [objdump](https://www.man7.org/linux/man-pages/man1/objdump.1.html) (Recommended)
