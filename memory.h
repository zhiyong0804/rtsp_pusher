#ifndef __OS_MEMORY_H__
#define __OS_MEMORY_H__

class OSMemory
{
    public:
        // Provides non-debugging behaviour for new and delete
        static void*    New(size_t inSize);
        static void     Delete(void* inMemory);
        
        //When memory allocation fails, the server just exits. This sets the code
        //the server exits with
        static void SetMemoryError(int inErr);
        
};


// NEW MACRO
// When memory debugging is on, this macro transparently uses the memory debugging
// overridden version of the new operator. When memory debugging is off, it just compiles
// down to the standard new.

#ifdef  NEW
#error Conflicting Macro "NEW"
#endif

#define NEW new
#include <new>
//inline void* operator new(size_t, void* ptr) { return ptr;}
#endif //__OS_MEMORY_H__
