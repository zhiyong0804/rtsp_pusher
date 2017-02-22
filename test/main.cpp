#include "API_PusherModule.h"

#include <stdio.h>
#include <unistd.h>
#include "media_src.h"
#include "mp3Parser.h"
#include <string>
#include <dirent.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/select.h>
#include "timer.h"

#include <BasicUsageEnvironment/include/UsageEnvironment.hh>
#include <BasicUsageEnvironment/include/BasicUsageEnvironment.hh>


#define PUSHER_DEBUG 1

bool g_running = false;
sem_t g_sem;

static RTSP_Pusher_Handler g_pusher_handler = NULL;
static MediaStream* g_media_stream = NULL;
unsigned int g_timestampSec = 0;
unsigned int g_timestampUsec = 0;
unsigned int g_frameIndex = 0;
unsigned long long g_lastFrameTimestamp = 0UL;
#define MAX_NUMBER_FRAME_ADJUST  10
int last10FrameSpendTimes[MAX_NUMBER_FRAME_ADJUST] = {0};
int g_deltaDelayTime = 0;

TaskScheduler* g_scheduler;
UsageEnvironment* g_env;
char volatile g_endEventLoop = 0;


static void SigHandle(int signo)
{
	switch(signo) {
		case SIGINT:
		case SIGTERM:
			g_running = false;
			printf("got SIGTERM sigal.\n");
			break;
		default:
			break;
	}
}

static int PusherStateCallbackFunc(RTSP_Pusher_State state, int rtspStatusCode, void* obj)
{
	char cnt = 0;
	switch (state)
	{
		case PUSHER_STATE_CONNECTING:
			printf("connecting ...\n");
			break;
		case PUSHER_STATE_CONNECTED:
			printf("connected .\n");
			g_running = true;
			break;
		case PUSHER_STATE_CONNECT_FAILED:
			printf("connect failed : %d!\n", rtspStatusCode);
			g_running = false;
			break;
		case PUSHER_STATE_CONNECT_ABORT:
			printf("connect abort !\n");
			g_running = false;
			break;
		case PUSHER_STATE_PUSHING:
			{
				//(cnt++ % 2) ? printf("+\r") : printf("*\r");
			}
			break;
		case PUSHER_STATE_DISCONNECTED:
		{
			printf("disconnected .\n");
			break;
		}
		case PUSHER_STATE_ERROR:
			printf("occur an error .\n");
			g_running = false;
			break;
	}
	if (state != PUSHER_STATE_CONNECTING && state != PUSHER_STATE_PUSHING && state != PUSHER_STATE_DISCONNECTED)
	{
		sem_post(&g_sem);
	}
    return 0;
}

void PushStreamTask(void* arg)
{
    MediaFrame mframe;
	//FrameParser frmParser;
	double duration = 0.0;
    unsigned char buf[1400] = {0};

    int frameLen = 0;
	int delaySendTime = 0;
	int frameduration = 0;
	bool sendnext = false;

sendNextFrame :

	if ((g_media_stream != NULL) &&  (frameLen = g_media_stream->read_frame(buf, sizeof(buf))) <= 0)
    {
        g_endEventLoop = 1;
		return;
    }
	else
    {
		//frmParser.ParseFrame(buf);
		double duration = RTSP_Pusher_Get_MP3_Frame_Duration(buf); //frmParser.Duration();
		g_timestampUsec += (duration * 1000);
		g_timestampSec += g_timestampUsec / 1000000;
		g_timestampUsec %= 1000000;

		mframe.frameLen = frameLen;
		mframe.frameData = buf;
        mframe.duration = duration;

		mframe.timestampSec = g_timestampSec;
		mframe.timestampUsec = g_timestampUsec;
        g_frameIndex++;

        if (0 == RTSP_Pusher_PushFrame(g_pusher_handler, &mframe))
		{
		    delaySendTime = static_cast<int>(duration * 1000);
			frameduration = delaySendTime;
			struct timeval tv;
            gettimeofday(&tv, NULL);
			unsigned long long currentTimeStamp = tv.tv_sec*1000000+tv.tv_usec;

			if ((g_lastFrameTimestamp != 0UL) && (currentTimeStamp - g_lastFrameTimestamp >= 2*delaySendTime))
			{
			    int throwFrameNumbers = (currentTimeStamp - g_lastFrameTimestamp - delaySendTime) / delaySendTime;
                while(throwFrameNumbers > 0)
				{
				    g_media_stream->read_frame(buf, sizeof(buf));
					printf("throw away throwFrameNumbers frames.\n");
					throwFrameNumbers --;
				}
			}
			
			gettimeofday(&tv, NULL);
			currentTimeStamp = tv.tv_sec*1000000+tv.tv_usec;
			
			printf("current is %llu, lasttimestamp is %llu, duration is %llu.\n", 
				currentTimeStamp, g_lastFrameTimestamp, currentTimeStamp - g_lastFrameTimestamp);
			if ((g_lastFrameTimestamp != 0UL) && (!sendnext))
			{
			    if (currentTimeStamp - g_lastFrameTimestamp > frameduration)
				{
					g_deltaDelayTime += (currentTimeStamp - g_lastFrameTimestamp - frameduration);
				}
				else
				{
					g_deltaDelayTime -= (frameduration - (currentTimeStamp - g_lastFrameTimestamp));
				}

                /* 提前200us预判传输的误差是不是已经超过一帧时长，如果是的则立即发下一帧 */
				if (g_deltaDelayTime + 200 > frameduration)  
				{
                    g_deltaDelayTime -= frameduration;
					sendnext = true;
					printf("called send next frame.\n");
					goto sendNextFrame;
				}
			}

            if (g_lastFrameTimestamp != 0UL)
			{
			    if (currentTimeStamp - g_lastFrameTimestamp > frameduration)
			    {
                    delaySendTime = frameduration - (currentTimeStamp - g_lastFrameTimestamp - frameduration);
			    }
			    else
			    {
                    delaySendTime = frameduration;
			    }
			}
			else
			{
                delaySendTime = frameduration;
			}

			g_lastFrameTimestamp = currentTimeStamp;
		}
	}

	g_env->taskScheduler().scheduleDelayedTask(delaySendTime, (TaskFunc*) PushStreamTask, arg);
}

int main(int argc, char * argv[]) 
{
	int ret = 0;

	if (argc < 4)
	{
		printf("usage: ./PusherModuleTest <server> <session> <dir>\n");
		return ret;
	}
	
	DIR *dir;
	struct dirent *ptr;
	char base[1000];
			  
    if ((dir=opendir(argv[3])) == NULL)
	{
	    printf("open dir error... \n");
	    return ret;
	}

	signal(SIGINT, SigHandle);
	signal(SIGTERM, SigHandle);
	sem_init(&g_sem, 0, 0);

	g_pusher_handler = RTSP_Pusher_Create();

	RTSP_Pusher_SetCallback(g_pusher_handler, PusherStateCallbackFunc, g_pusher_handler);

	MediaInfo mi;
	mi.audioChannel = 2;
	mi.audioCodec = AUDIO_CODEC_MP3;
	mi.audioSamplerate = 44100;
	
	char url[128] = {0};
	sprintf(url, "rtsp://%s/%s.sdp", argv[1], argv[2]);
	ret = RTSP_Pusher_StartStream(g_pusher_handler, url, RTP_OVER_TCP, "1", 0, 1, mi);
	
	while ((sem_wait(&g_sem) != 0));

    g_scheduler = BasicTaskScheduler::createNew();
	g_env = BasicUsageEnvironment::createNew(*g_scheduler);
    g_lastFrameTimestamp = 0UL;            
	g_endEventLoop = 0;
	
	while (g_running)
	{
		if ((ptr=readdir(dir)) == NULL) break;
		if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0 || ptr->d_type != 8) continue;

		g_media_stream = MediaStream::getStream(MS_MPA_File);
		
		std::string file = argv[3];
		file += "/";
		file +=  ptr->d_name;

		ret = g_media_stream->open(file.c_str(), file.length());
		if (ret < 0) break;

        g_endEventLoop = 0;
		g_lastFrameTimestamp = 0UL; 
		g_deltaDelayTime = 0;
		g_env->taskScheduler().scheduleDelayedTask(26000, (TaskFunc*) PushStreamTask, NULL);
	    g_env->taskScheduler().doEventLoop(&g_endEventLoop);
    }

    //getchar();
	 
    ret = RTSP_Pusher_CloseStream(g_pusher_handler);
	RTSP_Pusher_Release(g_pusher_handler);
	closedir(dir);
	return 0;
}
