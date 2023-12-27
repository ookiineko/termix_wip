Termix - *Mix it with Linux terminal*
==================================

> *Warning*
> This is just my personal learning project, now under ***VERY VERY*** early proof-of-concept stage, so please don't expect anything useful yet.

Termix aims to be yet another compatibility layer and distribution for running a Linux environment on non-Linux platforms,
currently mainly targeting at the Microsoft Windows.

Unlike Cygwin, Termix's design goal is to allow simple command-line applications to run with hopefully no porting efforts, but to directly reuse of Linux's existing software ecosystem.

Similar to WINE, Termix as a compatibility layer also does not try to emulate processor or rely on virtualization to run a whole real Linux kernel, instead it simply tries to load and execute the program directly and thus natively on supported machines.

However, in some conditions (see the [Windows limitation](#windows-limitation)), it may require recompiling the programs from source in order to load them with Termix, but this usually should not involve any source changes.

Another highlight is support for using native Windows APIs alongside with Linux ones in Termix is also planned to be added (the idea is inspired by Cygwin), thus the "mix" in the name.

## Project status

### TODO list

#### Milestones

- [x] basic ELF loader (WIP)
    - [ ] dynamic relocation (WIP)
    - [ ] setup stack

- [x] get a basic helloworld to run on Windows

#### Short-term

* port to MinGW (WIP)

#### Long-term

* work on a Libc stub or replacement on Windows

#### Optional

* support for 32-bit x86 machines

#### Need help

* ARM/ARM64 support (lacking of device to test)

## Usage

See the [manual](MANUAL.md) (currently available in MarkDown).

## Known issues

### ABI differences

Some potential ABI differences between Linux and Windows may cause all kinds of low-level difficulties or major issues for some features to be implemented and work with Termix (some of them may **NEVER** be possible to be workarounded or implemented), while workarounds might be possbile for some of them, it can hurt performance and make Termix runs slower and less efficient.

### Windows limitation

Currently on Windows, mapping file data to a page size aligned address is not easy and sometimes nearly impossible.

It's probably because of Windows's forward-compatibility decisions, the ability to create file mappings on page size boundary is
never exposed as a public API and it's not even visible to native APIs on anything other than 32-bit x86 machines.

However, Termix's ELF loader depends on this feature to work.

For now a workaround is used, which requires you to recompile all Linux programs with a fixed 64k page size alignment,

this can be done by adding a linker option: `-Wl,-z,max-page-size=0x10000` to `CFLAGS` when compiling the program.

## Licensing

Termix is licensed under the GNU Public License version 2, for more details, check the [LICENSE](LICENSE.txt) file.
