/*
    File:       Socket.cpp

    Contains:   implements Socket class
                    

    
*/

#include "common.h"
#include <string.h>
#include <stdio.h>

#include "Socket.h"
#include<netinet/tcp.h>
#include <sys/uio.h>

#ifdef USE_NETLOG
	#include <netlog.h>
#else
	#if defined(__Win32__) || defined(__sgi__) || defined(__osf__) || defined(__hpux__)		
		typedef int socklen_t; // missing from some platform includes
	#endif
#endif

Socket::Socket(uint32_t inSocketType)
:   fState(inSocketType),
    fLocalAddrStrPtr(NULL),
    fLocalDNSStrPtr(NULL),
    fPortStr(fPortBuffer, kPortBufSizeInBytes)
{
    fLocalAddr.sin_addr.s_addr = 0;
    fLocalAddr.sin_port = 0;
    
    fDestAddr.sin_addr.s_addr = 0;
    fDestAddr.sin_port = 0;
}

Socket::~Socket()
{
	if (fFileDesc != -1)
	{
#ifdef __Win32__
		::closesocket(fFileDesc);
#else
		::close(fFileDesc);
#endif				
	}
	fFileDesc = -1;
}

void Socket::InitNonBlocking(int inFileDesc)
{
	fFileDesc = inFileDesc;

#ifdef __Win32__
	u_long one = 1;
	int err = ::ioctrlsocket(fFileDesc, FIONBIO, &one);
#else
	int flag = ::fcntl(fFileDesc, F_GETFL, 0);
	::fcntl(fFileDesc, F_SETFL, flag|O_NONBLOCK);
#endif
}

ET_Error Socket::Open(int theType)
{
    fFileDesc = ::socket(PF_INET, theType, 0);
    if (fFileDesc == -1)
        return (ET_Error)errno;
            
    //
    // Setup this socket's event context
    if (fState & kNonBlockingSocketType)	
        this->InitNonBlocking(fFileDesc);   

    return ET_NoErr;
}

void Socket::ReuseAddr()
{
    int one = 1;
    int err = ::setsockopt(fFileDesc, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(int));
    assert(err == 0);   
}

void Socket::NoDelay()
{
    int one = 1;
    int err = ::setsockopt(fFileDesc, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof(int));
	if (err != 0)
	{
        printf("set TCP_NODELAY failed.\n");
	}
    assert(err == 0);   
}

void Socket::KeepAlive()
{
    int one = 1;
    int err = ::setsockopt(fFileDesc, SOL_SOCKET, SO_KEEPALIVE, (char*)&one, sizeof(int));
    assert(err == 0);   
}

void    Socket::SetSocketBufSize(uint32_t inNewSize)
{
    int bufSize = inNewSize;
    ::setsockopt(fFileDesc, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, sizeof(int));
}

ET_Error    Socket::SetSocketRcvBufSize(uint32_t inNewSize)
{
    int bufSize = inNewSize;
    int err = ::setsockopt(fFileDesc, SOL_SOCKET, SO_RCVBUF, (char*)&bufSize, sizeof(int));

    if (err == -1)
        return errno;
        
    return ET_NoErr;
}


ET_Error Socket::Bind(uint32_t addr, uint16_t port)
{
    socklen_t len = sizeof(fLocalAddr);
    ::memset(&fLocalAddr, 0, sizeof(fLocalAddr));
    fLocalAddr.sin_family = AF_INET;
    fLocalAddr.sin_port = htons(port);
    fLocalAddr.sin_addr.s_addr = htonl(addr);
    
    int err = ::bind(fFileDesc, (sockaddr *)&fLocalAddr, sizeof(fLocalAddr));
    
    if (err == -1)
    {
        fLocalAddr.sin_port = 0;
        fLocalAddr.sin_addr.s_addr = 0;
        return (ET_Error)errno;
    }
    else ::getsockname(fFileDesc, (sockaddr *)&fLocalAddr, &len); // get the kernel to fill in unspecified values
    fState |= kBound;
    return ET_NoErr;
}

StrPtrLen*  Socket::GetLocalAddrStr()
{
	/*
    //Use the array of IP addr strings to locate the string formatted version
    //of this IP address.
    if (fLocalAddrStrPtr == NULL)
    {
        for (uint32_t x = 0; x < SocketUtils::GetNumIPAddrs(); x++)
        {
            if (SocketUtils::GetIPAddr(x) == ntohl(fLocalAddr.sin_addr.s_addr))
            {
                fLocalAddrStrPtr = SocketUtils::GetIPAddrStr(x);
                break;
            }
        }
    }

    assert(fLocalAddrStrPtr != NULL);
	*/
	return fLocalAddrStrPtr;
}

StrPtrLen*  Socket::GetLocalDNSStr()
{
	/*    
	//Do the same thing as the above function, but for DNS names
    assert(fLocalAddr.sin_addr.s_addr != INADDR_ANY);
    if (fLocalDNSStrPtr == NULL)
    {
        for (uint32_t x = 0; x < SocketUtils::GetNumIPAddrs(); x++)
        {
            if (SocketUtils::GetIPAddr(x) == ntohl(fLocalAddr.sin_addr.s_addr))
            {
                fLocalDNSStrPtr = SocketUtils::GetDNSNameStr(x);
                break;
            }
        }
    }

    //if we weren't able to get this DNS name, make the DNS name the same as the IP addr str.
    if (fLocalDNSStrPtr == NULL)
        fLocalDNSStrPtr = this->GetLocalAddrStr();

    assert(fLocalDNSStrPtr != NULL);
	*/
    return fLocalDNSStrPtr;
}

StrPtrLen*  Socket::GetLocalPortStr()
{
    if (fPortStr.Len == kPortBufSizeInBytes)
    {
        int temp = ntohs(fLocalAddr.sin_port);
        sprintf(fPortBuffer, "%d", temp);
        fPortStr.Len = ::strlen(fPortBuffer);
    }
    return &fPortStr;
}

ET_Error Socket::Send(const char* inData, const uint32_t inLength, uint32_t* outLengthSent)
{
    assert(inData != NULL);
    
    if (!(fState & kConnected))
        return (ET_Error)ENOTCONN;
        
    int err;
    do {
       err = ::send(fFileDesc, inData, inLength, 0);//flags??
    } while((err == -1) && (errno == EINTR));
    if (err == -1)
    {
        //Are there any errors that can happen if the client is connected?
        //Yes... EAGAIN. Means the socket is now flow-controleld
        int theErr = errno;
        if ((theErr != EAGAIN) && (this->IsConnected()))
            fState ^= kConnected;//turn off connected state flag
        return (ET_Error)theErr;
    }
    
    *outLengthSent = err;
    return ET_NoErr;
}

ET_Error Socket::WriteV(const struct iovec* iov, const uint32_t numIOvecs, uint32_t* outLenSent)
{
    assert(iov != NULL);

    if (!(fState & kConnected))
        return (ET_Error)ENOTCONN;
        
    int err;
    do {
#ifdef __Win32__
        DWORD theBytesSent = 0;
        err = ::WSASend(fFileDesc, (LPWSABUF)iov, numIOvecs, &theBytesSent, 0, NULL, NULL);
        if (err == 0)
            err = theBytesSent;
#else
       err = ::writev(fFileDesc, iov, numIOvecs);//flags??
#endif
    } while((err == -1) && (errno == EINTR));
    if (err == -1)
    {
        // Are there any errors that can happen if the client is connected?
        // Yes... EAGAIN. Means the socket is now flow-controleld
        int theErr = errno;
        if ((theErr != EAGAIN) && (this->IsConnected()))
            fState ^= kConnected;//turn off connected state flag
        return (ET_Error)theErr;
    }
    if (outLenSent != NULL)
        *outLenSent = (uint32_t)err;
        
    return ET_NoErr;
}

ET_Error Socket::Read(void *buffer, const uint32_t length, uint32_t *outRecvLenP)
{
    assert(outRecvLenP != NULL);
    assert(buffer != NULL);

    if (!(fState & kConnected))
        return (ET_Error)ENOTCONN;
            
    //int theRecvLen = ::recv(fFileDesc, buffer, length, 0);//flags??
    int theRecvLen;
    do {
       theRecvLen = ::recv(fFileDesc, (char*)buffer, length, 0);//flags??
    } while((theRecvLen == -1) && (errno == EINTR));

    if (theRecvLen == -1)
    {
        // Are there any errors that can happen if the client is connected?
        // Yes... EAGAIN. Means the socket is now flow-controleld
        int theErr = errno;
        if ((theErr != EAGAIN) && (this->IsConnected()))
            fState ^= kConnected;//turn off connected state flag
        return (ET_Error)theErr;
    }
    //if we get 0 bytes back from read, that means the client has disconnected.
    //Note that and return the proper error to the caller
    else if (theRecvLen == 0)
    {
        fState ^= kConnected;
        return (ET_Error)ENOTCONN;
    }
    assert(theRecvLen > 0);
    *outRecvLenP = (uint32_t)theRecvLen;
    return ET_NoErr;
}

ET_Error Socket::RequestEvent(uint32_t evMask)
{
	int err = 0;

	fd_set rdfds,wrfds;
	timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	FD_ZERO(&rdfds);
	FD_ZERO(&wrfds);

	if (evMask&EV_RE)
		FD_SET(fFileDesc, &rdfds);
	if (evMask&EV_WR)
		FD_SET(fFileDesc, &wrfds);

	do {
		err = ::select(fFileDesc+1, &rdfds, &wrfds, 0, &tv);	
	} while ((-1 == err) && (EINTR == errno));

	if (err == 0) return ET_NETTIMEOUT;
	else if (err < 0) return ET_NETERROR;
	else return ET_NoErr;
}
