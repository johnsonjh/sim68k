# CP/M-68K Simulator

## About

   This is a work in progress but might be good enough for others to play
   with.

   This is built on the framework provided by the [20]Musashi MC68000
   simulator. In addition to simulating a MC68000 the only hardware simulated
   is a console port and the disk system. The code, and especially the
   console I/O assumes it is running under Linux.

## Building

   To build the simulator for CP/M-68K you will need a copy of the code above
   and this. The file cpmsim.c is the simulator and used the example.c
   program as a skeleton on which to build. Build using the makefile supplied
   along with cpmsim.c. This should create an executable named "cpmsim".

## Console

   The console port is a very simple simulation of a serial port. In this
   case a much reduced 6850 ACIA. While I have included code for interrupts,
   I haven't tested them. Standard input and output are switched to raw mode
   so that things like control-C can be handled by CP/M. The settings are
   restored on exit but if the signal handler doesn't get called, this will
   not happen.

## Disks

   The simulator provides for up to 16 simulated disk systems. You can mount
   a file as a simulated CP/M disk with the command line option of "-a
   filename" where "a" can be anything between "a" and "p". The simulator
   determines the size of the attached files when mounted and will not
   attempt to read or write beyond that limit.

   Drive C is attached to the file "diskc.cpm.fs" which is a 16MB file system
   and is included in this package. The image included contains a complete
   working CP/M-68K system. 16MB is much less than the 512MB maximum for
   CP/M-68K but seemed like a workable size. Big enough to hold all the tools
   and several projects. If you need more...

   Drive A simulates a plain vanilla 8" SS SD drive. This is handy for moving
   files into and out of the simulation using cpmtools. This is the default
   for cpmtools.

   I also included a 800K drive B which is what my CP/M-68K system used.

   Although it isn't much faster, drive M is a 16MB ram disk. I measured a
   long compile to be about twice as fast when run from the ram disk.
   Probably not worth the effort.

   Other drives of different sizes can be added by including the appropriate
   changes in the BIOS. Creating a new file system image is as easy as using
   the Unix tool "dd".

   If by some chance you run two instances of the simulator at the same time
   that mount the same file system images, only the first will be able to
   write to them. The second will inform you when an image is mounted read
   only.

## Booting

   There are two options for booting CP/M-68K. From binary images or from the
   drive C file system.

## Binary images

   The CP/M-68K distribution provides for a way to bring it up without the
   tools included. An image of the BDOS in S-record format is included. By
   loading this along with a rudimentory BIOS at a fixed address you can get
   things started. This is how I started here. By including the option "-s"
   on the commaned line the simulator will load the BDOS from "cpm400.bin"
   and the BIOS from "simbios.bin".

   While most of the programs in user area 0 of the drive C file system were
   relocated using this boot option, some were not. This will result in those
   programs not loading. If this happens you will have to dig up the
   relocatable version. I don't expect this to be a big problem because there
   is really no reason to boot this way.

## Drive C

   This replicates the normal multi-stage boot process. The CP/M loader is
   located in the system portion of the drive C file system and is loaded
   into memory and executed. It locates CPM.SYS on the file system and loads
   that into memory.

   You can make changes to CP/M using the available tools (C compiler,
   assembler, linker) creating a new CPM.SYS. Just be sure to keep backups.
   The source for the provided system is in user area 2.

## Exiting

   Ending the simulation is as easy as typing "bbye" which signals the
   simulator to wrap things up. The terminal is set to raw mode so ^C doesn't
   work. That gets passed through so that CP/M-68K can handle it.

## Extras

   I have included a few things that are not a part of the standard
   distribution:

```text
User           Contents
14   uemacs V3.6.1
13   uemacs V?
11   CC
10   make
9
8
7
6
5    loader and regular BIOS, etc
4    CP/M loader lib source
3
2    C libraries
1    C include files
```

## Micro-emacs

   Ed is usable as a text editor but that isn't saying much. A screen editor
   is much betters and micro-emacs fits the bill. Two versions are provided.
   The first is of unknown vintage as I can't find any indication of a
   version number. This is named "emacs". The second is built from the
   version 3.6.1 sources and is named "uemacs". The 3.6.1 version has key
   bindings more like what I am used to. Both use ANSI escape codes and work
   well for me in an xterm.

## cc

   Compiling C programs using the provided submit files, which can't be
   nested, is tedious at best. This program makes it more like the usual
   process in Unix. Compiling something like micro-emacs as easy as "cc *.c
   -o emacs.rel". This program comes in two parts. The first part is cc.68k
   and it loads the second part into the high part of the TPA. It then
   changes the TPA size before starting that program. This lets CC load and
   run other programs that have been converted from relocatable form using
   reloc.

## make

   The version in user 0 just creates a submit file. The source in user 10 is
   for something better although it requires work to port from its MSDOS
   origins.

## whereis

   Locating a file somewhere in the user areas can be a pain so this program
   lets you specify a filename with optional wildcards and it prints out a
   list of aveerything that matches.

## split

   For those times when you need to burn EPROMS and need to split a binary
   into the even and odd parts. Checks for a CP/M-68K executable file header
   and skips it if found.

   Not all of the files from the CP/M-68K distribution are here. Just what
   made it over while I was working on this.

## Changing things

   Besides changes to the BIOS you can also change CP/M-68K. One thing I did
   long ago was add improved command line editing. I had a chance to use a
   CPM+ system once and liked that a lot. My new readline function is part of
   the included CPMLIB. Souce is in newcon.c with documentation in
   newcon.doc. This function is available via the BDOS call as well.

   Other modules can be swapped out of CPMLIB using ar68. Changes are a lot
   easier now with the original source code available.

   If you break it just dig out your backup of the disk C image and start
   again.

## The Code

   Unzip the Musashi source first and run it through dos2unix or it will not
   compile. At least it wouldn't for me. Then unzip my file and be sure to
   use the m68kconf.h file in it. Then run make.
