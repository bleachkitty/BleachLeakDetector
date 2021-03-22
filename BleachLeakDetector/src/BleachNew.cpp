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
// Functions for the core leak detector.
//---------------------------------------------------------------------------------------------------------------------
namespace BleachNewInternal
{
    void InitLeakDetector()
    {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#if ENABLE_BLEACH_ALLOCATION_TRACKING
        BleachNewInternal::InitMemoryTracker();
#endif
    }

    void DestroyLeakDetector()
    {
#if ENABLE_BLEACH_ALLOCATION_TRACKING
        BleachNewInternal::DumpAndDestroyMemoryTracker();
#endif
    }

    inline static void* DebugAlloc(size_t size, const char* filename, int lineNum)
    {
        return _malloc_dbg(size, 1, filename, lineNum);
    }

    inline static void DebugFree(void* pMemory)
    {
        _free_dbg(pMemory, 1);
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Memory tracking.  We maintain two hash maps, one for all the records keyed by address and one to keep track of 
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
    #include <unordered_map>
    #include <string>
    #include <mutex>
    #include <atomic>

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

    namespace BleachNewInternal
    {
        //-------------------------------------------------------------------------------------------------------------
        // Functions for interacting with the debugger, specifically being able to break into the debugger 
        // programmatically and writing strings to the output window in Visual Studio.  Note that this is all 
        // likely very specific Visual Studio, so you will need to redefine these functions for other compilers.
        //-------------------------------------------------------------------------------------------------------------
        #ifdef _MSC_VER
            static void BreakIntoDebugger()
            {
                if (::IsDebuggerPresent() == TRUE)
                    ::DebugBreak();
            }

            static void OutputStringToDebugger(const char* str)
            {
                OutputDebugStringA(str);
            }
        #else  // non-Visual Studio
            static void BreakIntoDebugger() { }  // Potentially do __asm int 3?  That *should* work on most PCs.
            static void OutputStringToDebugger(const char*) { }  // Maybe print to std::cerr by default?
        #endif

        //-------------------------------------------------------------------------------------------------------------
        // The memory tracker.
        //-------------------------------------------------------------------------------------------------------------
        class MemoryTracker
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
                std::string filename;
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

            std::unordered_map<uint32_t, CountRecord> m_counts;  // memory hash => CountRecord
            std::unordered_map<size_t, MemoryRecord> m_records;  // pointer => MemoryRecord
            std::recursive_mutex m_mutex;
            std::atomic_bool m_destroying;

        public:
            MemoryTracker()
                : m_destroying(false)
            {
                // find() crashes if the bucket array has a zero length, so this gets around that
                m_counts.reserve(4);
                m_records.reserve(4);
            }

            ~MemoryTracker()
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
                        BreakIntoDebugger();
                    }
                    m_records.emplace(reinterpret_cast<size_t>(pPtr), MemoryRecord{ allocHash, pPtr, countRecord.count });
                }
                else
                {
                    if (breakPoint == 1)
                    {
                        BreakIntoDebugger();
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

            void DumpTrackerMemoryRecords()
            {
                if (m_destroying)
                    return;

                std::lock_guard<std::recursive_mutex> lock(m_mutex);

                OutputStringToDebugger("========================================\n");
                OutputStringToDebugger("Remaining Allocations:\n");

                std::string line;
                uint64_t count = 0;
                for (const auto& addressRecordPair : m_records)
                {
                    const auto& address = addressRecordPair.first;
                    const auto& record = addressRecordPair.second;

                    line = std::to_string(count) + "> ";

                    auto findIt = m_counts.find(record.allocLocationHash);
                    if (findIt != m_counts.end())
                        line += findIt->second.filename + "(" + std::to_string(findIt->second.line) + ")\n";
                    line += "    => [0x" + ToHexStr(address) + "] ID: " + std::to_string(record.id) + "\n";

                    OutputStringToDebugger(line.c_str());

                    ++count;
                }
                OutputStringToDebugger("========================================\n");
            }

        private:
            static uint32_t HashString32(const char* str)
            {
                static std::hash<const char*> hashFunc;
                return static_cast<uint32_t>(hashFunc(str));
            }

            static std::string ToHexStr(size_t val)
            {
                static constexpr unsigned int kMaxDigitsInInt64Base2 = (sizeof(unsigned long long) * 8) + 1;  // +1 for '\0'
                char str[kMaxDigitsInInt64Base2];
                memset(str, 0, kMaxDigitsInInt64Base2);
                _ui64toa_s(val, str, kMaxDigitsInInt64Base2, 16);
                return str;
            }

            static uint32_t HashMemoryEntry(const char* filename, int lineNum)
            {
                uint32_t allocHash = HashString32(filename);
                allocHash ^= lineNum;
                return allocHash;
            }
        };

        MemoryTracker* g_pMemoryDebugger = nullptr;

        void InitMemoryTracker()
        {
            if (!g_pMemoryDebugger)
                g_pMemoryDebugger = new MemoryTracker;  // purposefully not using the overloaded version of new
        }

        void DumpAndDestroyMemoryTracker()
        {
            if (g_pMemoryDebugger)
            {
                DumpTrackerMemoryRecords();
                delete g_pMemoryDebugger;  // purposefully not using the overloaded version of delete
                g_pMemoryDebugger = nullptr;
            }
        }

        void AddRecord(void* pPtr, const char* filename, int lineNum, uint64_t breakPoint = 0)
        {
            if (g_pMemoryDebugger)
                g_pMemoryDebugger->AddRecord(pPtr, filename, lineNum, breakPoint);
        }

        void RemoveRecord(void* pPtr)
        {
            if (g_pMemoryDebugger)
                g_pMemoryDebugger->RemoveRecord(pPtr);
        }

        void DumpTrackerMemoryRecords()
        {
            if (g_pMemoryDebugger)
                g_pMemoryDebugger->DumpTrackerMemoryRecords();
        }
    }
#endif  // ENABLE_BLEACH_ALLOCATION_TRACKING

//---------------------------------------------------------------------------------------------------------------------
// Debug new/delete overloads
//---------------------------------------------------------------------------------------------------------------------
void* operator new(size_t size, const char* filename, int lineNum)
{
#if ENABLE_BLEACH_ALLOCATION_TRACKING
    void* pPtr = BleachNewInternal::DebugAlloc(size, filename, lineNum);
    BleachNewInternal::AddRecord(pPtr, filename, lineNum);
    return pPtr;
#else
    return BleachNewInternal::DebugAlloc(size, filename, lineNum);
#endif
}

void operator delete(void* pMemory)
{
#if ENABLE_BLEACH_ALLOCATION_TRACKING
    BleachNewInternal::RemoveRecord(pMemory);
#endif
    BleachNewInternal::DebugFree(pMemory);
}

void* operator new[](size_t size, const char* filename, int lineNum)
{
#if ENABLE_BLEACH_ALLOCATION_TRACKING
    void* pPtr = BleachNewInternal::DebugAlloc(size, filename, lineNum);
    BleachNewInternal::AddRecord(pPtr, filename, lineNum);
    return pPtr;
#else
    return BleachNewInternal::DebugAlloc(size, filename, lineNum);
#endif
}

void operator delete[](void* pMemory)
{
#if ENABLE_BLEACH_ALLOCATION_TRACKING
    BleachNewInternal::RemoveRecord(pMemory);
#endif
    BleachNewInternal::DebugFree(pMemory);
}

#if ENABLE_BLEACH_ALLOCATION_TRACKING
    void* operator new(size_t size, const char* filename, int lineNum, uint64_t count)
    {
        void* pPtr = BleachNewInternal::DebugAlloc(size, filename, lineNum);
        BleachNewInternal::AddRecord(pPtr, filename, lineNum, count);
        return pPtr;
    }

    void* operator new[](size_t size, const char* filename, int lineNum, uint64_t count)
    {
        void* pPtr = BleachNewInternal::DebugAlloc(size, filename, lineNum);
        BleachNewInternal::AddRecord(pPtr, filename, lineNum, count);
        return pPtr;
    }

#endif  // ENABLE_BLEACH_ALLOCATION_TRACKING

#endif  // USE_DEBUG_BLEACH_NEW
