/*
    File:       OSArrayObjectDeleter.h

    Contains:   Auto object for deleting arrays.



*/

#ifndef __OS_ARRAY_OBJECT_DELETER_H__
#define __OS_ARRAY_OBJECT_DELETER_H__

#include <assert.h>

template <class T>
class OSArrayObjectDeleter
{
    public:
		OSArrayObjectDeleter() : fT(NULL) {}
        OSArrayObjectDeleter(T* victim) : fT(victim)  {}
        ~OSArrayObjectDeleter() { delete [] fT; }
        
        void ClearObject() { fT = NULL; }

        void SetObject(T* victim) 
        {
            assert(fT == NULL);
            fT = victim; 
        }
        T* GetObject() { return fT; }
        
        operator T*() { return fT; }
    
    private:
    
        T* fT;
};


template <class T>
class OSPtrDeleter
{
    public:
		OSPtrDeleter() : fT(NULL) {}
        OSPtrDeleter(T* victim) : fT(victim)  {}
        ~OSPtrDeleter() { delete fT; }
        
        void ClearObject() { fT = NULL; }

        void SetObject(T* victim) 
        {   assert(fT == NULL);
            fT = victim; 
        }
            
    private:
    
        T* fT;
};


typedef OSArrayObjectDeleter<char*> OSCharPointerArrayDeleter;
typedef OSArrayObjectDeleter<char> OSCharArrayDeleter;

#endif //__OS_OBJECT_ARRAY_DELETER_H__
