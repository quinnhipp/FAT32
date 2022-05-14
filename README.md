# FAT32
FAT32 file system emulation in C

Project Requirements:
For this project, you will be emulating part of the FAT32 file system. You are NOT allowed to mount the virtual
image I provide as part of your solution; your code (which you must write using only the resources I provide) must
be what reads the image, rather than using any O/S resources or system calls. The goal of this project is for you to
write the kind of “read the disk as raw sectors and interpret it as a file system” code that’s already in the O/S.
You may not use any source code for FAT32 drivers for other operating systems.
You will write a program (FAT32.c) that will take a single command-line parameter – the name of a file containing
the image of a FAT32 drive.
Your program will then prompt the user with “\>”, and wait for the user to type a command, which you will handle.
You may assume that everything resides in the root directory – there is no need to traverse the directory tree or deal
with path names. You are to ignore hidden files and/or directories – If you come across any, pretend they’re not
there.
The commands your program must support are:
  1) DIR
If the root directory is empty, display “File not found”; otherwise, for every entry in the directory, display its date
and time of creation, its size, and the filename (in both 8.3 and LFN formats), just like the DIR command would
with the “/x” option:
Output a summary line, telling how many file(s) was (were) listed, and what its (their) total file size is (not the space
taken up by their clusters; the sum of the sizes of the files themselves)
  2) EXTRACT <file>
First, locate <file>. Then, copy the contents of the file outside of the image file.
For example:
EXTRACT somefile.txt
Would cause your program to:
1) If the file does not exist, display “File not found”
2) If the file does exist, you would create somefile.txt on your VM, in the same folder as the executable.
The contents of the resulting somefile.txt would be the same as the one IN the image (this is essentially
“copy from INSIDE the image to OUTSIDE the image”).
  3) QUIT
Exits your program
