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
    #include <stdlib.h>
    #include <malloc.h>
    #include <crtdbg.h>
    #include <stdint.h>

    #pragma warning(push)
    #pragma warning(disable : 4291)  // 'void *operator new(size_t,const char *,int)': no matching operator delete found; memory will not be freed if initialization throws an exception

    inline void* DebugAlloc(size_t size, const char* filename, int lineNum)
    {
        return _malloc_dbg(size, 1, filename, lineNum);
    }

    inline void DebugFree(void* pMemory)
    {
        _free_dbg(pMemory, 1);
    }

    void* operator new(size_t size, const char* filename, int lineNum);
    void operator delete(void* pMemory);
    void* operator new[](size_t size, const char* filename, int lineNum);
    void operator delete[](void* pMemory);

    #define BLEACH_NEW(_type_) new(__FILE__, __LINE__) _type_
    #define BLEACH_NEW_ARRAY(_type_, _size_) new(__FILE__, __LINE__) _type_[_size_]
    #define BLEACH_DELETE(_ptr_) delete _ptr_
    #define BLEACH_DELETE_ARRAY(_ptr_) delete[] _ptr_

    #if ENABLE_BLEACH_MEMORY_DEBUGGING
        void* operator new(size_t size, const char* filename, int lineNum, uint64_t count);
        void* operator new[](size_t size, const char* filename, int lineNum, uint64_t count);

        namespace MemoryDebug
        {
            void Init();
            void DumpAndDestroy();
            void DumpMemoryRecords();
        }

        #define _MEMORY_DEBUG_INIT() MemoryDebug::Init()
        #define _MEMORY_DEBUG_DUMP_AND_DESTROY() MemoryDebug::DumpAndDestroy()
        #define BLEACH_NEW_BREAK(_type_, _count_) new(__FILE__, __LINE__, _count_) _type_
        #define BLEACH_NEW_ARRAY_BREAK(_type_, _size_, _count_) new(__FILE__, __LINE__, _count_) _type_[_size_]
        #define DUMP_MEMORY_RECORDS() MemoryDebug::DumpMemoryRecords()
    #else  // !ENABLE_BLEACH_MEMORY_DEBUGGING
        #define _MEMORY_DEBUG_INIT() void(0)
        #define _MEMORY_DEBUG_DUMP_AND_DESTROY() void(0)
        #define BLEACH_NEW_BREAK(_type_, _count_) BLEACH_NEW(_type_)
        #define BLEACH_NEW_ARRAY_BREAK(_type_, _size_, _count_) BLEACH_NEW_ARRAY(_type_, _size_)
        #define DUMP_MEMORY_RECORDS() void(0)
    #endif  // ENABLE_BLEACH_MEMORY_DEBUGGING

    #define INIT_LEAK_DETECTOR() \
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); \
        _MEMORY_DEBUG_INIT()

    #define DESTROY_LEAK_DETECTOR() _MEMORY_DEBUG_DUMP_AND_DESTROY()

    #pragma warning(pop)

#else  // !USE_DEBUG_BLEACH_NEW

    #define BLEACH_NEW(_type_) new _type_
    #define BLEACH_NEW_ARRAY(_type_, _size_) new _type_[_size_]
    #define BLEACH_NEW_BREAK(_type_, _count_) BLEACH_NEW(_type_)
    #define BLEACH_NEW_ARRAY_BREAK(_type_, _size_, _count_) BLEACH_NEW_ARRAY(_type_, _size_)
    #define BLEACH_DELETE(_ptr_) delete _ptr_
    #define BLEACH_DELETE_ARRAY(_ptr_) delete[] _ptr_
    #define MEMORY_DEBUG_INIT() void(0)
    #define MEMORY_DEBUG_DUMP_AND_DESTROY() void(0)
    #define DUMP_MEMORY_RECORDS() void(0)

    #define INIT_LEAK_DETECTOR() void(0)
    #define DESTROY_LEAK_DETECTOR() void(0)


#endif  // USE_DEBUG_BLEACH_NEW
