/*
    File:       RTSPClient.cpp

    Contains:   .  
                    
    
*/

#include "common.h"

#include "RTSPClient.h"
#include "StringParser.h"
#include "ArrayObjectDeleter.h"

#define ENABLE_AUTHENTICATION 0
#define OS_NotEnoughSpace -2

// STRTOCHAR is used in verbose mode and for debugging
static char temp[2048]; 
static char * STRTOCHAR(StrPtrLen *theStr)
{
    temp[0] = 0;
    uint32_t len = theStr->Len < 2047 ? theStr->Len : 2047;
    if (theStr->Len > 0 || NULL != theStr->Ptr)
    {   memcpy(temp,theStr->Ptr,len); 
        temp[len] = 0;
    }
    else 
        strcpy(temp,"Empty Ptr or len is 0");
    return temp;
}


static const char* sEmptyString = "";

namespace MyDarwin
{
const char* RTSPClient::sUserAgent = "RTSPPusherNode";
const char* RTSPClient::sControlID = "trackID";
RTSPClient::InterleavedParams RTSPClient::sInterleavedParams;

RTSPClient::RTSPClient(ClientSocket* inSocket, bool verbosePrinting, char* inUserAgent)
:	fSocket(inSocket),
    fCSeq(1),
    fStatus(0),
    fSessionID((char*)sEmptyString),
    fServerPort(0),
    fContentLength(1),
    fSetupHeaders(NULL),
    fNumChannelElements(kMinNumChannelElements),
    fNumFieldIDElements(0),
    fFieldIDMapSize(kMinNumChannelElements),
    fPacketBuffer(NULL),
    fPacketBufferOffset(0),
    fPacketOutstanding(false),
    fRecvContentBuffer(NULL),
    fContentRecvLen(0),
    fHeaderRecvLen(0),
    fHeaderLen(0),
    fSetupTrackID(0),
    fState(kInitial),
    fAuthAttempted(false),
    fTransportMode(kPushMode), 
    fPacketDataInHeaderBufferLen(0),
    fPacketDataInHeaderBuffer(NULL),
    fUserAgent(NULL),
    fControlID((char*)RTSPClient::sControlID),
	fGuarenteedBitRate(0),
	fMaxBitRate(0),
	fMaxTransferDelay(0),
	fBandwidth(0),
	fBufferSpace(0),
	fDelayTime(0)
{
    fChannelTrackMap = new ChannelMapElem[kMinNumChannelElements];
    ::memset(fChannelTrackMap, 0, sizeof(ChannelMapElem) * kMinNumChannelElements);

    fFieldIDMap = new FieldIDArrayElem[kMinNumChannelElements];
    ::memset(fFieldIDMap, 0, sizeof(FieldIDArrayElem) * kMinNumChannelElements);
    
    ::memset(fSendBuffer, 0,kReqBufSize + 1);
    ::memset(fRecvHeaderBuffer, 0,kReqBufSize + 1);
    
    fSetupHeaders = new char[2];
    fSetupHeaders[0] = '\0';
    
    ::memset(&sInterleavedParams, 0, sizeof(sInterleavedParams));
        
        if (inUserAgent != NULL)
        {
            fUserAgent = new char[::strlen(inUserAgent) + 1];
            ::strcpy(fUserAgent, inUserAgent);
        }
        else
        {
            fUserAgent = new char[::strlen(sUserAgent) + 1];
            ::strcpy(fUserAgent, sUserAgent);
        }
}

RTSPClient::~RTSPClient()
{
    delete [] fRecvContentBuffer;
    delete [] fURL.Ptr;
    delete [] fName.Ptr;
    delete [] fPassword.Ptr;
    if (fSessionID.Ptr != sEmptyString)
        delete [] fSessionID.Ptr;
        
    delete [] fSetupHeaders;
    delete [] fChannelTrackMap;
    delete [] fFieldIDMap;
    delete [] fPacketBuffer;
	
	//delete fAuthenticator;
        
    delete [] fUserAgent;
}

void RTSPClient::SetName(char *name)
{ 
    assert (name);  
    delete [] fName.Ptr;  
    fName.Ptr = new char[::strlen(name) + 2]; 
    ::strcpy(fName.Ptr, name);
    fName.Len = ::strlen(name);
}

void RTSPClient::SetPassword(char *password)
{   
    assert (password);  
    delete [] fPassword.Ptr;  
    fPassword.Ptr = new char[::strlen(password) + 2]; 
    ::strcpy(fPassword.Ptr, password);
    fPassword.Len = ::strlen(password);
}

void RTSPClient::Set(const StrPtrLen& inURL)
{
    delete [] fURL.Ptr;
    fURL.Ptr = new char[inURL.Len + 2];
    fURL.Len = inURL.Len;
    char* destPtr = fURL.Ptr;
    
    // add a leading '/' to the url if it isn't a full URL and doesn't have a leading '/'
    if ( !inURL.NumEqualIgnoreCase("rtsp://", strlen("rtsp://")) && inURL.Ptr[0] != '/')
    {
        *destPtr = '/';
        destPtr++;
        fURL.Len++;
    }
    ::memcpy(destPtr, inURL.Ptr, inURL.Len);
    fURL.Ptr[fURL.Len] = '\0';
}

ET_Error RTSPClient::SendDescribe()
{
    if (!IsTransactionInProgress())
    {
        sprintf(fMethod,"%s","DESCRIBE");

		StringFormatter fmt(fSendBuffer, kReqBufSize);
        fmt.PutFmtStr(
            	"DESCRIBE %s RTSP/1.0\r\n"
				"CSeq: %u\r\n"
				"Accept: application/sdp\r\n"
				"User-agent: %s\r\n",
				fURL.Ptr, fCSeq, fUserAgent);
		if (fBandwidth != 0)
			fmt.PutFmtStr("Bandwidth: %u\r\n", fBandwidth);
		
		fmt.PutFmtStr("\r\n");
		fmt.PutTerminator();
    }
    return this->DoTransaction();
}

ET_Error RTSPClient::SendSetParameter()
{
    if (!IsTransactionInProgress())
    {
        sprintf(fMethod,"%s","SET_PARAMETER");
        sprintf(fSendBuffer, "SET_PARAMETER %s RTSP/1.0\r\nCSeq:%u\r\nUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fUserAgent);
    }
    return this->DoTransaction();
}

ET_Error RTSPClient::SendOptions()
{
    if (!IsTransactionInProgress())
    {
        sprintf(fMethod,"%s","OPTIONS");
        sprintf(fSendBuffer, "OPTIONS * RTSP/1.0\r\nCSeq:%u\r\nUser-agent: %s\r\n\r\n", fCSeq, fUserAgent);
     }
    return this->DoTransaction();
}

ET_Error RTSPClient::SendUDPSetup(uint32_t inTrackID, uint16_t inClientPort)
{
    fSetupTrackID = inTrackID; // Needed when SETUP response is received.
    
    if (!IsTransactionInProgress())
    {
        sprintf(fMethod,"%s","SETUP");
        
		StringFormatter fmt(fSendBuffer, kReqBufSize);
		
        if (fTransportMode == kPushMode)
			fmt.PutFmtStr(
                    "SETUP %s/%s=%u RTSP/1.0\r\n"
                    "CSeq: %u\r\n"
                    "%sTransport: RTP/AVP;unicast;client_port=%u-%u;mode=record\r\n"
                    "User-agent: %s\r\n",
                    fURL.Ptr,fControlID, inTrackID, fCSeq, fSessionID.Ptr, inClientPort, inClientPort + 1, fUserAgent);
        else
            fmt.PutFmtStr(
                    "SETUP %s/%s=%u RTSP/1.0\r\n"
                    "CSeq: %u\r\n"
                    "%sTransport: RTP/AVP;unicast;client_port=%u-%u\r\n"
                    "%sUser-agent: %s\r\n",
                    fURL.Ptr,fControlID, inTrackID, fCSeq, fSessionID.Ptr, inClientPort, inClientPort + 1, fSetupHeaders, fUserAgent);

		if (fBandwidth != 0)
			fmt.PutFmtStr("Bandwidth: %u\r\n", fBandwidth);

		//Attach3GPPHeaders(fmt, inTrackID);
		fmt.PutFmtStr("\r\n");
		fmt.PutTerminator();
    }
    return this->DoTransaction();
}

ET_Error RTSPClient::SendTCPSetup(uint32_t inTrackID, uint16_t inClientRTPid, uint16_t inClientRTCPid, StrPtrLen* inTrackNamePtr)
{
    fSetupTrackID = inTrackID; // Needed when SETUP response is received.
	
	char trackName[64] = { 0 };
	if(inTrackNamePtr)
		::strncpy(trackName,inTrackNamePtr->Ptr, inTrackNamePtr->Len);
	else
		::sprintf(trackName,"%s=%u",fControlID, inTrackID);
    
    if (!IsTransactionInProgress())
    {   
        sprintf(fMethod,"%s","SETUP");
        
		StringFormatter fmt(fSendBuffer, kReqBufSize);
		
        if (fTransportMode == kPushMode)
			fmt.PutFmtStr(
                    "SETUP %s/%s RTSP/1.0\r\n"
                    "CSeq: %u\r\n"
                    "%sTransport: RTP/AVP/TCP;unicast;mode=record;interleaved=%u-%u\r\n"
                    "User-agent: %s\r\n",
                    fURL.Ptr, trackName, fCSeq, fSessionID.Ptr,inClientRTPid, inClientRTCPid, fUserAgent);
        else
            fmt.PutFmtStr(
                    "SETUP %s/%s RTSP/1.0\r\n"
                    "CSeq: %u\r\n"
                    "%sTransport: RTP/AVP/TCP;unicast;interleaved=%u-%u\r\n"
                    "%sUser-agent: %s\r\n",
                    fURL.Ptr, trackName, fCSeq, fSessionID.Ptr, inClientRTPid, inClientRTCPid,fSetupHeaders, fUserAgent);

		if (fBandwidth != 0)
			fmt.PutFmtStr("Bandwidth: %u\r\n", fBandwidth);

        //Attach3GPPHeaders(fmt, inTrackID);
		fmt.PutFmtStr("\r\n");
		fmt.PutTerminator();

    }

    return this->DoTransaction();

}

ET_Error RTSPClient::SendPlay(uint32_t inStartPlayTimeInSec, float inSpeed, uint32_t inTrackID)
{
    char speedBuf[128];
    speedBuf[0] = '\0';
    
    if (inSpeed != 1)
        sprintf(speedBuf, "Speed: %f5.2\r\n", inSpeed);
        
    if (!IsTransactionInProgress())
    {
	 	sprintf(fMethod,"%s","PLAY");
		
		StringFormatter fmt(fSendBuffer, kReqBufSize);
		
        fmt.PutFmtStr(
				"PLAY %s RTSP/1.0\r\n"
				"CSeq: %u\r\n"
				"%sRange: npt=%u.0-\r\n" 
				"%sx-prebuffer: maxtime=3.0\r\n" 
				"User-agent: %s\r\n",
				fURL.Ptr, fCSeq, fSessionID.Ptr, inStartPlayTimeInSec, speedBuf, fUserAgent);

		if (fBandwidth != 0)
			fmt.PutFmtStr("Bandwidth: %u\r\n", fBandwidth);

        //Attach3GPPHeaders(fmt, inTrackID);
		fmt.PutFmtStr("\r\n");
		fmt.PutTerminator();

        //sprintf(fSendBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %u\r\n%sRange: npt=7.0-\r\n%sx-prebuffer: maxtime=3.0\r\nUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fSessionID.Ptr, speedBuf, fUserAgent);
    }
    return this->DoTransaction();
}

ET_Error RTSPClient::SendAnnounce(char *sdp)
{
//ANNOUNCE rtsp://server.example.com/permanent_broadcasts/TestBroadcast.sdp RTSP/1.0
    if (!IsTransactionInProgress())
    {   
        sprintf(fMethod,"%s","ANNOUNCE");
		
		//&token=xxxxxx
		char tmpUrl[128] = {0};
		char *tmpUrlPtr = tmpUrl;

		if (fName.Ptr != NULL)
			sprintf(tmpUrl, "%s&token=%s", fURL.Ptr, fName.Ptr);
		else 
			tmpUrlPtr = fURL.Ptr;	

        if (sdp == NULL)
            sprintf(fSendBuffer, "ANNOUNCE %s RTSP/1.0\r\nCSeq: %u\r\nAccept: application/sdp\r\nUser-agent: %s\r\n\r\n", tmpUrlPtr, fCSeq, fUserAgent);
        else
        {   uint32_t len = strlen(sdp);
            if(len > kReqBufSize)
                return OS_NotEnoughSpace;
            sprintf(fSendBuffer, "ANNOUNCE %s RTSP/1.0\r\nCSeq: %u\r\nContent-Type: application/sdp\r\nUser-agent: %s\r\nContent-Length: %u\r\n\r\n%s", tmpUrlPtr, fCSeq, fUserAgent, len, sdp);
        }   
    }
    return this->DoTransaction();
}

ET_Error RTSPClient::SendRTSPRequest(iovec* inRequest, uint32_t inNumVecs)
{
    if (!IsTransactionInProgress())
    {
        uint32_t curOffset = 0;
        for (uint32_t x = 0; x < inNumVecs; x++)
        {
            ::memcpy(fSendBuffer + curOffset, inRequest[x].iov_base, inRequest[x].iov_len);
            curOffset += inRequest[x].iov_len;
        }
        assert(kReqBufSize > curOffset);
        fSendBuffer[curOffset] = '\0';
    }
    return this->DoTransaction();
}

ET_Error RTSPClient::PutMediaPacket(uint32_t inTrackID, bool isRTCP, char* inData, uint16_t inLen)
{
    // Find the right channel number for this trackID
    for (int x = 0; x < fNumChannelElements; x++)
    {
        if ((fChannelTrackMap[x].fTrackID == inTrackID) && (fChannelTrackMap[x].fIsRTCP == isRTCP))
        {
            char header[5];
            header[0] = '$';
            header[1] = (uint8_t)x;
            uint16_t* theLenP = (uint16_t*)header;
            theLenP[1] = htons(inLen);
                    
            //
            // Build the iovec
            iovec ioVec[2];
            ioVec[0].iov_len = 4;
            ioVec[0].iov_base = header;
            ioVec[1].iov_len = inLen;
            ioVec[1].iov_base = inData;
            
            //
            // Send it
            return fSocket->SendV(ioVec, 2);
        }
    }
    
    return ET_NoErr;
}


ET_Error RTSPClient::SendInterleavedWrite(uint8_t channel, uint16_t len, char*data,bool *getNext)
{

    assert(len < RTSPClient::kReqBufSize);
    
    iovec ioVEC[1];
    struct iovec* iov = &ioVEC[0];
    uint16_t interleavedLen =0;   
    uint16_t sendLen = 0;
    
    if (sInterleavedParams.extraLen > 0)
    {   *getNext = false; // can't handle new packet now. Send it again
        ioVEC[0].iov_base   = sInterleavedParams.extraBytes;
        ioVEC[0].iov_len    = sInterleavedParams.extraLen;
        sendLen = sInterleavedParams.extraLen;
    }
    else
    {   *getNext = true; // handle a new packet
        fSendBuffer[0] = '$';
        fSendBuffer[1] = channel;
        uint16_t netlen = htons(len);
        memcpy(&fSendBuffer[2],&netlen,2);
        memcpy(&fSendBuffer[4],data,len);
        
        interleavedLen = len+4;
        ioVEC[0].iov_base=&fSendBuffer[0];
        ioVEC[0].iov_len= interleavedLen;
        sendLen = interleavedLen;
        sInterleavedParams.extraChannel =channel;
    }   
        
    uint32_t outLenSent;
    ET_Error theErr = fSocket->GetSocket()->WriteV(iov, 1,&outLenSent);
    if (theErr != 0)
        outLenSent = 0;

    if (theErr == 0 && outLenSent != sendLen) 
    {   if (sInterleavedParams.extraLen > 0) // sending extra len so keep sending it.
        {   
            sInterleavedParams.extraLen = sendLen - outLenSent;
            sInterleavedParams.extraByteOffset += outLenSent;
            sInterleavedParams.extraBytes = &fSendBuffer[sInterleavedParams.extraByteOffset];
        }
        else // we were sending a new packet so record the data
        {   
            sInterleavedParams.extraBytes = &fSendBuffer[outLenSent];
            sInterleavedParams.extraLen = sendLen - outLenSent;
            sInterleavedParams.extraChannel = channel;
            sInterleavedParams.extraByteOffset = outLenSent;
        }
    }
    else // either an error occured or we sent everything ok
    {
        if (theErr == 0)
        {   
            if (sInterleavedParams.extraLen > 0) // we were busy sending some old data and it all got sent
            {   
				//printf("RTSPClient::SendInterleavedWrite FULL Send channel=%u bufferlen=%u err=%d amountSent=%u \n",(uint16_t) sInterleavedParams.extraChannel,sendLen,theErr,outLenSent);
            }
            else 
            {   // it all worked so ask for more data
				//printf("RTSPClient::SendInterleavedWrite FULL Send channel=%u bufferlen=%u err=%d amountSent=%u \n",(uint16_t) channel,sendLen,theErr,outLenSent);
            }
            sInterleavedParams.extraLen = 0;
            sInterleavedParams.extraBytes = NULL;
            sInterleavedParams.extraByteOffset = 0;
        }
        else // we got an error so nothing was sent
        {   
            if (sInterleavedParams.extraLen == 0) // retry the new packet
            {   
                sInterleavedParams.extraBytes = &fSendBuffer[0];
                sInterleavedParams.extraLen = sendLen;
                sInterleavedParams.extraChannel = channel;              
                sInterleavedParams.extraByteOffset = 0;
            }       
        }
    }   
    return theErr;          
}

ET_Error RTSPClient::SendTeardown()
{
    if (!IsTransactionInProgress())
    {   sprintf(fMethod,"%s","TEARDOWN");
        sprintf(fSendBuffer, "TEARDOWN %s RTSP/1.0\r\nCSeq: %u\r\n%sUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fSessionID.Ptr, fUserAgent);
    }
    return this->DoTransaction();
}

uint32_t  RTSPClient::GetSSRCByTrack(uint32_t inTrackID)
{
    for (uint32_t x = 0; x < fSSRCMap.size(); x++)
    {
        if (inTrackID == fSSRCMap[x].fTrackID)
            return fSSRCMap[x].fSSRC;
    }
    return 0;
}

ET_Error RTSPClient::DoTransaction()
{
	ET_Error theErr = ET_NoErr;
    StrPtrLen theRequest(fSendBuffer, ::strlen(fSendBuffer));
    StrPtrLen theMethod(fMethod);
    
	for(;;)
	{
		switch(fState)
		{
			//Initial state: getting ready to send the request; the authenticator is initialized if it exists.
			//This is the only state where a new request can be made.
			case kInitial:
				/*
				if (fAuthenticator != NULL)
				{
					fAuthAttempted = true;
        			fAuthenticator->RemoveAuthLine(&theRequest); // reset to original request
        			fAuthenticator->ResetAuthParams(); // if we had a 401 on an authenticated request clean up old params and try again with the new response data
        			fAuthenticator->SetName(&fName);
        			fAuthenticator->SetPassword(&fPassword);
        			fAuthenticator->SetMethod(&theMethod);
        			fAuthenticator->SetURI(&fURL);
        			fAuthenticator->AttachAuthParams(&theRequest);
				}
				*/
				fCSeq++;	//this assumes that the sequence number will not be read again until the next transaction
        		fPacketDataInHeaderBufferLen = 0;

				fState = kRequestSending;
				break;

			//Request Sending state: keep on calling Send while Send returns EAGAIN or EINPROGRESS
			case kRequestSending:
        		theErr = fSocket->Send(theRequest.Ptr, theRequest.Len);
        
        		if (theErr != ET_NoErr)
				{
					//printf("RTSPClient::DoTransaction Send len=%u err = %d\n", theRequest.Len, theErr);
            		return theErr;
				}

				//Done sending request; moving onto the response
        		fContentRecvLen = 0;
        		fHeaderRecvLen = 0;
        		fHeaderLen = 0;
        		::memset(fRecvHeaderBuffer, 0, kReqBufSize+1);

            	fState = kResponseReceiving;
				break;

			//Response Receiving state: keep on calling ReceiveResponse while it returns EAGAIN or EINPROGRESS
			case kResponseReceiving:
			//Header Received state: the response header has been received(and parsed), but the entity(response content) has not been completely received
			case kHeaderReceived:
        		theErr = this->ReceiveResponse();  //note that this function can change the fState

        		if (theErr != ET_NoErr)
            		return theErr;

				//The response has been completely received and parsed.  If the response is 401 unauthorized, then redo the request with authorization
				fState = kInitial;
				if (fStatus == 401 /*&& fAuthenticator != NULL && !fAuthAttempted*/)
					break;
				else
					return ET_NoErr;
				break;
		}
	}
	assert(false);  //not reached
	return 0;
}


//This implementation cannot parse interleaved headers with entity content.
ET_Error RTSPClient::ReceiveResponse()
{
	assert(fState == kResponseReceiving | fState == kHeaderReceived);
    ET_Error theErr = ET_NoErr;

    while (fState == kResponseReceiving)
    {
        uint32_t theRecvLen = 0;
        //fRecvHeaderBuffer[0] = 0;
        theErr = fSocket->Read(&fRecvHeaderBuffer[fHeaderRecvLen], kReqBufSize - fHeaderRecvLen, &theRecvLen);
        if (theErr != ET_NoErr)
            return theErr;
        
        fHeaderRecvLen += theRecvLen;
        fRecvHeaderBuffer[fHeaderRecvLen] = 0;

        //fRecvHeaderBuffer[fHeaderRecvLen] = '\0';
        // Check to see if we've gotten a complete header, and if the header has even started       
        // The response may not start with the response if we are interleaving media data,
        // in which case there may be leftover media data in the stream. If we encounter any
        // of this cruft, we can just strip it off.
        char* theHeaderStart = ::strstr(fRecvHeaderBuffer, "RTSP");
        if (theHeaderStart == NULL)
        {
            fHeaderRecvLen = 0;
            continue;
        }
        else if (theHeaderStart != fRecvHeaderBuffer)
        {
			//strip off everything before the RTSP
            fHeaderRecvLen -= theHeaderStart - fRecvHeaderBuffer;
            ::memmove(fRecvHeaderBuffer, theHeaderStart, fHeaderRecvLen);
            //fRecvHeaderBuffer[fHeaderRecvLen] = '\0';
        }

        char* theResponseData = ::strstr(fRecvHeaderBuffer, "\r\n\r\n");    

        if (theResponseData != NULL)
        {               
            // skip past the \r\n\r\n
            theResponseData += 4;
            
            // We've got a new response
			fState = kHeaderReceived;
            
            // Figure out how much of the content body we've already received
            // in the header buffer. If we are interleaving, this may also be packet data
            fHeaderLen = theResponseData - &fRecvHeaderBuffer[0];
            fContentRecvLen = fHeaderRecvLen - fHeaderLen;

            // Zero out fields that will change with every RTSP response
            fServerPort = 0;
            fStatus = 0;
            fContentLength = 0;
        
            // Parse the response.
            StrPtrLen theData(fRecvHeaderBuffer, fHeaderLen);
            StringParser theParser(&theData);
            
            theParser.ConsumeLength(NULL, 9); //skip past RTSP/1.0
            fStatus = theParser.ConsumeInteger(NULL);

            StrPtrLen theLastHeader;
            while (theParser.GetDataRemaining() > 0)
            {
                static StrPtrLen sSessionHeader((char*)"Session");
                static StrPtrLen sContentLenHeader((char*)"Content-length");
                static StrPtrLen sTransportHeader((char*)"Transport");
                static StrPtrLen sRTPInfoHeader((char*)"RTP-Info");
                static StrPtrLen sAuthenticateHeader((char*)"WWW-Authenticate");
                static StrPtrLen sSameAsLastHeader((char*)" ,");
                
                StrPtrLen theKey;
                theParser.GetThruEOL(&theKey);
                
                if (theKey.NumEqualIgnoreCase(sSessionHeader.Ptr, sSessionHeader.Len))
                {
                    if (fSessionID.Len == 0)
                    {
                        // Copy the session ID and store it.
                        // First figure out how big the session ID is. We copy
                        // everything up until the first ';' returned from the server
                        uint32_t keyLen = 0;
                        while ((theKey.Ptr[keyLen] != ';') && (theKey.Ptr[keyLen] != '\r') && (theKey.Ptr[keyLen] != '\n'))
                            keyLen++;
                        
                        // Append an EOL so we can stick this thing transparently into the SETUP request
                        
                        fSessionID.Ptr = new char[keyLen + 3];
                        fSessionID.Len = keyLen + 2;
                        ::memcpy(fSessionID.Ptr, theKey.Ptr, keyLen);
                        ::memcpy(fSessionID.Ptr + keyLen, "\r\n", 2);//Append a EOL
                        fSessionID.Ptr[keyLen + 2] = '\0';
                    }
                }
                else if (theKey.NumEqualIgnoreCase(sContentLenHeader.Ptr, sContentLenHeader.Len))
                {
					//exclusive with interleaved
                    StringParser theCLengthParser(&theKey);
                    theCLengthParser.ConsumeUntil(NULL, StringParser::sDigitMask);
                    fContentLength = theCLengthParser.ConsumeInteger(NULL);
                    
                    delete [] fRecvContentBuffer;
                    fRecvContentBuffer = new char[fContentLength + 1];
					::memset(fRecvContentBuffer, '\0', fContentLength + 1);
                    
                    // Immediately copy the bit of the content body that we've already
                    // read off of the socket.
					assert(fContentRecvLen <= fContentLength);
                    ::memcpy(fRecvContentBuffer, theResponseData, fContentRecvLen);
                }
                else if (theKey.NumEqualIgnoreCase(sAuthenticateHeader.Ptr, sAuthenticateHeader.Len))
                {   
                    #if ENABLE_AUTHENTICATION   
                        if (fAuthenticator != NULL) // already have an authenticator
                            delete fAuthenticator;
            
                        fAuthenticator = fAuthenticationParser.ParseChallenge(&theKey);
                        assert(fAuthenticator != NULL);
                        if (!fAuthenticator) 
                            return 401; // what to do? the challenge is bad can't authenticate.
                    #else
                        printf("--AUTHENTICATION IS DISABLED\n");
                    #endif                  
                }
                else if (theKey.NumEqualIgnoreCase(sTransportHeader.Ptr, sTransportHeader.Len))
                {
                    StringParser theTransportParser(&theKey);
                    StrPtrLen theSubHeader;

                    while (theTransportParser.GetDataRemaining() > 0)
                    {
                        static StrPtrLen sServerPort((char*)"server_port");
                        static StrPtrLen sInterleaved((char*)"interleaved");
						static StrPtrLen sSSRC((char*)"ssrc");

                        theTransportParser.GetThru(&theSubHeader, ';');
                        if (theSubHeader.NumEqualIgnoreCase(sServerPort.Ptr, sServerPort.Len))
                        {
                            StringParser thePortParser(&theSubHeader);
                            thePortParser.ConsumeUntil(NULL, StringParser::sDigitMask);
                            fServerPort = (uint16_t) thePortParser.ConsumeInteger(NULL);
                        }
                        //else if (theSubHeader.NumEqualIgnoreCase(sInterleaved.Ptr, sInterleaved.Len))	//exclusive with Content-length
                        //    this->ParseInterleaveSubHeader(&theSubHeader);
						else if (theSubHeader.NumEqualIgnoreCase(sSSRC.Ptr, sSSRC.Len))
						{
                            StringParser ssrcParser(&theSubHeader);
							StrPtrLen ssrcStr;
							
							ssrcParser.GetThru(NULL, '=');
                            ssrcParser.ConsumeWhitespace();
                            ssrcParser.ConsumeUntilWhitespace(&ssrcStr);
							
							if (ssrcStr.Len > 0)
							{
								OSArrayObjectDeleter<char> ssrcCStr = ssrcStr.GetAsCString();
								unsigned long ssrc = ::strtoul(ssrcCStr.GetObject(), NULL, 16);
								fSSRCMap.push_back(SSRCMapElem(fSetupTrackID, ssrc));
							}
						}
                    }
                }
                else if (theKey.NumEqualIgnoreCase(sRTPInfoHeader.Ptr, sRTPInfoHeader.Len))
                    ParseRTPInfoHeader(&theKey);
                else if (theKey.NumEqualIgnoreCase(sSameAsLastHeader.Ptr, sSameAsLastHeader.Len))
                {
                    //
                    // If the last header was an RTP-Info header
                    if (theLastHeader.NumEqualIgnoreCase(sRTPInfoHeader.Ptr, sRTPInfoHeader.Len))
                        ParseRTPInfoHeader(&theKey);
                }
                theLastHeader = theKey;
            }
            
            //
            // Check to see if there is any packet data in the header buffer; everything that is left should be packet data
            if (fContentRecvLen > fContentLength)
            {
                fPacketDataInHeaderBuffer = theResponseData + fContentLength;
                fPacketDataInHeaderBufferLen = fContentRecvLen - fContentLength;
            }
        }
        else if (fHeaderRecvLen == kReqBufSize) //the "\r\n" is not found --> read more data
            return ENOBUFS; // This response is too big for us to handle!
    }
    
	//the advertised data length is less than what has been received...need to read more data
    while (fContentLength > fContentRecvLen)
    {
        uint32_t theContentRecvLen = 0;
        theErr = fSocket->Read(&fRecvContentBuffer[fContentRecvLen], fContentLength - fContentRecvLen, &theContentRecvLen);
        if (theErr != ET_NoErr)
        {
            //fEventMask = EV_RE;
            return theErr;
        }
        fContentRecvLen += theContentRecvLen;       
    }
    return ET_NoErr;
}

//DOES NOT CURRENT WORK AS ADVERTISED; so I took it out
//RTP-Info has sequence number, not SSRC
void    RTSPClient::ParseRTPInfoHeader(StrPtrLen* inHeader)
{
}

}//namespace

