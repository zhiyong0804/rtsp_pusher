/*
    File:       OSMemory_Server.cpp

    Contains:   Implementation of OSMemory stuff, including all new & delete
                operators.
                    

*/

#include <string.h>
#include "memory.h"

#ifdef __APPLE__
//	#include <sys/malloc.h>
//	#include <stdlib.h>
#else
	#include <malloc.h>
#endif

#include <stdlib.h>
static int   sMemoryErr = 0;


//
// OPERATORS
#ifdef __MacOSX__
void* operator new (size_t s) throw(std::bad_alloc)
#else
void* operator new (size_t s)
#endif
{
    return OSMemory::New(s);
}

#ifdef __MacOSX__
void* operator new[](size_t s) throw(std::bad_alloc)
#else
void* operator new[](size_t s)
#endif
{
    return OSMemory::New(s);
}

#ifdef __MacOSX__
void operator delete(void* mem) throw()
#else
void operator delete(void* mem)
#endif
{
    OSMemory::Delete(mem);
}

#ifdef __MacOSX__
void operator delete[](void* mem) throw()
#else
void operator delete[](void* mem)
#endif
{
    OSMemory::Delete(mem);
}





void OSMemory::SetMemoryError(int inErr)
{
    sMemoryErr = inErr;
}

void*   OSMemory::New(size_t inSize)
{
    void *m = ::malloc(inSize);
    return m;
}

void    OSMemory::Delete(void* inMemory)
{
    if (inMemory == NULL)
        return;
    ::free(inMemory);
}

