/*
    File:       TCPSocket.cpp

    Contains:   implements TCPSocket class
                    
    
    
*/

#include "common.h"
#include "TCPSocket.h"

#ifdef USE_NETLOG
#include <netlog.h>
#endif

void TCPSocket::Set(int inSocket, struct sockaddr_in* remoteaddr)
{
    fRemoteAddr = *remoteaddr;
    fFileDesc = inSocket;
    
    if ( inSocket != -1 ) 
    {
        //make sure to find out what IP address this connection is actually occuring on. That
        //way, we can report correct information to clients asking what the connection's IP is
#if __Win32__ || __osf__ || __sgi__ || __hpux__	
        int len = sizeof(fLocalAddr);
#else
        socklen_t len = sizeof(fLocalAddr);
#endif
        ::getsockname(fFileDesc, (struct sockaddr*)&fLocalAddr, &len);
        fState |= kBound;
        fState |= kConnected;
    }
    else
        fState = 0;
}

StrPtrLen*  TCPSocket::GetRemoteAddrStr()
{
    return &fRemoteStr;
}

ET_Error  TCPSocket::Connect(uint32_t inRemoteAddr, uint16_t inRemotePort)
{
    ::memset(&fRemoteAddr, 0, sizeof(fRemoteAddr));
    fRemoteAddr.sin_family = AF_INET;        /* host byte order */
    fRemoteAddr.sin_port = htons(inRemotePort); /* short, network byte order */
    fRemoteAddr.sin_addr.s_addr = htonl(inRemoteAddr);

    /* don't forget to error check the connect()! */
	int err = ::connect(fFileDesc, (sockaddr *)&fRemoteAddr, sizeof(fRemoteAddr));
    fState |= kConnected;
    
    if (err == -1)
    {
        fRemoteAddr.sin_port = 0;
        fRemoteAddr.sin_addr.s_addr = 0;
        return (ET_Error)errno;
    }
    
    return ET_NoErr;

}

