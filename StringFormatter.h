/*
    File:       StringFormatter.h

    Contains:   Utility class for formatting text to a buffer.
                Construct object with a buffer, then call one
                of many Put methods to write into that buffer.
                    
    

*/

#ifndef __STRINGFORMATTER_H__
#define __STRINGFORMATTER_H__

#include <string.h>
#include "StrPtrLen.h"
#include <assert.h>
#include <stdint.h>


//Use a class like the ResizeableStringFormatter if you want a buffer that will dynamically grow
class StringFormatter
{
    public:
        
        //pass in a buffer and length for writing
        StringFormatter(char *buffer, uint32_t length) :  fCurrentPut(buffer), 
                                                        fStartPut(buffer),
                                                        fEndPut(buffer + length),
                                                        fBytesWritten(0) {}

        StringFormatter(StrPtrLen &buffer) :            fCurrentPut(buffer.Ptr),
                                                        fStartPut(buffer.Ptr),
                                                        fEndPut(buffer.Ptr + buffer.Len),
                                                        fBytesWritten(0) {}
        virtual ~StringFormatter() {}
        
        void Set(char *buffer, uint32_t length)   {   fCurrentPut = buffer; 
                                                    fStartPut = buffer;
                                                    fEndPut = buffer + length;
                                                    fBytesWritten= 0;
                                                }
                                            
        //"erases" all data in the output stream save this number
        void        Reset(uint32_t inNumBytesToLeave = 0)
            { fCurrentPut = fStartPut + inNumBytesToLeave; }

        //Object does no bounds checking on the buffer. That is your responsibility!
        //Put truncates to the buffer size
        void        Put(const int32_t num);
        void        Put(const char* buffer, uint32_t bufferSize);
        void        Put(const char* str)      { Put(str, strlen(str)); }
        void        Put(const StrPtrLen &str) { Put(str.Ptr, str.Len); }
        void        PutSpace()          { PutChar(' '); }
        void        PutEOL()            {  Put(sEOL, sEOLLen); }
        void        PutChar(char c)     { Put(&c, 1); }
        void        PutTerminator()     { PutChar('\0'); }
		
		//Writes a printf style formatted string
		bool		PutFmtStr(const char *fmt,  ...);

            
		//the number of characters in the buffer
        inline uint32_t       GetCurrentOffset();
        inline uint32_t       GetSpaceLeft();
        inline uint32_t       GetTotalBufferSize();
        char*               GetCurrentPtr()     { return fCurrentPut; }
        char*               GetBufPtr()         { return fStartPut; }
        
        // Counts total bytes that have been written to this buffer (increments
        // even when the buffer gets reset)
        void                ResetBytesWritten() { fBytesWritten = 0; }
        uint32_t              GetBytesWritten()   { return fBytesWritten; }
        
        inline void         PutFilePath(StrPtrLen *inPath, StrPtrLen *inFileName);
        inline void         PutFilePath(char *inPath, char *inFileName);
		
		//Return a NEW'd copy of the buffer as a C string
		char *GetAsCString()
		{
			StrPtrLen str(fStartPut, this->GetCurrentOffset());
			return str.GetAsCString();
		}

    protected:

        //If you fill up the StringFormatter buffer, this function will get called. By
        //default, the function simply returns false.  But derived objects can clear out the data,
		//reset the buffer, and then returns true.
        //Use the ResizeableStringFormatter if you want a buffer that will dynamically grow.
		//Returns true if the buffer has been resized.
        virtual bool    BufferIsFull(char* /*inBuffer*/, uint32_t /*inBufferLen*/) { return false; }

        char*       fCurrentPut;
        char*       fStartPut;
        char*       fEndPut;
        
        // A way of keeping count of how many bytes have been written total
        uint32_t fBytesWritten;

        static const char*    sEOL;
        static const uint32_t   sEOLLen;
        
        static const char* kPathDelimiterString;                                             
        static const char kPathDelimiterChar;                                                  
        static const int kPartialPathBeginsWithDelimiter; 
};

inline uint32_t StringFormatter::GetCurrentOffset()
{
    assert(fCurrentPut >= fStartPut);
    return (uint32_t)(fCurrentPut - fStartPut);
}

inline uint32_t StringFormatter::GetSpaceLeft()
{
    assert(fEndPut >= fCurrentPut);
    return (uint32_t)(fEndPut - fCurrentPut);
}

inline uint32_t StringFormatter::GetTotalBufferSize()
{
    assert(fEndPut >= fStartPut);
    return (uint32_t)(fEndPut - fStartPut);
}

inline void StringFormatter::PutFilePath(StrPtrLen *inPath, StrPtrLen *inFileName)
{
   if (inPath != NULL && inPath->Len > 0)
    {   
        Put(inPath->Ptr, inPath->Len);
        if (kPathDelimiterChar != inPath->Ptr[inPath->Len -1] )
            Put(kPathDelimiterString);
    }
    if (inFileName != NULL && inFileName->Len > 0)
        Put(inFileName->Ptr, inFileName->Len);
}

inline void StringFormatter::PutFilePath(char *inPath, char *inFileName)
{
   StrPtrLen pathStr(inPath);
   StrPtrLen fileStr(inFileName);
   
   PutFilePath(&pathStr,&fileStr);
}

#endif // __STRINGFORMATTER_H__

