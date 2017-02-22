/*
    File:       StrPtrLen.h

    Contains:   Definition of class that tracks a string ptr and a length.
                Note: this is NOT a string container class! It is a string PTR container
                class. It therefore does not copy the string and store it internally. If
                you deallocate the string to which this object points to, and continue 
                to use it, you will be in deep doo-doo.
                
                It is also non-encapsulating, basically a struct with some simple methods.
                    

*/

#ifndef __STRPTRLEN_H__
#define __STRPTRLEN_H__

#include <string.h>
#include <ctype.h> 
#include <stdint.h>
#include <assert.h>

#define STRPTRLENTESTING 0

class StrPtrLen
{
    public:

        //CONSTRUCTORS/DESTRUCTOR
        //These are so tiny they can all be inlined
        StrPtrLen() : Ptr(NULL), Len(0) {}
        StrPtrLen(char* sp) : Ptr(sp), Len(sp != NULL ? strlen(sp) : 0) {}
        StrPtrLen(char *sp, uint32_t len) : Ptr(sp), Len(len) {}
        virtual ~StrPtrLen() {}
        
        //OPERATORS:
        bool Equal(const StrPtrLen &compare) const;
        bool EqualIgnoreCase(const char* compare, const uint32_t len) const;
        bool EqualIgnoreCase(const StrPtrLen &compare) const { return EqualIgnoreCase(compare.Ptr, compare.Len); }
        bool Equal(const char* compare) const;
        bool NumEqualIgnoreCase(const char* compare, const uint32_t len) const;
        
        void Delete() { delete [] Ptr; Ptr = NULL; Len = 0; }
        char *ToUpper() { for (uint32_t x = 0; x < Len ; x++) Ptr[x] = toupper (Ptr[x]); return Ptr;}
        
        char *FindStringCase(char *queryCharStr, StrPtrLen *resultStr, bool caseSensitive) const;

        char *FindString(StrPtrLen *queryStr, StrPtrLen *outResultStr)              {   assert(queryStr != NULL);   assert(queryStr->Ptr != NULL); assert(0 == queryStr->Ptr[queryStr->Len]);
                                                                                        return FindStringCase(queryStr->Ptr, outResultStr,true);    
                                                                                    }
        
        char *FindStringIgnoreCase(StrPtrLen *queryStr, StrPtrLen *outResultStr)    {   assert(queryStr != NULL);   assert(queryStr->Ptr != NULL); assert(0 == queryStr->Ptr[queryStr->Len]); 
                                                                                        return FindStringCase(queryStr->Ptr, outResultStr,false); 
                                                                                    }

        char *FindString(StrPtrLen *queryStr)                                       {   assert(queryStr != NULL);   assert(queryStr->Ptr != NULL); assert(0 == queryStr->Ptr[queryStr->Len]); 
                                                                                        return FindStringCase(queryStr->Ptr, NULL,true);    
                                                                                    }
        
        char *FindStringIgnoreCase(StrPtrLen *queryStr)                             {   assert(queryStr != NULL);   assert(queryStr->Ptr != NULL); assert(0 == queryStr->Ptr[queryStr->Len]); 
                                                                                        return FindStringCase(queryStr->Ptr, NULL,false); 
                                                                                    }
                                                                                    
        char *FindString(char *queryCharStr)                                        { return FindStringCase(queryCharStr, NULL,true);   }
        char *FindStringIgnoreCase(char *queryCharStr)                              { return FindStringCase(queryCharStr, NULL,false);  }
        char *FindString(char *queryCharStr, StrPtrLen *outResultStr)               { return FindStringCase(queryCharStr, outResultStr,true);   }
        char *FindStringIgnoreCase(char *queryCharStr, StrPtrLen *outResultStr)     { return FindStringCase(queryCharStr, outResultStr,false);  }

        char *FindString(StrPtrLen &query, StrPtrLen *outResultStr)                 { return FindString( &query, outResultStr);             }
        char *FindStringIgnoreCase(StrPtrLen &query, StrPtrLen *outResultStr)       { return FindStringIgnoreCase( &query, outResultStr);   }
        char *FindString(StrPtrLen &query)                                          { return FindString( &query);           }
        char *FindStringIgnoreCase(StrPtrLen &query)                                { return FindStringIgnoreCase( &query); }
        
        StrPtrLen& operator=(const StrPtrLen& newStr) { Ptr = newStr.Ptr; Len = newStr.Len;
                                                        return *this; }
        char operator[](int i) { /*assert(i<Len);i*/ return Ptr[i]; }
        void Set(char* inPtr, uint32_t inLen) { Ptr = inPtr; Len = inLen; }
        void Set(char* inPtr) { Ptr = inPtr; Len = (inPtr) ?  ::strlen(inPtr) : 0; }

        //This is a non-encapsulating interface. The class allows you to access its
        //data.
        char*       Ptr;
        uint32_t      Len;

        // convert to a "NEW'd" zero terminated char array
        char*   GetAsCString() const;
        void    PrintStr();
        void    PrintStr(char *appendStr);
        void    PrintStr(char* prependStr, char *appendStr);

        void    PrintStrEOL(char* stopStr = NULL, char *appendStr = NULL); //replace chars x0A and x0D with \r and \n
 
        //Utility function
        uint32_t    TrimTrailingWhitespace();
        uint32_t    TrimLeadingWhitespace();
   
        uint32_t  RemoveWhitespace();
        void  TrimWhitespace() { TrimLeadingWhitespace(); TrimTrailingWhitespace(); }

#if STRPTRLENTESTING
        static bool   Test();
#endif

    private:

        static uint8_t    sCaseInsensitiveMask[];
        static uint8_t    sNonPrintChars[];
};



class StrPtrLenDel : public StrPtrLen
{
  public:
     StrPtrLenDel() : StrPtrLen() {}
     StrPtrLenDel(char* sp) : StrPtrLen(sp) {}
     StrPtrLenDel(char *sp, uint32_t len) : StrPtrLen(sp,len) {}
     ~StrPtrLenDel() { Delete(); }
};

#endif // __STRPTRLEN_H__
