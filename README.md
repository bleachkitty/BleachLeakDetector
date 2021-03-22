# Bleach Leak Detector Overview
This is a very simple leak detection system that wraps the CRT.  It's mostly here for students to be able to use without having to write their own.  ;)

There are three major features of the leak detection system:

1) It detects memory leaks and prints them to the debug output window.
2) You can track all allocations and deallocations so that any allocations that are freed are displayed with an ID number.
3) Once you have the ID number, you can break into the debugger when that specific ID is allocated.

Note that #1 and #2 are quite expensive, so they are not enabled by default.

# Usage
The best way to get started using the Bleach Leak Detector is to open the solution file and look at Example.cpp.  It's a simple showcase of the features.

To use it in your own projects, you must do the following:
1) Copy BleachNew.h, BleachNew.cpp, and BleachNewConfig.h somewhere into your project.  Alternatively, you could probably build it as a library that you link in.
2) Replace all calls to `new` with calls to the `BLEACH_NEW` or `BLEACH_NEW_ARRAY` macros as appropriate.
3) Do the same with `delete` and `BLEACH_DELETE`/`BLEACH_DELETE_ARRAY`.
4) Add a call to `INIT_LEAK_DETECTOR` at the top of main() before any memory allocations happen.
5) Add a call to `DESTROY_LEAK_DETECTOR` at the bottom of main() after all memory has been released.
6) Make any changes to the BleachNewConfig.h file that are appropriate.  See the comments in that file for details.

That's about it!

#Further Work
If you have any suggestions for future work, let me know.  I'm happy to consider feature requests and review any pull requests if you find something that's broken.
