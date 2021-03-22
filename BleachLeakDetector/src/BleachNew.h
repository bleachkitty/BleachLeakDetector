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
#include "BleachNewConfig.h"

#if USE_DEBUG_BLEACH_NEW

    #if ENABLE_BLEACH_ALLOCATION_TRACKING
        #include <cstdint>
    #endif

    #pragma warning(push)
    #pragma warning(disable : 4291)  // 'void *operator new(size_t,const char *,int)': no matching operator delete found; memory will not be freed if initialization throws an exception

    //-----------------------------------------------------------------------------------------------------------------
    // Internal interface used by the macros below.  These should not be called directly; call the macros instead.
    //-----------------------------------------------------------------------------------------------------------------
    namespace BleachNewInternal
    {
        void InitLeakDetector();
        void DestroyLeakDetector();

        #if ENABLE_BLEACH_ALLOCATION_TRACKING
            void InitMemoryTracker();
            void DumpAndDestroyMemoryTracker();
            void DumpTrackerMemoryRecords();
        #endif
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Base overloads for global new/delete.  You shouldn't call these directly; use the BLEACH_NEW / BLEACH_DELETE 
    // macros below.
    //-----------------------------------------------------------------------------------------------------------------
    void* operator new(size_t size, const char* filename, int lineNum);
    void operator delete(void* pMemory);
    void* operator new[](size_t size, const char* filename, int lineNum);
    void operator delete[](void* pMemory);

    //-----------------------------------------------------------------------------------------------------------------
    // Initialization & Destruction.  Call INIT_LEAK_DETECTOR() at the top of main() and DESTROY_LEAK_DETECTOR() at 
    // the bottom of main().
    //-----------------------------------------------------------------------------------------------------------------
    #define INIT_LEAK_DETECTOR() BleachNewInternal::InitLeakDetector()
    #define DESTROY_LEAK_DETECTOR() BleachNewInternal::DestroyLeakDetector()

    //-----------------------------------------------------------------------------------------------------------------
    // Allocation / Deallocation macros.  Replace your calls with new/delete with these.  If you don't, leaks will 
    // still be tracked but you won't know where they came from.
    //-----------------------------------------------------------------------------------------------------------------
    #define BLEACH_NEW(_type_) new(__FILE__, __LINE__) _type_
    #define BLEACH_NEW_ARRAY(_type_, _size_) new(__FILE__, __LINE__) _type_[_size_]
    #define BLEACH_DELETE(_ptr_) delete _ptr_
    #define BLEACH_DELETE_ARRAY(_ptr_) delete[] _ptr_

    //-----------------------------------------------------------------------------------------------------------------
    // Memory tracking.
    //-----------------------------------------------------------------------------------------------------------------
    #if ENABLE_BLEACH_ALLOCATION_TRACKING
        // memory tracking overloads
        void* operator new(size_t size, const char* filename, int lineNum, uint64_t count);
        void* operator new[](size_t size, const char* filename, int lineNum, uint64_t count);

        // Call these macros to break on a specific allocation number.
        #define BLEACH_NEW_BREAK(_type_, _count_) new(__FILE__, __LINE__, _count_) _type_
        #define BLEACH_NEW_ARRAY_BREAK(_type_, _size_, _count_) new(__FILE__, __LINE__, _count_) _type_[_size_]

        // You can call this to dump the the current allocations if you want.  It's not required or used anywhere 
        // in the system.
        #define DUMP_MEMORY_RECORDS() BleachNewInternal::DumpTrackerMemoryRecords()

    #else  // !ENABLE_BLEACH_ALLOCATION_TRACKING
        // Macros for when memory tracking is disabled.
        #define BLEACH_NEW_BREAK(_type_, _count_) BLEACH_NEW(_type_)
        #define BLEACH_NEW_ARRAY_BREAK(_type_, _size_, _count_) BLEACH_NEW_ARRAY(_type_, _size_)
        #define DUMP_MEMORY_RECORDS() void(0)
    #endif  // ENABLE_BLEACH_ALLOCATION_TRACKING

    #pragma warning(pop)

#else  // !USE_DEBUG_BLEACH_NEW
    #define INIT_LEAK_DETECTOR() void(0)
    #define DESTROY_LEAK_DETECTOR() void(0)

    #define BLEACH_NEW(_type_) new _type_
    #define BLEACH_NEW_ARRAY(_type_, _size_) new _type_[_size_]
    #define BLEACH_NEW_BREAK(_type_, _count_) BLEACH_NEW(_type_)
    #define BLEACH_NEW_ARRAY_BREAK(_type_, _size_, _count_) BLEACH_NEW_ARRAY(_type_, _size_)
    #define BLEACH_DELETE(_ptr_) delete _ptr_
    #define BLEACH_DELETE_ARRAY(_ptr_) delete[] _ptr_
    #define DUMP_MEMORY_RECORDS() void(0)

#endif  // USE_DEBUG_BLEACH_NEW
