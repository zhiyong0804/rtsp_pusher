#ifndef _MY_LOG_HH
#define _MY_LOG_HH

#ifdef ANDROID

#include <jni.h>
#include <android/log.h>
#include <stdio.h>
	#define ANDROID_LOG_INFO(info)  __android_log_write(ANDROID_LOG_INFO,"android_stream",info)

#else
	#define ANDROID_LOG_INFO(info)
#endif

#endif
