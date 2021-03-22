// WinIncludes.h
#pragma once

#ifdef _WIN32

// We generally want lean & mean win32, but a bug in VLD requires us to call a function that's stripped out by this 
// define, so we only define it when VLD isn't running.
#ifdef NDEBUG
    #define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

// Windows.h ends up including a file that defines these macros, which causes all sorts of compilation issues when 
// code tries to define functions with the same name.  A good example of eastl::numeric_limits::max().  To get around 
// this issue, we just forcibly undefine these macros whenever we #include windows.h.
#ifdef max
    #undef max
#endif
#ifdef min
    #undef min
#endif

#endif
