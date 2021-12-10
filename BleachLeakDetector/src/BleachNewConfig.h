//---------------------------------------------------------------------------------------------------------------------
// MIT License
// 
// Copyright(c) 2021 David "Rez" Graham
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

//---------------------------------------------------------------------------------------------------------------------
// BLEACH_WINDOWS is defined if we are building for the Windows platform.  You don't need to change this.
//---------------------------------------------------------------------------------------------------------------------
#ifndef BLEACH_WINDOWS
    #if defined(WIN32) || defined(_WIN32) || defined(__TOS_WIN__)
        #define BLEACH_WINDOWS
    #endif
#endif


//---------------------------------------------------------------------------------------------------------------------
// This must be set to 1 in order to use the leak detector.  If it's set to 0, the BLEACH_* macros will just call new 
// and delete.  Note that this should only be done in debug mode.  It's also a Windows-only thing since we're using 
// Microsoft-specific CRT extensions.
//---------------------------------------------------------------------------------------------------------------------
#if defined(BLEACH_WINDOWS) && defined(_DEBUG)
    #define USE_DEBUG_BLEACH_NEW 1
#else
    #define USE_DEBUG_BLEACH_NEW 0
#endif

//---------------------------------------------------------------------------------------------------------------------
// Set this to 1 to enable memory logs for each allocation.  This is very slow, but it shows you every individual 
// allocation and deallocation so that when something leaks, you can see exactly which allocation caused it.  This 
// is especially useful when you have a loop or something that calls new over and over, but only one of those 
// allocations is leaking.
//---------------------------------------------------------------------------------------------------------------------
#define ENABLE_BLEACH_ALLOCATION_TRACKING 0

//---------------------------------------------------------------------------------------------------------------------
// If set to 1, use EASTL internally.  EASTL is required to be in the include path and linked as normal.  If set to 0, 
// the leak detector will use whatever containers are in the std namespace.
//---------------------------------------------------------------------------------------------------------------------
#define BLEACH_NEW_USE_EASTL 0
