/**
 * @file API_PusherModule.cpp
 * @brief  
 * @author lizhiyong0804319@gmail.com
 * @version 1.0.0
 * @date 2015-12-01
 */
#include "common.h"
#include "API_PusherModule.h"
#include "PusherHandler.h"
#include "mp3Parser.h"

_API RTSP_Pusher_Handler _APICALL RTSP_Pusher_Create()
{
	return PusherHandler::createNew();
}

_API int _APICALL RTSP_Pusher_SetCallback(RTSP_Pusher_Handler handler,\
		PusherCallback cb, void* cbParam)
{
	PusherHandler* hdr = (PusherHandler*) handler;
	if (hdr == NULL) return -1;
	else return hdr->setCallbackFunc(cb, cbParam);
}


_API int _APICALL RTSP_Pusher_StartStream(RTSP_Pusher_Handler handler, const char* url, \
		RTP_ConnectType connType, const char* username, const char* password, int reconn, \
		 const MediaInfo& mi)
{
	PusherHandler* hdr = (PusherHandler*) handler;
	if (hdr == NULL) return -1;
	else return hdr->startStream(url, connType, username, password, reconn, mi);
}


_API int _APICALL RTSP_Pusher_CloseStream(RTSP_Pusher_Handler handler)
{
	PusherHandler* hdr = (PusherHandler*) handler;
	if (hdr == NULL) return -1;
	else return hdr->closeStream();
}


_API int _APICALL RTSP_Pusher_Release(RTSP_Pusher_Handler handler)
{
	PusherHandler* hdr = (PusherHandler*) handler;
	if (hdr == NULL) return -1;
	else return hdr->release();
}

_API int _APICALL RTSP_Pusher_PushFrame(RTSP_Pusher_Handler handler, MediaFrame* frame)
{
	PusherHandler* hdr = (PusherHandler*) handler;
	if (hdr == NULL) return -1;
	else return hdr->pushFrame(frame);
}

_API double _APICALL RTSP_Pusher_Get_MP3_Frame_Duration(void* frameData)
{    
    FrameParser frmParser;
	frmParser.ParseFrame((unsigned char*)frameData);
	return frmParser.Duration();
}


