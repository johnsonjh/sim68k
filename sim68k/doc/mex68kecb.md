# CP/M-68K on the MEX68KECB

## Introduction

I recently stumbled on a blog entry about building a simple CP/M-68K system from
scratch. This reminded me of my third computer. (The first was a COSMAC Elf
built using the Popular Electronics articles and the second was a scratch built
6809 based board.)

Sometime around 1984 I purchased a Motorola MEX68KECB. I also purchased and
built a Heathkit H-19 terminal to go with it. (I ended up buying some extra
parts from Heathkit and installed it inside along with couple of floppy drives.
Sort of a strange H-89.) While I started with fig-FORTH I used that to install
CP/M-68K. An interesting process.

The CP/M-68K distribution disks provided some help in porting to new systems by
including an image of the BDOS (main CP/M code) in S-record format. Once loaded
all you needed to do was patch a couple of addresses that linked to your machine
specific BIOS. This provided a path for installation that did not require a
large tool set.

I used FORTH to load the BDOS image. (I forget how I loaded the BIOS. It might
have been from tape and written using the MEX68KECB one line assembler.) My
first BIOS was dead simple and nearly useless. It had no sector buffer so the
mismatch between the CP/M sector size of 128 and the physical sector size of
1024 really slowed things down. But I quickly built a new BIOS using the CP/M
tools which was a lot faster. My final BIOS included enough buffer space so that
it limited disk seeks to the read directory extents.

I haven't used this machine in nearly 20 years and the H-19 terminal (into which
I stuffed the MEX68KECB, floppy disk interface card, and two TEAC 5.25 inch disk
drives) no longer powers up. But I have a mild interest in my old code and
discovered something called cpmtools to read the old disks.

## cpmtools

In order to use cpmtools you must either use a disk definition from the included
table or add one. Yet the details of how the diskdefs file works are missing.
Add to that the problem that my memory of the format used is a bit fuzzy and
reading the disks will take some work.

One problem is sector skew and how heads are handled. The diskdef has no way of
describing the number of heads. Looking at the provided file it appears as
though it is handled by telling it the number of sectors per cylinder as sectors
per track. But how the code handles the translation is never mentioned. Add to
this the sector skew problem. I used a skew to improve transfer speed and the
order of my 1K sectors is 0, 2, 4, 1, 3. But since I have to tell cpmtools that
I have 10 sectors/track will it do it as: 0, 2, 4, 6...? I also noticed an entry
in the diskdefs that has a skewtab. Which is of course completely undocumented.
Sigh.

My first crack didn't work out too well. First I had to track down my floppy
drive which is a combo 3.5/5.25 unit. I got that hooked up and then it took a
while to figure out that Linux doesn't load the floppy module by default
anymore. Then there is the problem that my brain dead motherboard only supports
a single drive. I had to swap connectors so it would access the 5.25 drive
rather than the 3.5. Cpmtools didn't work too well so then I tried another tool:
libdsk. After futzing around with that for a while I discovered that I needed to
use the density specifier that means "double density disk in high density
drive". While I was trying to figure that out (the dskutil program was reporting
missing address mark) I stumbled on some old handwritten format documentation.

With the WD 2797 controller you format a disk by using the write track command.
Certain values produce special results. Here is what I used:

```text
   +------------------------------------------------------------------------+
   | num bytes |    value     |                    notes                    |
   |-----------+--------------+---------------------------------------------|
   | 60        | 4E           | preamble                                    |
   |-----------+--------------+---------------------------------------------|
   | 12        | 00           | start of sector (repeat for each of five    |
   |           |              | sectors)                                    |
   |-----------+--------------+---------------------------------------------|
   | 3         | F5           | address mark                                |
   |-----------+--------------+---------------------------------------------|
   | 1         | FE           |                                             |
   |-----------+--------------+---------------------------------------------|
   | 1         | track number | (0-79)                                      |
   |-----------+--------------+---------------------------------------------|
   | 1         | side         | (0 or 1)                                    |
   |-----------+--------------+---------------------------------------------|
   | 1         | sector       | (1-5)                                       |
   |-----------+--------------+---------------------------------------------|
   | 1         | 01           | sector length                               |
   |-----------+--------------+---------------------------------------------|
   | 1         | F7           |                                             |
   |-----------+--------------+---------------------------------------------|
   | 22        | 4E           |                                             |
   |-----------+--------------+---------------------------------------------|
   | 12        | 00           |                                             |
   |-----------+--------------+---------------------------------------------|
   | 3         | F5           | address mark                                |
   |-----------+--------------+---------------------------------------------|
   | 1         | FB           |                                             |
   |-----------+--------------+---------------------------------------------|
   | 1024      | E5           | sector data                                 |
   |-----------+--------------+---------------------------------------------|
   | 1         | F7           | write two byte CRC                          |
   |-----------+--------------+---------------------------------------------|
   | 24        | 4E           | end repeated section                        |
   |-----------+--------------+---------------------------------------------|
   | 718       | 4E           | outrun at end of track                      |
   +------------------------------------------------------------------------+
```

As each track is written only the track and side information has to be changed
to match.

Now that I have figured out that I can use libdsk to read the format I need to
convince cpmtools to do so as well. But that diskdefs documentation problem
kicks up again. But at least I have verified that I have two reserved tracks
(one cylinder) at the beginning. It also looks like there are 6 sectors
allocated to the directory. (192 entries)

Recompiling cpmtools helped, but not enough. While dskutil finds and likes my
.libdskrc file, when cpmtools calls libdsk, it does not.

After much thrashing I found that I could set the disk parameters using setfdprm
(after downloading fdutils), copying the disk using dd, and finally using
cpmtools to copy the files off. The copy using dd is achingly slow so it is
apparently getting one sector per revolution. I don't think I have enough
patience to process all of the hundred or so disks that I have but I have sucked
up two. One has what looks like one of my last versions of the BIOS for the
MEX68KECB hardware. The other looks like a BIOS for new hardware that I worked
on for a bit that had an interface to IBM PC (8 bit only) hardware. At least the
floppy driver says it is for a 8272.

I also found a copy of my boot code. I had to type this in using the TUTOR one
line assembler so it is as short as I could make it. I had this memorized and
could type it in really fast.

```text
 0x0900  move.l  #$2000f,a0         * pointer to PT FD-2 controller
         move.l  #$f81000,a1        * address to load data to
         move.l  a1,$70             * set FDC interrupt vector
         moveq.l #7,d7              * used for bit tests
         move.b  #2,(a0)            * select 2MHZ clock
         clr.b   2(a0)              * issue 2797 restore command
         btst    #6,(a0)            * wait for track zero output
         beq     $91a
         btst    d7,2(a0)           * wait till disk ready
         bne     $920
         clr.b   (a0)               * switch to normal 2797 clock rate
         move.b  #1,6(a0)           * set sector register to 1
         move.b  #$88,2(a0)         * read sector command
         move.w  #$2300,sr          * enable FDC interrupt level
         btst    d7,(a0)            * wait for data ready
         beq     $938
         move.b  8(a0),(a1)+        * copy data
         bra     $938
```

It has been a long time so the details are a bit fuzzy but I have tried to
comment it as well as I can remember. While the PT FD-2 controller was mostly
stock I made one significant modification to it. At the normal clock speed for
5.25" disks its fastest step rate made my TEAC drives sound awful. By doubling
the clock rate I could match the step rate match the drive and they were nearly
silent when stepping. So address $2000f actually points to a latch that selects
the clock. I can also read some status bits from the FDC at this address as
well. The actual WD2797 registers then start at 2(a0).

The FDC interrupt vector points to the start of the loaded data so when the
transfer complete interrupt happens the code jumps to the freshly loaded code.
That code then continues the boot process.

For CP/M-68K there is a special reduced version (CPMLDR) that needs to run and
it is bigger than my 1K sector size so the first thing the freshly loaded code
has to do is read in the rest of the reserved tracks. Both side 1 and side 2 are
reserved for a total of 10K.

I don't know if I have enough interest in this old system to drag out the
hardware and see if it still runs. But I may read a few of the old disks. If
anyone is interested in that old code, my contact information is on the home
page.

## Old Code

After more effort than I expected, I was able to read my old disks. Most of the
trouble was the result of my selection of 1K physical sectors. This was
non-standard but as I recall, I figured that by using 1K sectors I could get
800KB per disk rather than 720KB.

First I had to convince Linux to load the floppy driver since it wasn't doing it
by default. (sudo modprobe floppy) Then I finagled the drive parameters into
shape using either a call to dskutil (part of libdsk) or setfdprm. I then used
dd (or ddrescue if needed) to copy the contents of the floppy. By using the flag
for direct I/O I increased the read speed to a blistering (not) 5KB/s which was
tolerable. Finally I invoked cpmtools (cpmls and cpmcp) to find out what was
present and then move it to my Linux file system. Along the way I found some old
projects.

One interesting one was a replacement for the CP/M-68K line input routine:
condbdos. According to the documentation I wrote, this was an attempt to match
what CP/M-3 provided. At the least it provided command line editing and a very
simple one line history.

I started by attempting to create a C source file which when compiled produced
code identical to the original. I was able to do this except for a couple of
unsigned comparisons. Once I had that I then added the editing and history
features.

Another useful tool that I built was something to automate using the C compiler.
As it was you had to use not very bright batch files. I think I copied the code
I used as a starting point for my version of CC from somewhere but after 20+
years I have no idea where. The one tricky part was being able to execute other
programs. This required my CC program to have two parts. The first was a short
stub in assembly to keep it small that would load the actual CC program into
memory. But it relocated it to the top of the memory space and then lowered the
top of memory so any called programs wouldn't (hopefully) overwrite it. In any
case this made compiling programs easier.

## Resurrecting the MEX68KECB

I dragged the H-19 out of the closet and plugged it in to see if it still worked
after 20 years of neglect. It did for a while and then went dark. Presumably
something in the power supply died. Rather than deal with all of that I instead
pulled the MEX68KECB out of it. After rebuilding the serial cable (It had a DB25
and I need DB9 and I couldn't find any adapters but plenty of new DB9's.) I
connected it to my bench power supply. I was happy to be greeted with the TUTOR
prompt in the minicom window. So that works. Here is a picture of it:

## MEX68KECB

You can see that it has been modified quite a bit. The primary modification was
to replace the stock 4116 (16 kilobit) DRAMs with 41256 (256 kilobit) devices.
This requires a bit of surgery because the 4116 requires three power supplies:
5V, 12V and -5V. The only other place the +/-12V supplies are used are in the
RS-232 drivers. Then I needed to modify the address multiplexers to add four
more bits and of course change the address decode. I ended up with a slightly
odd memory map. The MEX68KECB comes with 32KB of memory running from 0x000000 to
0x01ffff and I needed to keep that but I couldn't continue above that because
that is where the I/O and ROM are located. So the memory is mapped to start at
0xF00000 and run to 0xffffff. In addition it also responds to 0x000000 to
0x01ffff. This has the advantage that a 16 bit version of fig-Forth can access
64K because loading a 16 bit address into a MC68000 address register
automatically is sign extended. (0x007fff is followed by 0xff8000, not 0x008000)

512KB wasn't enough so I then piggy-backed another set of 41256 DRAM onto the
first set for a final total of 1MB. This is plenty for CP/M and in fact I used
quite a bit of it for a RAM disk to speed things up.

I found a web page that has a pdf file of the MEX68KECB manual and schematics.
It also has sources for a version of XINU for it. As luck would have it, I have
a copy of the text for XINU so I may have to play with it.

The next step is to get the FDC and floppies working. I pulled the TEAC FD55F's
out of the H-19 and put them into an external floppy case/power supply. (I
forget what I bought the case for and haven't used it in years but it is still
around.) Connecting the FDC is tricky. It is Peripheral Technology FD-2 that has
been modified. The first mod being a short ribbon cable to a DIP16 header rather
than the S50 bus connector. The second being this wire dangling from it. Luckily
I found some of my notes in the FD-2 manual that tells me that this wire taps a
2MHZ clock on the MEX68KECB card.

There are a couple of wires soldered directly to the 7805 regulator on the FD-2.
After a quick check to verify that this is on the input side, I connected it to
the +12V output of my bench supply. This works but I noticed that the 7805
heatsink is really a lot hotter than I care for.

After connecting the floppy cable I typed in the pre-boot code (see above) and
started it up and it all works. CP/M-68K starts up, the ramdisk is initialized
and basic program development tools are loaded:

```text
 TUTOR  1.3 > GO 900
 PHYSICAL ADDRESS=00000900
 CP/M-68K Loader Ver. 1.0


 CP/M-68K(tm) Version 1.2  03/20/84
 Copyright (c) 1984 Digital Research, Inc.

 CP/M-68K BIOS Version 2.0
 TPA =   256K

 A>AUTOST.SUB

 A>INIT C:
 Do you really want to init disk C ? Y
 A>PIP C:=PIP.68K

 A>C:PIP C:=STAT.68K

 A>C:PIP C:=MAKE.68K

 A>C:PIP C:=CC.*

 COPYING -
 CC.68K
 CC.REL

 A>C:PIP C:=AS*.*

 COPYING -
 AS68.68K
 AS68SYMB.DAT

 A>C:PIP C:=EMACS.68K

 A>C:PIP C:=EMACS.RC

 A>C:

 C>USER 1

 1C>STAT LST:=LPT:

 1C>
```

All appears to be working but with that hot 7805 I turned it off.

## A new case

Some packaging is in order to eliminate the clutter and potential hazards. I
think I have an open frame power supply that will do the trick (surplus from All
Electronics many years ago. I think it was for the TI 99/4.) although it
provides +5V, +12V, and -5V. The -5V isn't strictly what is required but since
the only thing it is running are the RS-232 drivers I don't think it will matter
much. Bypassing the 7805 regulator on the FD-2 might be a good idea.

A nice clear case would be neat and I have a sheet of clear acrylic that might
do the trick. Something in a 12X9X4 inch size would work.

## Upgrades

SD memory card interface

The MEX68KECB doesn't have anything even vaguely resembling an SPI port but I
would like to use SD flash memory cards. I have had several ideas on ways to
handle this:

1. Bit banged using the MC68230 PIT. (Connecting a printer seems unlikely at
this point.) 2. Add an I/O port consisting of a couple of latches (data out and
clock) and an input. 3. Program a GAL as an I/O port which automagically
generates a clock cycle when a bit is written and latches the resulting input
data bit. 4. Program a GAL to control a shift register (74299, or another GAL)
so that it can be done a byte at a time instead of bit. 5. Program some
micro-controller to be a byte wide I/O port and use its built in SPI hardware.
If it were a MCU that could run on 3.3V with 5V tolerant I/O it could also do
the level shifting.

After taking a closer look at the documentation I decided that the path of least
resistance was to bit bang it using the PIT. Several pins on port B of the PIT
are brought out to the 50 pin connector but not used by the printer interface.
For level conversion I am going to use a CD4049B CMOS inverter running on 3.3V.
I think it will tolerate the outputs of the PIT (While the nominal range is
0-5V, this is an NMOS part and doesn't drive high very well.) and I will also
buffer the data line coming back just as some extra protection for the SD card.

Because I am using 4 bits of port B to do all of the work, all of the bit
flipping has to be done with read-modify-write operations which really slows
things down.

# SD memory card file system

SD (and SDHC) memory cards now exceed the 512MB upper limit of the CP/M-68K file
system and they are cheap. So if I want to use them I need to think about them a
bit more.

It would be nice if when the card is inserted the BIOS is smart enough to figure
out the details of the file system. It would also be nice if large cards could
be split (sort of like partitions in the MSDOS/Windows world). To that end,
something like partitions are in order so long as it doesn't look so much like a
regular partition that Windows would get confused if it were inserted on such a
machine.

CP/M supports a maximum of 16 drives (A to P) so the maximum is 16 512MB
partitions for a total of 8GB. A stunningly large amount of memory. There is no
need for all the extra information in the MSDOS partitions nor are extended
partitions needed. By allocating 32 long words (32 bits each) in the first
sector all of the partions can be described. This even leaves some space for
boot code if needed.

The basic layout is:

* MBR * reserved sectors (CPMLDR) * Partition 1 (A:) * etc.

In addition the first sector of each partition is reserved and used to store
information about the file system in that partition. This will describe the size
of allocation units, number of directory entries, etc.

On second thought, the idea of a CP/M file system with 512MB of data is just too
scary since it really isn't much of a file system at all. I have a couple of
128MB SD cards so maybe I will only use them.
