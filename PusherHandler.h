/**
 * @file PusherHandler.h
 * @brief  1.0
 *
 * @author lizhiyong0804319@gmail.com
 * @version 1.0
 * @date 2015-12-01
 */
#ifndef PUSHER_HANDLER_H
#define PUSHER_HANDLER_H

#include <pthread.h>
#include <stdint.h>
#include <string>
#include "API_PusherTypes.h"

#include "RTSPClient.h"
#include "MsgQueue.h"

class ClientSocket;

class PusherHandler
{
	public:
		static PusherHandler* createNew();
		
		int setCallbackFunc(PusherCallback cb, void* cbParam);
		
		int startStream(const char* url, RTP_ConnectType connType, const char* username, const char* password, \
			int reconn, const MediaInfo& mi);
		int closeStream();

		int pushFrame(MediaFrame* frame);
		
		int release(); 

	protected:
		enum
		{
			kSendingOptions		= 0,
			kSendingAnnounce	= 1,
			kSendingSetup		= 2,
			kSendingPlay		= 3,
			kPushing			= 4,
			kSendingTeardown	= 5,
			kDone				= 6
		};
		
		PusherHandler();
		virtual ~PusherHandler();

		virtual ET_Error SetupStream();
	
	private:
		int parseDetailRTSPURL(char const* url, char* &username, char* &password, \
				char* address,int* portNum);
		
		int generateSDPString(const char* addr, const MediaInfo& mi);

	private:
		PusherCallback m_callbackFunc;
		void* m_cbParam;

		pthread_t m_tid;
		MyDarwin::RTSPClient* m_rtspClient;
		ClientSocket* m_socket;
		RTP_ConnectType m_connType;
		int m_reconn;
		char* m_sdp;

		uint32_t m_state;
		uint16_t m_rtpSeq;
		RTSP_Pusher_State m_pusherState;
		uint32_t m_ssrc;
		uint32_t m_timestampBase;
		MediaInfo m_mediaInfo;

};

#endif
