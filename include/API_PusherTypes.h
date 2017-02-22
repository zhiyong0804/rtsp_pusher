/**
 * @file TypesDefine.h
 * @brief  types define
 * @author K.D, kofera.deng@gmail.com
 * @version 1.0
 * @date 2015-11-11
 */

#ifndef API_PUSHER_TYPES_H
#define API_PUSHER_TYPES_H

#ifdef _WIN32
#define _API __declspec(dllexport)
#define _APICALL __stdcall
#else
#define _API
#define _APICALL
#endif

#define RTSP_Pusher_Handler void*

enum
{
	MC_NoErr				=	 0,
	MC_NotEnoughSpace		=	-1,
	MC_BadURLFormat			=	-2,
	MC_NotInPushingState	=	-3,
	MC_NotConn				=	-4,	
};
typedef  int MC_Error;

/* 推送的媒体帧数据定义 */
typedef struct MEDIA_FRAME_T
{
	unsigned int       frameLen;				/* 帧数据长度 */
	unsigned char*     frameData;			    /* 帧数据 */
	unsigned int       timestampSec;			/* 时间戳,秒 */
	unsigned int       timestampUsec;			/* 时间戳,微秒 */
    double             duration;                /* frame broadcast duration , millisecond */
} MediaFrame;

#define AUDIO_CODEC_G711			0x00
#define AUDIO_CODEC_MP3				0x0E
#define AUDIO_CODEC_IMAADPCM_8K		0x05
#define AUDIO_CODEC_IMAADPCM_16K	0x06

/* 推送流的媒体属性定义 */
typedef struct MEDIA_INFO_T
{
	unsigned int audioCodec;			/* 音頻編碼类型*/
	unsigned int audioSamplerate;		/* 音頻采样率*/
	unsigned int audioChannel;			/* 音頻通道数*/
} MediaInfo;

/* 推送事件类型定义 */
typedef enum PUSHER_STATE_T
{
    PUSHER_STATE_CONNECTING   =   1,     /* 连接中 */
    PUSHER_STATE_CONNECTED,              /* 连接成功 */
    PUSHER_STATE_CONNECT_FAILED,         /* 连接失败 */
    PUSHER_STATE_CONNECT_ABORT,          /* 连接异常中断 */
    PUSHER_STATE_PUSHING,                /* 推流中 */
    PUSHER_STATE_DISCONNECTED,           /* 断开连接 */
    PUSHER_STATE_ERROR
} RTSP_Pusher_State;

/* 连接类型 */
typedef enum __RTP_CONNECT_TYPE
{
	RTP_OVER_TCP	=	0x01,		/* RTP Over TCP */
	RTP_OVER_UDP					/* RTP Over UDP */
} RTP_ConnectType;


/* 推送回调函数定义 obj 表示用户自定义数据 */
typedef int (*PusherCallback)(RTSP_Pusher_State state, int rtspStatusCode, void *obj);

#endif
