/**
 * @file API_PusherModule.h
 * @brief  Pusher Module API
 * @author K.D, kofera.deng@gmail.com
 * @version 1.0
 * @date 2015-11-13
 */

#ifndef API_PUSHER_MODULE_H
#define API_PUSHER_MODULE_H

#include "API_PusherTypes.h"

#ifdef __cplusplus
extern "C"
{
#endif 

	/**
	 * @brief  RTSP_Pusher_Create 
	 *		创建推送流句柄
	 * @return  NULL or 推送流句柄 
	 */
	_API RTSP_Pusher_Handler _APICALL RTSP_Pusher_Create();


	/**
	 * @brief  RTSP_Pusher_SetCallback 
	 *		推送流回调函数,　根据推送状态触发
	 * @param handler	推送流句柄
	 * @param cb		注册的回调函数
	 *
	 * @return  返回处理结果 
	 */
	_API int _APICALL RTSP_Pusher_SetCallback(RTSP_Pusher_Handler handler, PusherCallback cb, void* cbParam);


	/**
	 * @brief  RTSP_Pusher_StartStream 
	 *		开始推送流
	 * @param handler	推送流句柄
	 * @param url		推送流RTSP URL
	 * @param connType	RTP 数据推送连接类型: tcp or udp 
	 * @param username  推送授权用户
	 * @param password　授权用户密码
	 * @param reconn　　推送流连接次数(当断开连接或连接失败时), 0:循环连接,
	 *					nonzero 相应连接次数
	 * @param mi		推送流媒体信息
	 *
	 * @return			返回处理结果 
	 */
    _API int _APICALL RTSP_Pusher_StartStream(RTSP_Pusher_Handler handler, const char* url, RTP_ConnectType connType, const char* username, const char* password, int reconn, const MediaInfo& mi);

	/**
	 * @brief  RTSP_Pusher_CloseStream 
	 *		结束推送流
	 * @param handler　推送流句柄
	 *
	 * @return   返回处理结果
	 */
	_API int _APICALL RTSP_Pusher_CloseStream(RTSP_Pusher_Handler handler);

	/**
	 * @brief  RTSP_Pusher_Release 
	 *		推送流数据资源释放
	 * @param handler	推送流句柄
	 *
	 * @return   返回处理结果
	 */
	_API int _APICALL RTSP_Pusher_Release(RTSP_Pusher_Handler handler);


	/**
	 * @brief  RTSP_Pusher_PushFrame 
	 *		推送媒体数据帧
	 * @param handler	推送流句柄
	 * @param frame		媒体数据帧
	 *
	 * @return  返回处理结果 
	 */
	_API int _APICALL RTSP_Pusher_PushFrame(RTSP_Pusher_Handler handler, MediaFrame* frame);

    /**
	 * @brief  RTSP_Pusher_Get_MP3_Frame_Duration 
	 *
	 * @param frameData		媒体数据帧
	 *
	 * @return  返回处理结果
	 */
    _API double _APICALL RTSP_Pusher_Get_MP3_Frame_Duration(void* frameData);

#ifdef __cplusplus
}
#endif 

#endif 
