# Project 1: System Inspector
Author: Ramneet Kaur

# What's it about?
This project is essentially about building a Linux system inspector utility that gives a user an simple verson of a activty monitor on their screen, with access to information about the processes and hardware as well. The task information displayed is from the files read into proc. 

# Program Options
The display can be maneuvered through with specific command lines:
You can use:
-a: for displaying all (equivalent to -lrst, default)
-h: for help/usage information
-l: for the Task List
-p procfs_dir: for change the expected procfs mount point (default: /proc)
-r: for hardware Information
-s: for System Information
-t: for Task Information

# All files included
Makefile
display.c
display.h
Doxyfile
inspector.c
logger.h
procfs.c
procfs.h
util.c
util.h

In order to compile the files and run the program:
first enter: make
then: ./inspector

# Want to test?
Run make test to run through and test all cases. To test updated cases run 'make testupdate'


See: https://www.cs.usfca.edu/~mmalensek/cs326/assignments/project-1.html 

To compile and run:

```bash
make
./inspector
```

## Testing

To execute the test cases, use `make test`. To pull in updated test cases, run `make testupdate`. You can also run a specific test case instead of all of them:

```
# Run all test cases:
make test

# Run a specific test case:
make test run=4

# Run a few specific test cases (4, 8, and 12 in this case):
make test run='4 8 12'
```
