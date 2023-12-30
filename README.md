Termix - *Mix it with Linux terminal* 😈
==================================

> *Warning*
> This is just my personal learning project, now under ***VERY VERY*** early proof-of-concept stage 🚧, so please don't expect anything useful yet ⛔.

Termix is yet another compatibility layer and distribution for running a Linux environment on non-Linux platforms,
currently it mainly targets at the Microsoft Windows. 🪟✨

Unlike Cygwin, Termix's design goal is to allow simple command-line applications that doesn't depend on too much advanced and complicated Linux features or behaviors to run with hopefully no porting efforts, and to directly reuse of Linux's existing software ecosystem 🐧💪. In this way, we don't need upstream support to run these software on Termix, as long as they have already supported Linux. ❤️‍🩹

Similar to WINE 🍷, Termix does not try to emulate or rely on virtualization to run a whole real Linux kernel, instead it provides a minimal Linux system call layer and a Libc replacement to allow loading and executing the program directly and thus natively on supported machines. ⏪🚗

Another highlight is support for using native Windows APIs alongside with the Linux ones in Termix is also planned to be added in the future 😈 (the idea is inspired by Cygwin), hence the "mix" in the name.

## Why not just use WSL?

This project can be considered as a WSL alternative. However, it doesn't support running unmodified Linux binaries, programs and libraries in the Termix distribution are recompiled specifically to work under it.

## Project status

### TODO list

#### Milestones

- [x] basic ELF loader (WIP)
    - [ ] dynamic relocation (WIP)
      - [ ] support for ELF shared objects
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

## Licensing

Termix is licensed under the GNU Public License version 2, for more details, check the [LICENSE](LICENSE.txt) file.
