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

//---------------------------------------------------------------------------------------------------------------------
// We have to override global new & delete so that they call _malloc_dbg() and _free_dbg() or we won't get the file
// and line number where the allocation took place.  Only do this in debug mode.
//---------------------------------------------------------------------------------------------------------------------
#if USE_DEBUG_BLEACH_NEW

        #include <cstdint>

    //-----------------------------------------------------------------------------------------------------------------
    // Internal interface used by the macros below.  These should not be called directly; call the macros instead.
    //-----------------------------------------------------------------------------------------------------------------
    namespace BleachNewInternal
    {
        void InitLeakDetector();
        void DumpAndDestroyLeakDetector();
        void* DebugAlloc(size_t size, const char* filename, int lineNum, uint64_t breakAtCount = 0);  // 0 means no breakpoint
        void DebugFree(void* pMemory);
    }

    //-----------------------------------------------------------------------------------------------------------------
    // Base overloads for global new/delete.  You shouldn't call these directly; use the BLEACH_NEW / BLEACH_DELETE 
    // macros below.
    //-----------------------------------------------------------------------------------------------------------------
    // scalar new / delete
    void* operator new(size_t size, const char* filename, int lineNum);
    void operator delete(void* pMemory);
    void operator delete(void* pMemory, const char*, int);

    // array new / delete
    void* operator new[](size_t size, const char* filename, int lineNum);
    void operator delete[](void* pMemory);
    void operator delete[](void* pMemory, const char*, int);

    //-----------------------------------------------------------------------------------------------------------------
    // Initialization & Destruction.  Call INIT_LEAK_DETECTOR() at the top of main() and DESTROY_LEAK_DETECTOR() at 
    // the bottom of main().
    //-----------------------------------------------------------------------------------------------------------------
    #define BLEACH_INIT_LEAK_DETECTOR() BleachNewInternal::InitLeakDetector()
    #define BLEACH_DUMP_AND_DESTROY_LEAK_DETECTOR() BleachNewInternal::DumpAndDestroyLeakDetector()

    //-----------------------------------------------------------------------------------------------------------------
    // Allocation / Deallocation macros.  Replace your calls to new/delete with these.  If you don't, leaks will still 
    // be tracked but you won't know where they came from.
    // 
    // Basic Functionality:
    // 
    // BLEACH_NEW:          This replaces new.  Calling it looks like this: 
    //                          Foo* pFoo = BLEACH_NEW(Foo(params));
    // BLEACH_NEW_ARRAY:    This replaces new[].  Calling it looks like this: 
    //                          Foo* pFooArray = BLEACH_NEW_ARRAY(Foo, arraySize);
    // BLEACH_ALLOC:        This allocates a raw block of memory, like calling operator new() or malloc().  No 
    //                      constructors will be called, so if you need to construct something in this block, you 
    //                      will need to use placement new.  It looks like this:
    //                          void* pRawMem = BLEACH_ALLOC(sizeInBytes);
    // BLEACH_DELETE:       Scalar form of delete.  The counterpart to BLEACH_NEW.
    // BLEACH_DELETE_ARRAY: Array form of delete.  The counterpart to BLEACH_NEW_ARRAY.
    // BLEACH_FREE:         Destroys a raw block of memory allocated with BLEACH_ALLOC.  Note that no destructors are 
    //                      called, so you will have to do this manually.
    //-----------------------------------------------------------------------------------------------------------------
    #define BLEACH_NEW(_type_) new(__FILE__, __LINE__) _type_
    #define BLEACH_NEW_ARRAY(_type_, _size_) new(__FILE__, __LINE__) _type_[_size_]
    #define BLEACH_ALLOC(_size_) BleachNewInternal::DebugAlloc(_size_, __FILE__, __LINE__)
    #define BLEACH_DELETE(_ptr_) delete _ptr_
    #define BLEACH_DELETE_ARRAY(_ptr_) delete[] _ptr_
    #define BLEACH_FREE(_ptr_) BleachNewInternal::DebugFree(_ptr_)

    //-----------------------------------------------------------------------------------------------------------------
    // Memory tracking.
    //-----------------------------------------------------------------------------------------------------------------
    #if ENABLE_BLEACH_ALLOCATION_TRACKING
        namespace BleachNewInternal
        {
            void DumpMemoryRecords();
        }

        // memory tracking overloads
        void* operator new(size_t size, const char* filename, int lineNum, uint64_t count);
        void* operator new[](size_t size, const char* filename, int lineNum, uint64_t count);
        void operator delete(void* pMemory, const char*, int, uint64_t);
        void operator delete[](void* pMemory, const char*, int, uint64_t);

        // The BREAK versions of the macros work just like the non-break versions except that the debugger will break when 
        // the count (last param) is reached.  This allows you to break on the specific allocation that's causing the leak.
        // See the example for details.
        #define BLEACH_NEW_BREAK(_type_, _count_) new(__FILE__, __LINE__, _count_) _type_
        #define BLEACH_NEW_ARRAY_BREAK(_type_, _size_, _count_) new(__FILE__, __LINE__, _count_) _type_[_size_]
        #define BLEACH_ALLOC_BREAK(_size_, _count_) BleachNewInternal::DebugAlloc(_size_, __FILE__, __LINE__, _count_)

        // You can call this to dump the current allocations if you want.  It's not required or used anywhere 
        // in the system.
        #define BLEACH_DUMP_MEMORY_RECORDS() BleachNewInternal::DumpMemoryRecords()

    #else  // !ENABLE_BLEACH_ALLOCATION_TRACKING
        // Macros for when memory tracking is disabled.
        #define BLEACH_NEW_BREAK(_type_, _count_) BLEACH_NEW(_type_)
        #define BLEACH_NEW_ARRAY_BREAK(_type_, _size_, _count_) BLEACH_NEW_ARRAY(_type_, _size_)
        #define BLEACH_ALLOC_BREAK(_size_, _count_) BLEACH_ALLOC(_size_, __FILE__, __LINE__)
        #define BLEACH_DUMP_MEMORY_RECORDS() void(0)
    #endif  // ENABLE_BLEACH_ALLOCATION_TRACKING

#else  // !USE_DEBUG_BLEACH_NEW
    #define BLEACH_INIT_LEAK_DETECTOR() void(0)
    #define BLEACH_DUMP_AND_DESTROY_LEAK_DETECTOR() void(0)
    #define BLEACH_DUMP_MEMORY_RECORDS() void(0)

    #define BLEACH_NEW(_type_) new _type_
    #define BLEACH_NEW_ARRAY(_type_, _size_) new _type_[_size_]
    #define BLEACH_ALLOC(_size_) ::operator new(_size_)

    #define BLEACH_NEW_BREAK(_type_, _count_) BLEACH_NEW(_type_)
    #define BLEACH_NEW_ARRAY_BREAK(_type_, _size_, _count_) BLEACH_NEW_ARRAY(_type_, _size_)
    #define BLEACH_ALLOC_BREAK(_size_, _count_) BLEACH_ALLOC(_size_)

    #define BLEACH_DELETE(_ptr_) delete _ptr_
    #define BLEACH_DELETE_ARRAY(_ptr_) delete[] _ptr_
    #define BLEACH_FREE(_ptr_) ::operator delete(_ptr_)
#endif  // USE_DEBUG_BLEACH_NEW
