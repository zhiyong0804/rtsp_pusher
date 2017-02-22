/*
    File:       StringFormatter.cpp

    Contains:   Implementation of StringFormatter class.  
                    
    
    
    
*/

#include <string.h>
#include <stdarg.h>
#include "StringFormatter.h"
#include <assert.h>
#include <stdio.h>


const char*   StringFormatter::sEOL = "\r\n";
const uint32_t  StringFormatter::sEOLLen = 2;

#ifdef __MacOSX__                                                                       
        const char* kPathDelimiterString = ":";                                                
        const char kPathDelimiterChar = ':';                                                  
        const int kPartialPathBeginsWithDelimiter = 1;                                       
#else                                                                               
        const char* kPathDelimiterString = "/";                                                
        const char kPathDelimiterChar = '/';
        const int kPartialPathBeginsWithDelimiter = 0;
#endif

void StringFormatter::Put(const int32_t num)
{
    char buff[32];
    sprintf(buff, "%d", num);
    Put(buff);
}

void StringFormatter::Put(const char* buffer, uint32_t bufferSize)
{
	//optimization for writing 1 character
    if((bufferSize == 1) && (fCurrentPut != fEndPut)) {
        *(fCurrentPut++) = *buffer;
        fBytesWritten++;
        return;
    }       
        
    //loop until the input buffer size is smaller than the space in the output
    //buffer. Call BufferIsFull at each pass through the loop
    uint32_t spaceLeft = this->GetSpaceLeft();
    uint32_t spaceInBuffer =  spaceLeft - 1;
    uint32_t resizedSpaceLeft = 0;
    
    while ( (spaceInBuffer < bufferSize) || (spaceLeft == 0) ) // too big for destination
    {
        if (spaceLeft > 0)
        {
			//copy as much as possible; truncating the result
            ::memcpy(fCurrentPut, buffer, spaceInBuffer);
            fCurrentPut += spaceInBuffer;
            fBytesWritten += spaceInBuffer;
            buffer += spaceInBuffer;
            bufferSize -= spaceInBuffer;
        }
        this->BufferIsFull(fStartPut, this->GetCurrentOffset()); // resize buffer
        resizedSpaceLeft = this->GetSpaceLeft();
        if (spaceLeft == resizedSpaceLeft) // couldn't resize, nothing left to do
        {  
           return; // done. There is either nothing to do or nothing we can do because the BufferIsFull
        }
        spaceLeft = resizedSpaceLeft;
        spaceInBuffer =  spaceLeft - 1;
    }
    
    //copy the remaining chunk into the buffer
    ::memcpy(fCurrentPut, buffer, bufferSize);
    fCurrentPut += bufferSize;
    fBytesWritten += bufferSize;
    
}

//Puts a printf-style formatted string; except that the NUL terminator is not written.  If the buffer is too small, returns false and does not
//Alter the buffer.  Will not count the '\0' terminator as among the bytes written
bool StringFormatter::PutFmtStr(const char *fmt,  ...)
{
	assert(fmt != NULL);

    va_list args;
	for(;;)
	{
		va_start(args,fmt);
		int length = ::vsnprintf(fCurrentPut, this->GetSpaceLeft(), fmt, args);
		va_end(args);
		
		if (length < 0)
			return false;
		if (static_cast<uint32_t>(length) >= this->GetSpaceLeft()) //was not able to write all the output
		{
			if (this->BufferIsFull(fStartPut, this->GetCurrentOffset()))
				continue;
			//can only output a portion of the string
			uint32_t bytesWritten = fEndPut - fCurrentPut - 1; //We don't want to include the NUL terminator
			fBytesWritten += bytesWritten;
			fCurrentPut += bytesWritten;
			return false;
		}
		else
		{
			fBytesWritten += length;
			fCurrentPut += length;
		}
		return true;
	}
}

