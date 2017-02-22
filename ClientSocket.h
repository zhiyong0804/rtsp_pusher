/*
    File:       ClientSocket.h

    
    
*/


#ifndef __CLIENT_SOCKET__
#define __CLIENT_SOCKET__

#include "TCPSocket.h"
#include <stdint.h>
typedef int ET_Error;


class ClientSocket
{
    public:
    
        ClientSocket();
        virtual ~ClientSocket() {}
        
        void    Set(uint32_t hostAddr, uint16_t hostPort)
            { fHostAddr = hostAddr; fHostPort = hostPort; }
            
        //
        // Sends data to the server. If this returns EAGAIN or EINPROGRESS, call again
        // until it returns ET_NoErr or another error. On subsequent calls, you need not
        // provide a buffer.
        //
        // When this call returns EAGAIN or EINPROGRESS, caller should use GetEventMask
        // and GetSocket to wait for a socket event.
        ET_Error    Send(char* inData, const uint32_t inLength);

        //
        // Sends an ioVec to the server. Same conditions apply as above function 
        virtual ET_Error    SendV(iovec* inVec, uint32_t inNumVecs) = 0;
        
        //
        // Reads data from the server. If this returns EAGAIN or EINPROGRESS, call
        // again until it returns ET_NoErr or another error. This call may return ET_NoErr
        // and 0 for rcvLen, in which case you should call it again.
        //
        // When this call returns EAGAIN or EINPROGRESS, caller should use GetEventMask
        // and GetSocket to wait for a socket event.
        virtual ET_Error    Read(void* inBuffer, const uint32_t inLength, uint32_t* outRcvLen) = 0;
        
        //
        // ACCESSORS
        uint32_t          GetHostAddr()           { return fHostAddr; }
        virtual uint32_t  GetLocalAddr() = 0;

        // If one of the above methods returns EWOULDBLOCK or EINPROGRESS, you
        // can check this to see what events you should wait for on the socket
        uint32_t      GetEventMask()          { return fEventMask; }
        Socket*     GetSocket()             { return fSocketP; }
        
        virtual void    SetRcvSockBufSize(uint32_t inSize) = 0;

    protected:
    
        // Generic connect function
        ET_Error    Connect(TCPSocket* inSocket);   
        // Generic open function
        ET_Error    Open(TCPSocket* inSocket);
        
        ET_Error    SendSendBuffer(TCPSocket* inSocket);
    
        uint32_t      fHostAddr;
        uint16_t      fHostPort;
        
        uint32_t      fEventMask;
        Socket*     fSocketP;

        enum
        {
            kSendBufferLen = 2048
        };
        
        // Buffer for sends.
        char        fSendBuf[kSendBufferLen + 1];
        StrPtrLen   fSendBuffer;
        uint32_t      fSentLength;
};

class TCPClientSocket : public ClientSocket
{
    public:
        
        TCPClientSocket(uint32_t inSocketType);
        virtual ~TCPClientSocket() {}
        
        //
        // Implements the ClientSocket Send and Receive interface for a TCP connection
        virtual ET_Error    SendV(iovec* inVec, uint32_t inNumVecs);
        virtual ET_Error    Read(void* inBuffer, const uint32_t inLength, uint32_t* outRcvLen);

        virtual uint32_t  GetLocalAddr() { return fSocket.GetLocalAddr(); }
        virtual void    SetRcvSockBufSize(uint32_t inSize) { fSocket.SetSocketRcvBufSize(inSize); }
        virtual void    SetOptions(int sndBufSize = 8192,int rcvBufSize=1024);
        
        virtual uint16_t  GetLocalPort() { return fSocket.GetLocalPort(); }
        
    private:
    
        TCPSocket   fSocket;
};
#endif //__CLIENT_SOCKET__
