/*
    File:       StringParser.h

    Contains:   A couple of handy utilities for parsing a stream.  
                    
    

*/


#ifndef __STRINGPARSER_H__
#define __STRINGPARSER_H__

#include "StrPtrLen.h"
#include <assert.h>
#include <stdint.h>

#define STRINGPARSERTESTING 0


class StringParser
{
    public:
        
        StringParser(StrPtrLen *inStream)
            :   fStartGet(inStream == NULL ? NULL : inStream->Ptr),
                fEndGet(inStream == NULL ? NULL : inStream->Ptr + inStream->Len),
                fCurLineNumber(1),
                fStream(inStream) {}
        ~StringParser() {}
        
        // Built-in masks for common stop conditions
        static uint8_t sDigitMask[];      // stop when you hit a digit
        static uint8_t sWordMask[];       // stop when you hit a word
        static uint8_t sEOLMask[];        // stop when you hit an eol
        static uint8_t sEOLWhitespaceMask[]; // stop when you hit an EOL or whitespace
        static uint8_t sEOLWhitespaceQueryMask[]; // stop when you hit an EOL, ? or whitespace
       
        static uint8_t sWhitespaceMask[]; // skip over whitespace
        

        //GetBuffer:
        //Returns a pointer to the string object
        StrPtrLen*      GetStream() { return fStream; }
        
        //Expect:
        //These functions consume the given token/word if it is in the stream.
        //If not, they return false.
        //In all other situations, true is returned.
        //NOTE: if these functions return an error, the object goes into a state where
        //it cannot be guarenteed to function correctly.
        bool          Expect(char stopChar);
        bool          ExpectEOL();
        
        //Returns the next word
        void            ConsumeWord(StrPtrLen* outString = NULL)
                            { ConsumeUntil(outString, sNonWordMask); }

        //Returns all the data before inStopChar
        void            ConsumeUntil(StrPtrLen* outString, char inStopChar);

        //Returns whatever integer is currently in the stream
        uint32_t          ConsumeInteger(StrPtrLen* outString = NULL);
        float         ConsumeFloat();
        float         ConsumeNPT();
        
        //Keeps on going until non-whitespace
        void            ConsumeWhitespace()
                            { ConsumeUntil(NULL, sWhitespaceMask); }
        
        //Assumes 'stop' is a 255-char array of booleans. Set this array
        //to a mask of what the stop characters are. true means stop character.
        //You may also pass in one of the many prepackaged masks defined above.
        void            ConsumeUntil(StrPtrLen* spl, uint8_t *stop);


        //+ rt 8.19.99
        //returns whatever is avaliable until non-whitespace
        void            ConsumeUntilWhitespace(StrPtrLen* spl = NULL)
                            { ConsumeUntil( spl, sEOLWhitespaceMask); }

        void            ConsumeUntilDigit(StrPtrLen* spl = NULL)
                            { ConsumeUntil( spl, sDigitMask); }

		void			ConsumeLength(StrPtrLen* spl, int32_t numBytes);

		void			ConsumeEOL(StrPtrLen* outString);

        //GetThru:
        //Works very similar to ConsumeUntil except that it moves past the stop token,
        //and if it can't find the stop token it returns false
        inline bool       GetThru(StrPtrLen* spl, char stop);
        inline bool       GetThruEOL(StrPtrLen* spl);
        inline bool       ParserIsEmpty(StrPtrLen* outString);
        //Returns the current character, doesn't move past it.
        inline char     PeekFast() { if (fStartGet) return *fStartGet; else return '\0'; }
        char operator[](int i) { assert((fStartGet+i) < fEndGet);return fStartGet[i]; }
        
        //Returns some info about the stream
        uint32_t          GetDataParsedLen() 
            { assert(fStartGet >= fStream->Ptr); return (uint32_t)(fStartGet - fStream->Ptr); }
        uint32_t          GetDataReceivedLen()
            { assert(fEndGet >= fStream->Ptr); return (uint32_t)(fEndGet - fStream->Ptr); }
        uint32_t          GetDataRemaining()
            { assert(fEndGet >= fStartGet); return (uint32_t)(fEndGet - fStartGet); }
        char*           GetCurrentPosition() { return fStartGet; }
        int         GetCurrentLineNumber() { return fCurLineNumber; }
        
        // A utility for extracting quotes from the start and end of a parsed
        // string. (Warning: Do not call this method if you allocated your own  
        // pointer for the Ptr field of the StrPtrLen class.) - [sfu]
        // 
        // Not sure why this utility is here and not in the StrPtrLen class - [jm]
        static void UnQuote(StrPtrLen* outString);


#if STRINGPARSERTESTING
        static bool       Test();
#endif

    private:

        void        AdvanceMark();
        
        //built in masks for some common stop conditions
        static uint8_t sNonWordMask[];

        char*       fStartGet;
        char*       fEndGet;
        int         fCurLineNumber;
        StrPtrLen*  fStream;
        
};


bool StringParser::GetThru(StrPtrLen* outString, char inStopChar)
{
    ConsumeUntil(outString, inStopChar);
    return Expect(inStopChar);
}

bool StringParser::GetThruEOL(StrPtrLen* outString)
{
    ConsumeUntil(outString, sEOLMask);
    return ExpectEOL();
}

bool StringParser::ParserIsEmpty(StrPtrLen* outString)
{
    if (NULL == fStartGet || NULL == fEndGet)
    {
        if (NULL != outString)
        {   outString->Ptr = NULL;
            outString->Len = 0;
        }
        
        return true;
    }
    
    assert(fStartGet <= fEndGet);
    
    return false; // parser ok to parse
}


#endif // __STRINGPARSER_H__
