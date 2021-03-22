// BleachNew.h
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

        #define MEMORY_DEBUG_INIT() MemoryDebug::Init()
        #define MEMORY_DEBUG_DUMP_AND_DESTROY() MemoryDebug::DumpAndDestroy()
        #define BLEACH_NEW_BREAK(_type_, _count_) new(__FILE__, __LINE__, _count_) _type_
        #define BLEACH_NEW_ARRAY_BREAK(_type_, _size_, _count_) new(__FILE__, __LINE__, _count_) _type_[_size_]
        #define DUMP_MEMORY_RECORDS() MemoryDebug::DumpMemoryRecords()
    #else  // !ENABLE_BLEACH_MEMORY_DEBUGGING
        #define MEMORY_DEBUG_INIT() void(0)
        #define MEMORY_DEBUG_DUMP_AND_DESTROY() void(0)
        #define BLEACH_NEW_BREAK(_type_, _count_) BLEACH_NEW(_type_)
        #define BLEACH_NEW_ARRAY_BREAK(_type_, _size_, _count_) BLEACH_NEW_ARRAY(_type_, _size_)
        #define DUMP_MEMORY_RECORDS() void(0)
    #endif  // ENABLE_BLEACH_MEMORY_DEBUGGING

    #define INIT_LEAK_DETECTOR() \
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); \
        MEMORY_DEBUG_INIT()

    #define DESTROY_LEAK_DETECTOR() MEMORY_DEBUG_DUMP_AND_DESTROY()

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

#endif  // USE_DEBUG_BLEACH_NEW
