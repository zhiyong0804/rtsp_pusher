/**
 * @file PusherHandler.cpp
 * @brief  Pusher Handler 实现
 * @author lizhiyong0804319@gmail.com
 * @version 1.0
 * @date 2015-12-01
 */
#include "common.h"
#include "PusherHandler.h"
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "RTPPacket.h"

#define MAX_RTP_PAYLOAD 1400
#define RTP_HDR_SZ 12


PusherHandler* PusherHandler::createNew()
{
	return new PusherHandler();
}

int PusherHandler::setCallbackFunc(PusherCallback cb, void* cbParam)
{
	m_callbackFunc = cb;
	m_cbParam = cbParam;
	return 0;
}

int PusherHandler::startStream(const char* url, 
	                                RTP_ConnectType connType, 
	                                const char* username,
		                            const char* password, 
		                            int reconn, 
		                            const MediaInfo& mi)
{
	int ret = 0;

	char addr[32] = {0};
	int port = 0;

	if (m_rtspClient == NULL)
	{
		char *tuser = NULL, *tpasswd = NULL;	
		ret = parseDetailRTSPURL(url, tuser, tpasswd, &addr[0], &port);
		if (ret < 0) return ret;

		m_socket = new TCPClientSocket(Socket::kNonBlockingSocketType);
		uint32_t ipAddr = (uint32_t)ntohl(::inet_addr(addr));
		m_socket->Set(ipAddr, port);
		m_rtspClient = new MyDarwin::RTSPClient(m_socket);
		m_rtspClient->Set((char*)url);

		if (tuser != NULL)
		{
			m_rtspClient->SetName(tuser);
			delete[] tuser;
			
		}
		if (tpasswd != NULL)
		{
			m_rtspClient->SetPassword(tpasswd);
			delete[] tpasswd;
		}
	
		if (username != NULL)
		{
			m_rtspClient->SetName((char*)username);
		}

		if (password != NULL)
		{
			m_rtspClient->SetPassword((char*)password);
		}
	}
	else
	{
		// is running.
		return ret;
	}

    //����SDP
	m_connType = connType;
	memcpy(&m_mediaInfo, &mi, sizeof(mi));
	ret = generateSDPString(addr, mi);

    m_state = kSendingOptions;
	return SetupStream();
}

int PusherHandler::closeStream()
{
    int theErr = ET_NoErr;

    if (m_state != kSendingTeardown)
    {
	    m_state = kSendingTeardown;
		if (m_rtspClient != NULL )
		{
            theErr = m_rtspClient->SendTeardown();
            if (theErr == ET_NoErr)
            {
                m_state = kDone;
                m_pusherState = PUSHER_STATE_DISCONNECTED;
			    if (m_callbackFunc != NULL) 
				    m_callbackFunc(m_pusherState, 0, m_cbParam);

			return 0;
            }
		}
    }

	return -1;
}

int PusherHandler::release() 
{
	if (m_rtspClient != NULL)
	{
		delete m_rtspClient;
		m_rtspClient = NULL;
	}

	if (m_sdp != NULL)
	{
		delete[] m_sdp;
		m_sdp = NULL;
	}

	m_rtpSeq = 0;
    delete this; 
    return 0; 
}

int PusherHandler::pushFrame(MediaFrame* frame)
{
	if (frame == NULL || m_state != kPushing) return ET_NotInPushingState;
	char buf[1500] = {0};
    uint32_t len = sizeof(buf);
	uint32_t timestamp = 0;
    int readSz = RTP_HDR_SZ;
	int theErr = ET_NoErr;

#if 0
	MediaFrame* mframe = new MediaFrame;
	if (mframe == NULL) return ET_NotEnoughSpace;

	mframe->frameData = new uint8_t[frame->frameLen+4];
	if (mframe->frameData == NULL) 
	{
		delete mframe;
		return ET_NotEnoughSpace;
	}

	::memset(mframe->frameData, 0, 4);
	::memcpy(mframe->frameData+4, frame->frameData, frame->frameLen);
	mframe->frameLen = frame->frameLen+4;
	mframe->timestampSec = frame->timestampSec;
	mframe->timestampUsec = frame->timestampUsec;
    mframe->duration = frame->duration;
#endif

    ::memcpy(buf + readSz + 4, frame->frameData, frame->frameLen);
	readSz += frame->frameLen + 4;
	uint32_t timestampIncrement = m_mediaInfo.audioSamplerate * frame->timestampSec;
	timestampIncrement += (uint32_t) (m_mediaInfo.audioSamplerate *(frame->timestampUsec/1000000.0) + 0.5);
	timestamp = m_timestampBase + timestampIncrement;

	len = readSz;
	RTPPacket rtpPkt(buf, len);
	rtpPkt.SetRtpHeader(m_mediaInfo.audioCodec, true);
	rtpPkt.SetSeqNum(++m_rtpSeq);
	rtpPkt.SetTimeStamp(timestamp);
	rtpPkt.SetSSRC(m_ssrc);

	bool getNext = true;
	do {
		theErr = m_rtspClient->SendInterleavedWrite(0, len, buf, &getNext);
		
		if (theErr != ET_NoErr)
		{
			if ((theErr == EINPROGRESS) || (theErr == EAGAIN))
			{
				m_socket->GetSocket()->RequestEvent(EV_WR);
				//printf("lost fream"); 				
				theErr = ET_NoErr;
			}
			break;
		}
		else
		{
			m_pusherState = PUSHER_STATE_PUSHING;
			if (m_callbackFunc != NULL) 
				m_callbackFunc(m_pusherState, 0, m_cbParam);
		}
	}while (!getNext);

	if (theErr != ET_NoErr)
	{
	    m_pusherState = PUSHER_STATE_ERROR;
		//close socket        
		delete m_socket; m_socket = NULL;
		if (m_callbackFunc != NULL)
		{
		    int status = m_rtspClient->GetStatus();
			if (status == 200) status = errno;
			m_callbackFunc(m_pusherState, status, m_cbParam);
		}
	}

	return ET_NoErr;
}

PusherHandler::PusherHandler()
	: m_callbackFunc(NULL), m_cbParam(NULL), m_tid(0), m_rtspClient(NULL),
	m_socket(NULL), m_connType(RTP_OVER_TCP), m_sdp(NULL), 
	m_state(kSendingOptions), m_rtpSeq(0), 
	m_pusherState(PUSHER_STATE_CONNECTING), m_ssrc(0), m_timestampBase(0)
{
	srand((unsigned)time(NULL));
	m_ssrc = rand();
	m_timestampBase = rand();
}

PusherHandler::~PusherHandler()
{
}

ET_Error PusherHandler::SetupStream()
{
	signal(SIGPIPE, SIG_IGN);
	int theErr = ET_NoErr;
	
	if (m_callbackFunc != NULL)
	{
		m_pusherState = PUSHER_STATE_CONNECTING;
		m_callbackFunc(m_pusherState, 0, m_cbParam);
	}

    bool endLoop = false;
	while ((theErr == ET_NoErr) && (m_state != kDone) && (!endLoop))
	{
		switch(m_state)
		{
			case kSendingOptions:
			{
				theErr = m_rtspClient->SendOptions();
				if (theErr == ET_NoErr)
				{
					if (m_rtspClient->GetStatus() != 200)
					{
						theErr = ENOTCONN;
						break;
					}
					else
					{
						m_state = kSendingAnnounce;
					}
				}
				break;
			}
			case kSendingAnnounce:
			{
				theErr = m_rtspClient->SendAnnounce(m_sdp);
				if (theErr == ET_NoErr)
				{
					if (m_rtspClient->GetStatus() != 200)
					{
						theErr = ENOTCONN;
						break;
					}
					else
					{
						m_state = kSendingSetup;
					}
				}
				break;
			}
			case kSendingSetup:
			{
				if (m_connType == RTP_OVER_TCP)
				{
					theErr = m_rtspClient->SendTCPSetup(1, 0, 1);
				}
					
				if (theErr == ET_NoErr)
				{
					if (m_rtspClient->GetStatus() != 200)
					{
						theErr = ENOTCONN;
						break;
					}
					else
					{
						m_state = kSendingPlay;
					}
				}
				break;
			}
			case kSendingPlay:
			{
				theErr = m_rtspClient->SendPlay(0);
				theErr = ET_NoErr;
				printf("after send play, theErr = %d.\n", theErr);
				if (theErr == ET_NoErr)
				{
					if (m_rtspClient->GetStatus() != 200)
					{
						theErr = ENOTCONN;
						break;
					}
					else
					{
						m_state = kPushing;
						m_pusherState = PUSHER_STATE_CONNECTED;
						if (m_callbackFunc != NULL) 
						m_callbackFunc(m_pusherState, 0, m_cbParam);
					}
				}
				endLoop = true;
				printf("after get send play response, theErr = %d, m_state = %d.\n", theErr, m_state);
				break;
			}
		}

		if ((theErr == EINPROGRESS) || (theErr == EAGAIN))
    	{
			uint32_t em = m_socket->GetEventMask();
			theErr = m_socket->GetSocket()->RequestEvent(em);
			if (theErr != ET_NoErr) break;
    	}
	}

    if (m_state != kPushing)
	{
        m_pusherState = PUSHER_STATE_ERROR;
		//close socket
        delete m_socket; m_socket = NULL;
		if (m_callbackFunc != NULL)
		{
            int status = m_rtspClient->GetStatus();
            if (status == 200) status = errno;
			m_callbackFunc(m_pusherState, status, m_cbParam);
		}
		return -1;
	}
	
    return 0;
}

int PusherHandler::parseDetailRTSPURL(char const* url, char* &username, char* &password,\
		char* address,int* portNum)   
{ 
	  do {  
        // Parse the URL as "rtsp://[<username>[:<password>]@]<server-address-or-name>[:<port>][/<stream-name>]"  
        char const* prefix = "rtsp://";  
        unsigned const prefixLength = (unsigned)strlen(prefix);
  
        unsigned const parseBufferSize = 100;  
          
        char const* from = &url[prefixLength];  
          
        // Check whether "<username>[:<password>]@" occurs next.  
        // We do this by checking whether '@' appears before the end of the URL, or before the first '/'.  
        username = password = NULL; // default return values  
        char const* colonPasswordStart = NULL;  
        char const* p;  
  
        for (p = from; *p != '\0' && *p != '/'; ++p)   
        {  
            if (*p == ':' && colonPasswordStart == NULL)   
            {  
                colonPasswordStart = p;  
            }
            else if (*p == '@')   
            {  
                // We found <username> (and perhaps <password>).  Copy them into newly-allocated result strings:  
                if (colonPasswordStart == NULL) colonPasswordStart = p;  
  
                char const* usernameStart = from;  
                unsigned long usernameLen = colonPasswordStart - usernameStart;
                username = new char[usernameLen + 1] ; // allow for the trailing '\0'  
                  
                for (unsigned i = 0; i < usernameLen; ++i) username[i] = usernameStart[i];  
                username[usernameLen] = '\0';  
  
                char const* passwordStart = colonPasswordStart;  
                if (passwordStart < p) ++passwordStart; // skip over the ':'  
                  
                unsigned long passwordLen = p - passwordStart;
                password = new char[passwordLen + 1]; // allow for the trailing '\0'  
                  
                for (unsigned j = 0; j < passwordLen; ++j) password[j] = passwordStart[j];  
                password[passwordLen] = '\0';  
  
                from = p + 1; // skip over the '@'  
                break;  
            }  
        }  
  
        // Next, parse <server-address-or-name>  
        char* to = &address[0];//parseBuffer[0];  
        unsigned i;  
        for (i = 0; i < parseBufferSize; ++i)   
        {  
            if (*from == '\0' || *from == ':' || *from == '/')   
            {  
                // We've completed parsing the address  
                *to = '\0';  
                break;  
            }  
            *to++ = *from++;  
        }  
        if (i == parseBufferSize) {  
            printf("URL is too long");  
            break;  
        }  
  
        *portNum = 554; // default value  
        char nextChar = *from;  
        if (nextChar == ':') {  
            int portNumInt;  
            if (sscanf(++from, "%d", &portNumInt) != 1)   
            {  
                fprintf(stderr,"No port number follows :%d\n",portNumInt);  
                return -1;  
            }  
            if (portNumInt < 1 || portNumInt > 65535)   
            {  
                fprintf(stderr,"Bad port number\n");  
                return -1;  
            }  
            *portNum = portNumInt;  
        }  
  
        return 0;  
  
    } while (0);  
  
    return -1;  
}


int PusherHandler::generateSDPString(const char* addr, const MediaInfo& mi)
{
	if (m_sdp == NULL)
	{
		m_sdp = new char[1024];
		char encodingName[16] = {0};

		switch(mi.audioCodec)
		{
			case AUDIO_CODEC_G711:
				strcpy(encodingName, "PCMU");
				break;
			case AUDIO_CODEC_MP3:
				strcpy(encodingName, "MPA");
				break;
			case AUDIO_CODEC_IMAADPCM_8K:
			case AUDIO_CODEC_IMAADPCM_16K:
				strcpy(encodingName, "DVI4");
				break;
		}

		sprintf(m_sdp,"v=0\r\n" 
			"o=- 2813265695 2813265695 IN IP4 127.0.0.1\r\n"                                       
			"s=PusherClient\r\n"                                                                
			"i=RTSP PusherNode\r\n"                                                        
			"c=IN IP4 %s\r\n"	// very important 
			"t=0 0\r\n"
			"a=x-qt-text-nam:PusherClient\r\n"
			"a=x-qt-text-inf:RTSP PusherNode\r\n"
			"a=x-qt-text-cmt:source application:PusherClient\r\n"
			"a=x-qt-text-aut:\r\n"
			"a=x-qt-text-cpy:\r\n"
			"m=audio 0 RTP/AVP %d\r\n" // audio attr
			"a=control:trackID=1\r\n"
			"a=rtpmap:%d %s/%d/%d\r\n",
			addr, mi.audioCodec, mi.audioCodec, encodingName,\
			mi.audioSamplerate, mi.audioChannel);		
	}	
	
	return 0;
}
