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
// In order to use this leak detector, you should #define USE_DEBUG_BLEACH_NEW and set the value to 1.  This should 
// only be done in debug mode.  It's also a Windows-only thing since we're using Microsoft-specific CRT extensions.
//---------------------------------------------------------------------------------------------------------------------
#if defined(_WIN32) && defined(_DEBUG)
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
#define ENABLE_BLEACH_MEMORY_DEBUGGING 1
