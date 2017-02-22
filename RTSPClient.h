/*
    File:       RTSPClient.h

	Works only for QTSS.
	Assumes that different streams within a session is distinguished by trackID=
	For example, streams within sample.mov would be refered to as sample.mov/trackID=4
	Does not work if the URL contains digits!!!
*/

#ifndef __RTSP_CLIENT_H__
#define __RTSP_CLIENT_H__

#include "StrPtrLen.h"
#include "TCPSocket.h"
#include "ClientSocket.h"
#include "StringFormatter.h"
#include "SVector.h"

//#define ET_Error int

namespace MyDarwin
{
class RTSPClient
{
    public:
        // verbosePrinting = print out all requests and responses
        RTSPClient(ClientSocket* inSocket, bool verbosePrinting = false, char* inUserAgent = NULL);
        ~RTSPClient();
        
        // This must be called before any other function. Sets up very important info; sets the URL
        void        Set(const StrPtrLen& inURL);
        
        // Send specified RTSP request to server, wait for complete response.
        // These return EAGAIN if transaction is in progress, ET_NoErr if transaction
        // was successful, EINPROGRESS if connection is in progress, or some other
        // error if transaction failed entirely.
        ET_Error    SendDescribe();
        
        ET_Error    SendReliableUDPSetup(uint32_t inTrackID, uint16_t inClientPort);
        ET_Error    SendUDPSetup(uint32_t inTrackID, uint16_t inClientPort);
        ET_Error    SendTCPSetup(uint32_t inTrackID, uint16_t inClientRTPid, uint16_t inClientRTCPid, StrPtrLen* inTrackNamePtr = NULL);
        ET_Error    SendPlay(uint32_t inStartPlayTimeInSec, float inSpeed = 1, uint32_t inTrackID = UINT32_MAX); //use a inTrackID of UINT32_MAX to turn off per stream headers
        ET_Error    SendAnnounce(char *sdp);
        ET_Error    SendTeardown();
        ET_Error    SendInterleavedWrite(uint8_t channel, uint16_t len, char*data,bool *getNext);
                
        ET_Error    SendSetParameter();
        ET_Error    SendOptions();
        //
        // If you just want to send a generic request, do it this way
        ET_Error    SendRTSPRequest(iovec* inRequest, uint32_t inNumVecs);

                //
        // Once you call all of the above functions, assuming they return an error, you
        // should call DoTransaction until it returns ET_NoErr, then you can move onto your
        // next request
        ET_Error    DoTransaction();
        
        // If any of the tracks are being interleaved, this puts a media packet to the control
        // connection. This function assumes that SendPlay has already completed
        // successfully and media packets are being sent.
        ET_Error    PutMediaPacket( uint32_t inTrackID, bool isRTCP, char* inBuffer, uint16_t inLen);

        // set name and password for authentication in case we are challenged
        void    SetName(char *name);                
        void    SetPassword(char *password);

        //Use 0 for undefined
        void SetBandwidth(uint32_t bps = 0)   { fBandwidth = bps; }
  
         // ACCESSORS

        StrPtrLen*  GetURL()                { return &fURL; }
        uint32_t      GetStatus()             { return fStatus; }
        StrPtrLen*  GetSessionID()          { return &fSessionID; }
        uint16_t      GetServerPort()         { return fServerPort; }
        uint32_t      GetContentLength()      { return fContentLength; }
        char*       GetContentBody()        { return fRecvContentBuffer; }
        ClientSocket*   GetSocket()         { return fSocket; }
        uint32_t      GetTransportMode()      { return fTransportMode; }
        void        SetTransportMode(uint32_t theMode)  { fTransportMode = theMode; };
        
        char*       GetResponse()           { return fRecvHeaderBuffer; }
        uint32_t      GetResponseLen()        { return fHeaderLen; }
        bool      IsTransactionInProgress() { return fState != kInitial; }
        
        enum { kPlayMode=0,kPushMode=1,kRecordMode=2};

        //
        // If available, returns the RTP-Meta-Info field ID array for
        // a given track. For more details, see RTPMetaInfoPacket.h
        //RTPMetaInfoPacket::FieldID* GetFieldIDArrayByTrack(uint32_t inTrackID);
        
        enum
        {
            kMinNumChannelElements = 5,
            kReqBufSize = 4095,
            kMethodBuffLen = 24 //buffer for "SETUP" or "PLAY" etc.
        };
        
        //OSMutex*            GetMutex()      { return &fMutex; }

    private:
        static const char*   sUserAgent; 
    
        // Helper methods void        ParseInterleaveSubHeader(StrPtrLen* inSubHeader);
        void        ParseRTPInfoHeader(StrPtrLen* inHeader);

		uint32_t		GetSSRCByTrack(uint32_t inTrackID);
        // Call this to receive an RTSP response from the server.
        // Returns EAGAIN until a complete response has been received.
        ET_Error    ReceiveResponse();
        
        //OSMutex             fMutex;//this data structure is shared!
        
        //AuthParser      fAuthenticationParser;
        //DSSAuthenticator   *fAuthenticator; // only one will be supported

        ClientSocket*   fSocket;

        // Information we need to send the request
        StrPtrLen   fURL;
        uint32_t      fCSeq;
        
        // authenticate info
        StrPtrLen   fName;
        StrPtrLen   fPassword;
        
        // Response data we get back
        uint32_t      fStatus;
        StrPtrLen   fSessionID;
        uint16_t      fServerPort;
        uint32_t      fContentLength;
        //StrPtrLen   fRTPInfoHeader;
        
        // Special purpose SETUP params
        char*           fSetupHeaders;
        
        // If we are interleaving, this maps channel numbers to track IDs
        struct ChannelMapElem
        {
            uint32_t  fTrackID;
            bool  fIsRTCP;
        };
        ChannelMapElem* fChannelTrackMap;
        uint8_t           fNumChannelElements;
        
        //Maps between trackID and SSRC number
        struct SSRCMapElem
        {
            uint32_t fTrackID;
            uint32_t fSSRC;
			SSRCMapElem(uint32_t trackID = UINT32_MAX, uint32_t inSSRC = 0)
			: fTrackID(trackID), fSSRC(inSSRC)
			{}
        };
		SVector<SSRCMapElem> fSSRCMap;

        // For storing FieldID arrays
        struct FieldIDArrayElem
        {
            uint32_t fTrackID;
         //   RTPMetaInfoPacket::FieldID fFieldIDs[RTPMetaInfoPacket::kNumFields];
        };
        FieldIDArrayElem*   fFieldIDMap;
        uint32_t              fNumFieldIDElements;
        uint32_t              fFieldIDMapSize;
        
        // If we are interleaving, we need this stuff to support the GetMediaPacket function
        char*           fPacketBuffer;
        uint32_t          fPacketBufferOffset;
        bool          fPacketOutstanding;
        
        
        // Data buffers
        char        fMethod[kMethodBuffLen]; // holds the current method
        char        fSendBuffer[kReqBufSize + 1];   // for sending requests
        char        fRecvHeaderBuffer[kReqBufSize + 1];// for receiving response headers
        char*       fRecvContentBuffer;             // for receiving response body

        // Tracking the state of our receives
        uint32_t      fContentRecvLen;
        uint32_t      fHeaderRecvLen;
        uint32_t      fHeaderLen;
        uint32_t      fSetupTrackID;					//is valid during a Setup Transaction
        
        enum { kInitial, kRequestSending, kResponseReceiving, kHeaderReceived };
        uint32_t      fState;

        //uint32_t      fEventMask;
        bool      fAuthAttempted;
        uint32_t      fTransportMode;

        //
        // For tracking media data that got read into the header buffer
        uint32_t      fPacketDataInHeaderBufferLen;
        char*       fPacketDataInHeaderBuffer;

            
        char*       fUserAgent;
static  const char*       sControlID;
        char*       fControlID;

        //These values are for the wireless links only -- not end-to-end
		//For the following values; use 0 for undefined.
        uint32_t fGuarenteedBitRate;				//kbps
        uint32_t fMaxBitRate;						//kbps
        uint32_t fMaxTransferDelay;				//milliseconds
		uint32_t fBandwidth;						//bps
		uint32_t fBufferSpace;					//bytes
		uint32_t fDelayTime;						//milliseconds
        
        struct InterleavedParams
        {
            char    *extraBytes;
            int     extraLen;
            uint8_t   extraChannel;
            int     extraByteOffset;
        };
        static InterleavedParams sInterleavedParams;
        
};
}//namespace
#endif //__CLIENT_SESSION_H__
