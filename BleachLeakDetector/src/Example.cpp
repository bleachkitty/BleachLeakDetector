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
#include <cstring>

class Foo
{
    static constexpr size_t kBufferSize = 4;

    char* m_pBuffer;

public:
    Foo()
        : m_pBuffer(BLEACH_NEW_ARRAY(char, kBufferSize))
    {
        std::memset(m_pBuffer, 0, kBufferSize);
        m_pBuffer[0] = 'X';
    }

    ~Foo()
    {
        BLEACH_DELETE_ARRAY(m_pBuffer);
        m_pBuffer = nullptr;
    }
};

int main()
{
    // Initialize the leak detector.
    BLEACH_INIT_LEAK_DETECTOR();

    // Basic test.  You should get the following message in the debug output window:
    //      <path>\Example.cpp(35) : {161} normal block at 0x000001969397CB30, 32 bytes long.
    //      Data : <                > CD CD CD CD CD CD CD CD CD CD CD CD CD CD CD CD
    char* pBuffer = BLEACH_NEW_ARRAY(char, 32);

    // Allocate an array of five ints and delete all but the middle one.  If you set ENABLE_BLEACH_ALLOCATION_TRACKING 
    // to 1 (it's 0 by default), you should get a message saying that ID 3 leaked.  ID's start at 1, so this is the 
    // 3rd allocation, which means it's index #2.  Also note the data is 04 00 00 00, which is correct (2 * 2 = 4).
    // If ENABLE_BLEACH_ALLOCATION_TRACKING is set to 0, you will just get the standard leak message above.
    int* pValues[5];
    for (size_t i = 0; i < 5; ++i)
    {
        // Swap these two calls to BLEACH_NEW to break on the specific allocation that leaks.  Note that the 3 
        // we're passing in here is because ID 3 has leaked.  When you break, note that i == 2, which is the 3rd 
        // element.
        pValues[i] = BLEACH_NEW(int);
        //pValues[i] = BLEACH_NEW_BREAK(int, 3);
        *pValues[i] = static_cast<int>(i * i);
    }
    BLEACH_DELETE(pValues[0]);
    BLEACH_DELETE(pValues[1]);
    BLEACH_DELETE(pValues[3]);
    BLEACH_DELETE(pValues[4]);

    // Using a custom class.
    Foo* pFoo = BLEACH_NEW(Foo);
    BLEACH_DELETE(pFoo);

    // Destroy the leak detector.  This dumps all memory leaks.
    BLEACH_DUMP_AND_DESTROY_LEAK_DETECTOR();

    return 0;
}
