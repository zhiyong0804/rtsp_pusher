/*
    File:       TCPSocket.h

    Contains:   TCP socket object
                    
    
    
    
*/

#ifndef __TCPSOCKET_H__
#define __TCPSOCKET_H__

#include <stdio.h>
#include <stdlib.h>
#ifndef __Win32__
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include "Socket.h"
#include "StrPtrLen.h"

class TCPSocket : public Socket
{
    public:

        //TCPSocket takes an optional task object which will get notified when
        //certain events happen on this socket. Those events are:
        //
        //S_DATA:               Data is currently available on the socket.
        //S_CONNECTIONCLOSING:  Client is closing the connection. No longer necessary
        //                      to call Close or Disconnect, Snd & Rcv will fail.
        TCPSocket(uint32_t inSocketType)
            :   Socket(inSocketType),
                fRemoteStr(fRemoteBuffer, kIPAddrBufSize)  {}
        virtual ~TCPSocket() {}

        //Open
        ET_Error    Open() { return Socket::Open(SOCK_STREAM); }

        // Connect. Attempts to connect to the specified remote host. If this
        // is a non-blocking socket, this function may return EINPROGRESS, in which
        // case caller must wait for either an EV_RE or an EV_WR. You may call
        // CheckAsyncConnect at any time, which will return ET_NoErr if the connect
        // has completed, EINPROGRESS if it is still in progress, or an appropriate error
        // if the connect failed.
        ET_Error    Connect(uint32_t inRemoteAddr, uint16_t inRemotePort);
        //ET_Error  CheckAsyncConnect();

        //ACCESSORS:
        //Returns NULL if not currently available.
        
        uint32_t      GetRemoteAddr() { return ntohl(fRemoteAddr.sin_addr.s_addr); }
        uint16_t      GetRemotePort() { return ntohs(fRemoteAddr.sin_port); }
        //This function is NOT thread safe!
        StrPtrLen*  GetRemoteAddrStr();

    protected:

        void        Set(int inSocket, struct sockaddr_in* remoteaddr);
                            
        enum
        {
            kIPAddrBufSize = 20 //uint32_t
        };

        struct sockaddr_in  fRemoteAddr;
        char fRemoteBuffer[kIPAddrBufSize];
        StrPtrLen fRemoteStr;

        
        friend class TCPListenerSocket;
};
#endif // __TCPSOCKET_H__

