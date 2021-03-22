// Example.cpp
#include "BleachNew.h"

int main()
{
    // Initialize the leak detector.
    INIT_LEAK_DETECTOR();

    // Basic test.  You should get the following message in the debug output window:
    //      <path>\Example.cpp(12) : {161} normal block at 0x000001969397CB30, 32 bytes long.
    //      Data : <                > CD CD CD CD CD CD CD CD CD CD CD CD CD CD CD CD
    char* pBuffer = BLEACH_NEW_ARRAY(char, 32);

    // Allocate an array of five ints and delete all but the middle one.  You should get a message saying that 
    // ID 3 leaked.  ID's start at 1, so this is the 3rd allocation, which means it's index #2.  Also note the 
    // data is 04 00 00 00, which is correct (2 * 2 = 4).
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

    // Destroy the leak detector.  This dumps all memory leaks.
    DESTROY_LEAK_DETECTOR();

    return 0;
}
