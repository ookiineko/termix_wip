Termix - *Mix it with Linux terminal*
==================================

> *Warning*
> This is a learning project, nothing useful yet.
>
> For documentation, view the [development page](DEVELOPMENT.md).

Termix aims to be yet another compatibility layer and distribution for running a Linux environment on non-Linux platforms,
currently mainly targeting at the Microsoft Windows.

Unlike Cygwin, Termix's design goal is to allow running simple command-line applications with hopefully no porting efforts, but allow directly reusing the existing Linux software ecosystem.

Similar to WINE, Termix is also not an emulator, it directly loads and executes the program natively on supported machines. However, in some conditions, it may require recompiling the programs from source (see the [Windows limitation](#windows-limitation)) to run, but usually this should not involve any source changes.

## Project status

### TODO list

#### Milestones

- [ ] basic ELF loader
    - [ ] dynamic relocation (WIP)
    - [ ] setup stack

- [ ] get a basic helloworld to run on Windows

#### Long-term

* work on a Libc stub or replacement on Windows

#### Optional

* support for 32-bit x86 machines

#### Need help

* ARM/ARM64 support (lacking of device to test)

## Known issues

### Windows limitation

Currently on Windows, mapping file data to a page size aligned address is not easy and sometimes nearly impossible.

It's probably because of Windows's forward-compatibility decisions, the ability to create file mappings on page size boundary is
never exposed as a public API and it's not even visible to native APIs on anything other than 32-bit x86 machines.

However, Termix's ELF loader depends on this feature to work.

For now a workaround is used, which requires you to recompile all Linux programs with a fixed 64k page size alignment,

this can be done by adding a linker option: `-Wl,-z,max-page-size=0x10000` to `CFLAGS` when compiling the program.

## Licensing

Termix is licensed under the GNU Public License version 2, for more details, check the [LICENSE](LICENSE.txt) file.
