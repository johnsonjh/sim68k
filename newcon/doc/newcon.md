# NEWCON

## Introduction

NEWCON.C is a replacement for the CONBDOS.O portion of CP/M-68K (tm DRI). To use
it you must have a copy of CPMLIB ( library of object files makeing up the BDOS
and CCP of CP/M-68K ) and a copy of the object for your BIOS. It is necessary to
relink these files to produce a CPM.REL file.

NEWCON.C provides an enhanced version of BDOS function #10 - read console
buffer. All other console functions are unchanged. The enhancments are several
new editing functions and command line recall. These functions behave as much
like those in CP/M+ as was possible without actually having a CP/M+ system to
use.

## Functions

```text
        ^A      Moves the cursor one one character to the left. No effect
                at beginning of line.

        ^B      Moves the cursor to the beginning of the line. If already
                at the beginning of the line, moves to the end.

        ^C      If first character entered on line, warmboots the system.

        ^E      Moves the cursor to beginning of the next line. Does not
                send the command line to the system.

        ^F      Moves the cursor one character to the right. No effect
                at the end of the line.

        ^G      Deletes the character at the current cursor position
        DEL     without moving the cursor.

        ^H      Moves the cursor back one character and deletes that
                character. No effect at beginning of line.

        ^I      Moves the cursor to the next tab stop. Tabs are at
        TAB     every eighth column.

        ^J      Sends the command line to the system and moves the cursor
        LF      to the beginning of the next line.

        ^K      Deletes all characters from the current position to the
                end of the line.

        ^M      Sends the command line to the system and moves the cursor
        CR      to the beginning of the next line.

        ^R      Retypes the current line. Places a # at the current
                position, moves to the next line, and retypes any
                characters entered so far. Positions the cursor at the
                end of the line.

        ^U      Discards all characters in the line. Places a # at the
                current cursor position and moves to a new line. Any
                characters entered can be recalled with ^W

        ^W      Recalls a previously entered line and makes it available
                for editing. Places the cursor at the end of the line.
                The keys ^J (LF) ^M (CR) and ^U determine what characters
                can be recalled.

        ^X      Deletes all characters to the left of the cursor and moves
                the cursor to the beginning of the line.
```

As an option you can enable another feature of the function #10 call if you can
access the location where your BIOS stores its DMA pointer. If you call function
#10 with register D1 = 0, then the function will assume that an initialized
buffer is present at the DMA address. The buffer will consist of the following:

```text
        mx  nc  c1  c2  c3 .... 0

        mx      Is the maximum number of characters that the buffer can
                hold. The mx and nc bytes do not count in this total.
        nc      The number of characters in the buffer. You do not
                need to initialize this before calling.
        c1 c2 ...
                This a null terminated string. The characters up to
                the null will be counted and displayed. You can then
                edit them as usual.
```

You can enable this function by changeing the extern decleration of "def_dma" to
match the symbol used in your bios. Note the symbol in your bios would be
\_def_dma because of the leading underscore added by the C compiler. You then
define INIT before compiling.

## Installation

After compiling to a .o file you must replace the conbdos.o file in CPMLIB.
First make a backup copy of CPMLIB.

Then:

```text
        ar68 d cpmlib conbdos.o
                                to delete the conbdos.o module
        ar68 ra bdosif.o cpmlib newcon.o
                                this inserts newcon.o after bdosif.o
        which is where conbdos.o was
```

Then relink as described in the System Guide

The best feature of this program is that now you can change it to do anything
that you want. You can add more editing commands or an improved command line
recall.

## Original author

David W. Schultz CIS [73157,2242]
