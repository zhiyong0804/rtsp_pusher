/**
 * @file common.h
 * @brief  common ref header file
 * @author lizhiyong0804319@gmail.com
 * @version 1.0
 * @date 2015-12-03
 */

#ifndef COMMON_H
#define COMMON_H

#ifdef _WIN32

//#pragma comment(lib, "Ws2_32.lib")
#include <WinSock2.h>
#include <windows.h>
//#pragma warning(disable:4996)
typedef SOCKET socket_t;
#define SOCK_ADDR_IP(sa) (sa).sin_addr.S_un.S_addr
typedef int socklen_t;

#else

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>

typedef int socket_t;
#define SOCK_ADDR_IP(sa) (sa).sin_addr.s_addr

#include <signal.h>
#include <errno.h>
#define closesocket close

#endif

#ifndef NDEBUG
#define NDEBUG
#endif

#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFF
#endif

#include <assert.h>

typedef  int ET_Error;

enum 
{
	ET_NoErr = 0,
	ET_NotEnoughSpace		=	-1,
	ET_BadURLFormat			=	-2,
	ET_NotInPushingState	=	-3,
	ET_NotConn				=	-4,
	ET_NoData				=	-5,
	ET_NETTIMEOUT			=	-10,
	ET_NETERROR				=	-11
};

#include "MyLog.h"

#endif
