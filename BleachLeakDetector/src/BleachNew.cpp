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
#include "BleachNew.h"

#if USE_DEBUG_BLEACH_NEW

#include <crtdbg.h>

//---------------------------------------------------------------------------------------------------------------------
// Memory debugging.  We maintain two hash maps, one for all the records keyed by address and one to keep track of 
// the incremental ids, keyed by source hash.  The source hash is the hash of the filename and line number.  When an 
// allocation happens, we generate a new record and insert it into the records hash and then increment the count for 
// it in the counts hash.
// 
// This process gives us two important things:
//      1) Allocations are categorized by source.
//      2) All allocations of a particular source will have a consistent id.
// 
// So if you have a loop allocating 10 objects, you will have 10 different records keyed by the addresses which 
// contain the order in which that allocation was made.  You can then use BLEACH_NEW_BREAK() to break on that specific 
// allocation.
//---------------------------------------------------------------------------------------------------------------------
#if ENABLE_BLEACH_ALLOCATION_TRACKING
    #include <mutex>
    #include <atomic>

    #if BLEACH_NEW_USE_EASTL
        #include <EASTL/hash_map.h>
        #include <EASTL/string.h>
    #else
    #include <unordered_map>
    #include <string>
    #endif

    //-----------------------------------------------------------------------------------------------------------------
    // Windows is required.
    //-----------------------------------------------------------------------------------------------------------------
    #ifdef _WIN32
        #define WIN32_LEAN_AND_MEAN
        #include <Windows.h>

        // Windows.h ends up including a file that defines these macros, so we undef them.
        #ifdef max
            #undef max
        #endif
        #ifdef min
            #undef min
        #endif
    #else
        #error "BleachNew requires a Windows platform."
    #endif

    //-----------------------------------------------------------------------------------------------------------------
    // Macro to break into the debugger.
    //-----------------------------------------------------------------------------------------------------------------
    #ifndef BREAK_INTO_DEBUGGER
        #if defined(_MSC_VER)
            extern void __cdecl __debugbreak(void);
            #define BREAK_INTO_DEBUGGER() __debugbreak()
        #else
            #error "Couldn't generate BREAK_INTO_DEBUGGER() macro."
        #endif
    #endif

    namespace BleachNewInternal
    {
        //-----------------------------------------------------------------------------------------------------------------
        // Raw Allocator for EASTL
        //-----------------------------------------------------------------------------------------------------------------
        #if BLEACH_NEW_USE_EASTL
            class RawEastlAllocator
            {
                const char* m_pName;

            public:
                RawEastlAllocator(const char* name = "Raw EASTL") : m_pName(name) {}
                RawEastlAllocator(const RawEastlAllocator&) = default;
                RawEastlAllocator([[maybe_unused]] const RawEastlAllocator& x, const char* name) : RawEastlAllocator(name) {}
                RawEastlAllocator& operator=(const RawEastlAllocator&) = default;

                void* allocate(size_t n, [[maybe_unused]] int flags = 0) { return ::operator new(n); }
                void* allocate(size_t n, [[maybe_unused]] size_t alignment, [[maybe_unused]] size_t offset, [[maybe_unused]] int flags = 0) { return allocate(n); }
                void  deallocate(void* p, size_t n) { ::operator delete(p, n); }

                const char* get_name() const { return m_pName; }
                void        set_name(const char* name) { m_pName = name; }
            };

            // All RawEastlAllocator's are totally interchangeable.
            inline bool operator==([[maybe_unused]] const RawEastlAllocator& a, [[maybe_unused]] const RawEastlAllocator& b) { return true; }
            inline bool operator!=([[maybe_unused]] const RawEastlAllocator& a, [[maybe_unused]] const RawEastlAllocator& b) { return false; }

            // container aliases
            using Filename = eastl::basic_string<char, RawEastlAllocator>;
            using StringHasher = eastl::hash<const char*>;
        #else
            using Filename = std::string;
            using StringHasher = std::hash<const char*>;
        #endif  // BLEACH_NEW_USE_EASTL

        //---------------------------------------------------------------------------------------------------------------------
        // Memory debugger class, used for storing memory allocation records.
        //---------------------------------------------------------------------------------------------------------------------
        class MemoryDebugger
        {
            struct MemoryRecord
            {
                uint64_t id;  // unique ID per allocation which is incrementally updated
                uint32_t allocLocationHash;  // hash of location and line number to serve as a unique allocation point
                void* pAddress;  // the address of the returned allocation

                MemoryRecord(uint32_t _allocLocationHash, void* _pAddress, uint64_t _id)
                    : allocLocationHash(_allocLocationHash)
                    , pAddress(_pAddress)
                    , id(_id)
                {
                    //
                }
            };

            struct CountRecord
            {
                Filename filename;
                int line;
                uint64_t count;

                CountRecord(const char* _filename, int _line, uint64_t _count)
                    : filename(_filename)
                    , line(_line)
                    , count(_count)
                {
                    //
                }
            };

            #if BLEACH_NEW_USE_EASTL
                using Counts = eastl::hash_map<uint32_t, CountRecord, eastl::hash<uint32_t>, eastl::equal_to<uint32_t>, RawEastlAllocator>;
                using Records = eastl::hash_map<size_t, MemoryRecord, eastl::hash<size_t>, eastl::equal_to<size_t>, RawEastlAllocator>;
            #else
                using Counts = std::unordered_map<uint32_t, CountRecord>;
                using Records = std::unordered_map<size_t, MemoryRecord>;
            #endif  // BLEACH_NEW_USE_EASTL


            Counts m_counts;  // memory hash => CountRecord
            Records m_records;  // pointer => MemoryRecord
            std::recursive_mutex m_mutex;
            std::atomic_bool m_destroying;

        public:
            MemoryDebugger()
                : m_destroying(false)
            {
                // find() crashes if the bucket array has a zero length, so this gets around that
                m_counts.reserve(4);
                m_records.reserve(4);
            }

            ~MemoryDebugger()
            {
                m_destroying = true;  // *sigh*
            }

            void AddRecord(void* pPtr, const char* filename, int lineNum, uint64_t breakPoint = 0)
            {
                if (m_destroying)
                    return;

                std::lock_guard<std::recursive_mutex> lock(m_mutex);

                // generate the hash
                const uint32_t allocHash = HashMemoryEntry(filename, lineNum);

                // add or update the memory records
                auto findIt = m_counts.find(allocHash);
                if (findIt != m_counts.end())
                {
                    CountRecord& countRecord = findIt->second;
                    ++countRecord.count;
                    if (countRecord.count == breakPoint)
                    {
                        BREAK_INTO_DEBUGGER();
                    }
                    m_records.emplace(reinterpret_cast<size_t>(pPtr), MemoryRecord{ allocHash, pPtr, countRecord.count });
                }
                else
                {
                    if (breakPoint == 1)
                    {
                        BREAK_INTO_DEBUGGER();
                    }
                    m_counts.emplace(allocHash, CountRecord{ filename, lineNum, 1 });
                    m_records.emplace(reinterpret_cast<size_t>(pPtr), MemoryRecord{ allocHash, pPtr, 1 });
                }
            }

            void RemoveRecord(void* pPtr)
            {
                if (m_destroying)
                    return;
                std::lock_guard<std::recursive_mutex> lock(m_mutex);
                m_records.erase(reinterpret_cast<size_t>(pPtr));
            }

            void DumpMemoryRecords()
            {
                static constexpr size_t kBufferLength = 256;

                if (m_destroying)
                    return;

                std::lock_guard<std::recursive_mutex> lock(m_mutex);

                ::OutputDebugStringA("========================================\n");
                ::OutputDebugStringA("Remaining Allocations:\n");

                char buffer[kBufferLength];
                uint64_t rowNum = 0;
                for (const auto& addressRecordPair : m_records)
                {
                    // should be a structured binding, but I want to keep compatible with C++ 14
                    const auto& address = addressRecordPair.first;
                    const auto& record = addressRecordPair.second;

                    std::memset(buffer, 0, kBufferLength);
                    auto findIt = m_counts.find(record.allocLocationHash);
                    if (findIt != m_counts.end())
                        InternalSprintf(buffer, kBufferLength, "%llu> %s(%d)\n    => [0x%x] ID: %llu\n", rowNum, findIt->second.filename.c_str(), findIt->second.line, address, record.id);
                    else
                        InternalSprintf(buffer, kBufferLength, "%llu> (No Record)\n    => [0x%x] ID: %llu\n", rowNum, address, record.id);
                    ::OutputDebugStringA(buffer);
                    ++rowNum;
                }
                ::OutputDebugStringA("========================================\n");
            }

        private:
            static uint32_t HashMemoryEntry(const char* filename, int lineNum)
            {
                uint32_t allocHash = static_cast<uint32_t>(StringHasher()(filename));
                allocHash ^= lineNum;
                return allocHash;
            }

            template <class... Args>
            static void InternalSprintf(char* buffer, size_t sizeOfBuffer, const char* format, Args&&... args)
            {
            #ifdef _MSC_VER
                _sprintf_p(buffer, sizeOfBuffer, format, std::forward<Args>(args)...);
            #else
                sprintf(buffer, format, std::forward<Args>(args)...);
            #endif
            }
        };

        //---------------------------------------------------------------------------------------------------------------------
        // Pointer to the global memory debugger instance.
        //---------------------------------------------------------------------------------------------------------------------
        static MemoryDebugger* g_pMemoryDebugger = nullptr;

        //---------------------------------------------------------------------------------------------------------------------
        // Interface free funcitons.  These are exposed to the interface, but you should prefer the macros instead.
        //---------------------------------------------------------------------------------------------------------------------
        void InitLeakDetector()
        {
            ::OutputDebugStringA("Initializing Bleach Leak Detector.\n");
            _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
            if (!g_pMemoryDebugger)
                g_pMemoryDebugger = new MemoryDebugger;  // purposefully not using the overloaded version of new
        }

        void DumpAndDestroyLeakDetector()
        {
            if (g_pMemoryDebugger)
            {
                DumpMemoryRecords();
                delete g_pMemoryDebugger;
                g_pMemoryDebugger = nullptr;  // purposefully not using the overloaded version of delete
                ::OutputDebugStringA("Exiting Bleach Leak Detector.\n");
            }
        }

        void DumpMemoryRecords()
        {
            if (g_pMemoryDebugger)
                g_pMemoryDebugger->DumpMemoryRecords();
        }

        //---------------------------------------------------------------------------------------------------------------------
        // Internal free functions.
        //---------------------------------------------------------------------------------------------------------------------
        static void AddRecord(void* pPtr, const char* filename, int lineNum, uint64_t breakPoint = 0)
        {
            if (g_pMemoryDebugger)
                g_pMemoryDebugger->AddRecord(pPtr, filename, lineNum, breakPoint);
        }

        static void RemoveRecord(void* pPtr)
        {
            if (g_pMemoryDebugger)
                g_pMemoryDebugger->RemoveRecord(pPtr);
        }
    }  // end namespace BleachNewInternal

#else  // !ENABLED_MEMORY_DEBUGGING

//---------------------------------------------------------------------------------------------------------------------
    // Stubs for the free functions if we're not using logging allocations.
//---------------------------------------------------------------------------------------------------------------------
    namespace BleachNewInternal
{
        void InitLeakDetector() { _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); }
        void DumpAndDestroyLeakDetector() {}
        void DumpMemoryRecords() {}
        static void AddRecord(void*, const char*, int, uint64_t) {}
        static void RemoveRecord(void*) {}
}

#endif  // ENABLE_BLEACH_ALLOCATION_TRACKING


//---------------------------------------------------------------------------------------------------------------------
// Debug memory allocation and deallocation functions.
//---------------------------------------------------------------------------------------------------------------------
namespace BleachNewInternal
{
    // Debug allocators.  These appear to be Microsoft-specific, so I've wrapped them my own functions and pulled 
    // them into their own internal namespace.  This lets us easily replace them based on compiler, OS, or whatever.
    namespace Internal
{
        static void* RawAlloc(size_t size, const char* filename, int lineNum) { return _malloc_dbg(size, 1, filename, lineNum); }
        static void RawFree(void* pMemory) { _free_dbg(pMemory, 1); }
}

    void* DebugAlloc(size_t size, const char* filename, int lineNum, uint64_t breakAtCount /*= 0*/)
{
        void* pPtr = Internal::RawAlloc(size, filename, lineNum);
        AddRecord(pPtr, filename, lineNum, breakAtCount);
    return pPtr;
}

    void DebugFree(void* pMemory)
{
    BleachNewInternal::RemoveRecord(pMemory);
        Internal::RawFree(pMemory);
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Debug new/delete overloads
//---------------------------------------------------------------------------------------------------------------------

// Scalar
void* operator new(size_t size, const char* filename, int lineNum) { return BleachNewInternal::DebugAlloc(size, filename, lineNum); }
void operator delete(void* pMemory) { BleachNewInternal::DebugFree(pMemory); }
void operator delete(void* pMemory, const char*, int) { BleachNewInternal::DebugFree(pMemory); }

// array
void* operator new[](size_t size, const char* filename, int lineNum) { return BleachNewInternal::DebugAlloc(size, filename, lineNum); }
void operator delete[](void* pMemory) { BleachNewInternal::DebugFree(pMemory); }
void operator delete[](void* pMemory, const char*, int) { BleachNewInternal::DebugFree(pMemory); }

// memory tracking
#if ENABLE_BLEACH_ALLOCATION_TRACKING
    void* operator new(size_t size, const char* filename, int lineNum, uint64_t count) {return BleachNewInternal::DebugAlloc(size, filename, lineNum, count); }
    void* operator new[](size_t size, const char* filename, int lineNum, uint64_t count) { return BleachNewInternal::DebugAlloc(size, filename, lineNum, count); }
    void operator delete(void* pMemory, const char*, int, uint64_t) { BleachNewInternal::DebugFree(pMemory); }
    void operator delete[](void* pMemory, const char*, int, uint64_t) { BleachNewInternal::DebugFree(pMemory); }
#endif  // ENABLE_BLEACH_ALLOCATION_TRACKING

#endif  // USE_DEBUG_BLEACH_NEW
