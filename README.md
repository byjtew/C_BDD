# C_BDD: C/C++ Byjtew Dumb Debugger

[![CMake](https://github.com/byjtew/C_BDD/actions/workflows/cmake.yml/badge.svg)](https://github.com/byjtew/C_BDD/actions/workflows/cmake.yml)

C_BDD is a free and open source program analysis tool written in C++, for UNIX system only. It is designed to support C
& C++ executables.

## Supported features

- [X] **Program execution**: Run - Pause - Stop
- [X] **ELF**: Display headers and sections
- [ ] **Segmentation faults**: Catch and analyse
- [X] **Stack**: Display
- [ ] **Register values**: Display
- [ ] **Function parameters**: Display
- [ ] **Global variables**: Display
- [ ] **Local variables**: Display
- [X] **Program assembly**: Display entirely or a restricted section
- [X] **C/C++ program**: Display entirely or a restricted section
- [ ] **Breakpoint**: Stop the program before a function, line or address
- [ ] **Memory**: Display and trace (de-)allocation

## Quick start

Please read the [requirements](#Requirements) specific notes before running this script.

```bash
git clone https://github.com/byjtew/C_BDD.git
mkdir build
cd build
cmake ..
make -j
./bin/C_BDD ./samples/tutorial_program
```

## Documentation & Usage

### Program arguments commands

- `-m`/`--memory-check`: Run the program with a memory tracing
- `-h`/`--help`: Show help message
- `-v`/`--version`: Show bugger version

### CLI commands

Before typing any fo theses commands, you should run the debugger:\
`./bin/C_BDD <traced-program-path>`

- `r`/`run`: Run the traced program
- `stop`: Try to stop the traced program
- `kill`: Force the traced program to stop (a memory leak issue may occur)
- `status`: Display the overall traced program status
- `functions <full>`: Display every functions
- `d`/`dump <n>`: Display the program (assembly & C/C++) with the next *n* lines at the current location
- `df`/`dump full <n>`: More detailed version of: `dump`
- `s`/`step`: Run one assembly instruction in the traced-program
- `bp <address|function-name|line>`: Creates a breakpoint at the location
- `bp off <address|function-name|line>`: Removes a breakpoint at the location
- `memory`: Show memory allocation status
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
- [addr2line](https://www.man7.org/linux/man-pages/man1/addr2line.1.html) (Recommended)
