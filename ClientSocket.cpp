/*
    File:       ClientSocket.cpp

    
    
*/
#include "common.h"
#include "ClientSocket.h"

#define CLIENT_SOCKET_DEBUG 0

ClientSocket::ClientSocket()
:   fHostAddr(0),
    fHostPort(0),
    fEventMask(0),
    fSocketP(NULL),
    fSendBuffer(fSendBuf, 0),
    fSentLength(0)
{}

ET_Error ClientSocket::Open(TCPSocket* inSocket)
{
    ET_Error theErr = ET_NoErr;
    if (!inSocket->IsBound())
    {
        theErr = inSocket->Open();
		inSocket->NoDelay();
        if (theErr == ET_NoErr)
            theErr = inSocket->Bind(0, 0);

        if (theErr != ET_NoErr)
            return theErr;
            
        inSocket->NoDelay();
#if __FreeBSD__ || __MacOSX__
    // no KeepAlive -- probably should be off for all platforms.
#else
        inSocket->KeepAlive();
#endif

    }
    return theErr;
}

ET_Error ClientSocket::Connect(TCPSocket* inSocket)
{
    ET_Error theErr = this->Open(inSocket);
   // assert(theErr == ET_NoErr);
    if (theErr != ET_NoErr)
        return theErr;

    if (!inSocket->IsConnected())
    {
        theErr = inSocket->Connect(fHostAddr, fHostPort);
        if ((theErr == EINPROGRESS) || (theErr == EAGAIN))
        {
            fSocketP = inSocket;
			fEventMask = EV_RE | EV_WR;
            return theErr;
        }
    }
    return theErr;
}

ET_Error ClientSocket::Send(char* inData, const uint32_t inLength)
{
    iovec theVec[1];
    theVec[0].iov_base = (char*)inData;
    theVec[0].iov_len = inLength;
    
    return this->SendV(theVec, 1);
}

ET_Error ClientSocket::SendSendBuffer(TCPSocket* inSocket)
{
    ET_Error theErr = ET_NoErr;
    uint32_t theLengthSent = 0;
    
    if (fSendBuffer.Len == 0)
        return ET_NoErr;
    
    do
    {
        // theLengthSent should be reset to zero before passing its pointer to Send function
        // otherwise the old value will be used and it will go into an infinite loop sometimes
        theLengthSent = 0;
        //
        // Loop, trying to send the entire message.
        theErr = inSocket->Send(fSendBuffer.Ptr + fSentLength, fSendBuffer.Len - fSentLength, &theLengthSent);
        fSentLength += theLengthSent;
        
    } while (theLengthSent > 0);
    
    if (theErr == ET_NoErr)
        fSendBuffer.Len = fSentLength = 0; // Message was sent
    else
    {
        // Message wasn't entirely sent. Caller should wait for a read event on the POST socket
        fSocketP = inSocket;
		fEventMask = EV_WR;
    }
    return theErr;
}


TCPClientSocket::TCPClientSocket(uint32_t inSocketType)
 : fSocket(inSocketType)
{
    //
    // It is necessary to open the socket right when we construct the
    // object because the QTSSSplitterModule that uses this class uses
    // the socket file descriptor in the QTSS_CreateStreamFromSocket call.
    fSocketP = &fSocket;
    this->Open(&fSocket);
}

void TCPClientSocket::SetOptions(int sndBufSize,int rcvBufSize)
{   //set options on the socket

    //qtss_printf("TCPClientSocket::SetOptions sndBufSize=%d,rcvBuf=%d,keepAlive=%d,noDelay=%d\n",sndBufSize,rcvBufSize,(int)keepAlive,(int)noDelay);
    int err = 0;
    err = ::setsockopt(fSocket.GetSocketFD(), SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(int));
		   
    err = ::setsockopt(fSocket.GetSocketFD(), SOL_SOCKET, SO_RCVBUF, (char*)&rcvBufSize, sizeof(int));

#if __FreeBSD__ || __MacOSX__
    struct timeval time;
    //int len = sizeof(time);
    time.tv_sec = 0;
    time.tv_usec = 0;

    err = ::setsockopt(fSocket.GetSocketFD(), SOL_SOCKET, SO_RCVTIMEO, (char*)&time, sizeof(time));

    err = ::setsockopt(fSocket.GetSocketFD(), SOL_SOCKET, SO_SNDTIMEO, (char*)&time, sizeof(time));
#endif

}

ET_Error TCPClientSocket::SendV(iovec* inVec, uint32_t inNumVecs)
{
    if (fSendBuffer.Len == 0)
    {
        for (uint32_t count = 0; count < inNumVecs; count++)
        {
            ::memcpy(fSendBuffer.Ptr + fSendBuffer.Len, inVec[count].iov_base, inVec[count].iov_len);
            fSendBuffer.Len += inVec[count].iov_len;
            assert(fSendBuffer.Len < ClientSocket::kSendBufferLen);
        }
    }
    
    ET_Error theErr = this->Connect(&fSocket);
    if (theErr != ET_NoErr)
        return theErr;
        
    return this->SendSendBuffer(&fSocket);
}
            
ET_Error TCPClientSocket::Read(void* inBuffer, const uint32_t inLength, uint32_t* outRcvLen)
{
    this->Connect(&fSocket);
    ET_Error theErr = fSocket.Read(inBuffer, inLength, outRcvLen);
    if (theErr != ET_NoErr)
		fEventMask = EV_RE;
	
    return theErr;
}
