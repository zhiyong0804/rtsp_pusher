//
// media_src.h
// interface definition of media stream source
//
// S.Y.W
// 2010/09/06
//

#include "media_src.h"
#include <assert.h>
#include <errno.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <mmreg.h>
#else
#define WAVE_FORMAT_PCM     1
#define  WAVE_FORMAT_ALAW   0x0006 /* Microsoft Corporation */
#endif


using namespace std;


///////////////////////////////////////////////////////////////////////////////

MediaStream* MediaStream::getStream(int ms_type)
{
    switch (ms_type)
    {
    case MS_MPA_File:
	return new MPAMediaStream();

    case MS_WAV_File:
	return new WavMediaStream();	
    }

    return NULL;
}


//
// MPAMediaStream
// 

MPAMediaStream::MPAMediaStream() : _file(NULL), _cur_frame(0), _frm_size(0), _frm_duration(0.0),
								_total_seconds(0), _total_bytes(0),
								_total_frames(0)
{
}

MPAMediaStream::~MPAMediaStream()
{
    close();
}

int MPAMediaStream::open(const void *arg, int arg_size)
{
    assert(arg != NULL);
    assert(_file == NULL);

    const char *path = (const char *)arg;

    FILE *f = fopen(path, "r+b");
    if ( f == NULL)
    {
	printf("open '%s' failed, err=%d\n", path, errno);
	return -1;
    }
	fseek(f, 0, SEEK_END);
	_total_bytes = ftell(f);
	fseek(f, 0, SEEK_SET);

    if (parse(f) != 0)
    {
	fclose(f);
	return -1;
    }

    _file = f;
    _is_first = true;

	_total_frames = _total_bytes / _frm_size;
	_total_seconds = (int)(_frm_duration * _total_frames / 1000);
	_cur_frame = 0;

    return 0;
}

void MPAMediaStream::close()
{
    if (_file != NULL)
    {
	fclose(_file);
	_file = NULL;
    }
}

int MPAMediaStream::read_frame(unsigned char *buff, int size)
{
    assert(_file != NULL);    
    if ( !_is_first )
    {
	if (seek_frame(_file) != 0)
	    return -1;	
    }
    
    if (size < _frm_size)
	return -2;

    memcpy(buff, _head, MPA_HEAD_SIZE);
    int data_size = _frm_size - MPA_HEAD_SIZE;
    int ret = fread(buff + MPA_HEAD_SIZE, 1, data_size, _file);
    if (ret != data_size)
	return -1;
    

	_cur_frame++;
    _is_first = false;
    return _frm_size;
}

int MPAMediaStream::get_media_attr(MediaAttr *attr)
{
    assert(_file != NULL);
    attr->fmt = FMT_MPA;
    attr->bitrate = _bps;
    attr->channel_num = _chnum;
    attr->freq = _freq;
    attr->sample_size = 2;

    return 0;
}

#define ID3V2_FIX_HEAD_SIZE 10

int MPAMediaStream::parse(FILE *f)
{
    // check ID3v2 Header
    char buff[ID3V2_FIX_HEAD_SIZE] = {0};
    int ret = fread(buff, 1, ID3V2_FIX_HEAD_SIZE, f);
    if (ret != ID3V2_FIX_HEAD_SIZE)
    {
	printf("%s\n", "invalid mp3 file, too small");
	return -1;
    }

    if (strncmp(buff, "ID3", 3) == 0)
    {
	skip_id3v2_tag(f, buff);
    }
    else
    {
	fseek(f, 0, SEEK_SET);
    }

    // get the first frame
    return seek_frame(f);
}

/*
struct MPAFrameHead
{
    unsigned int sync:11;		// 同步信息
    unsigned int version:2;		// 版本
    unsigned int layer:2;		// 层
    unsigned int protection:1;		// CRC校验
    unsigned int bitrate:4;		// 位率
    unsigned int frequency:2;		// 频率
    unsigned int padding:1;		// 帧长调节
    unsigned int private_bit:1;		// 保留字
    unsigned int mode:2;		// 声道模式
    unsigned int mode_ext:2;		// 扩充模式
    unsigned int copyright:1;		// 版权
    unsigned int original:1;		// 原版标志
    unsigned int emphasis:2;		// 强调模式
};
*/

int MPAMediaStream::parse_head(const unsigned char *h)
{
    static int bitrates[2][3][16] = 
    {
	{ // MPEG 2 & 2.5
	    {0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,0}, // Layer III
	    {0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,0}, // Layer II
	    {0, 32, 48, 56, 64, 80, 96,112,128,144,160,176,192,224,256,0}  // Layer I
	},
	{ // MPEG 1
	    {0, 32, 40, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,0}, // Layer III
	    {0, 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384,0}, // Layer II
	    {0, 32, 64, 96,128,160,192,224,256,288,320,352,384,416,448,0}  // Layer I
	}
    };

    static int frequecies[3][4] =
    {    
	{44100, 48000, 32000, 0},	// MPEG 1
	{22050, 24000, 16000, 0},	// MPEG 2
	{11025, 12000, 8000,  0}	// MPEG 2.5	
    };

    static double frameduration[3][4] =
    {
	{26.12245f, 24.0f, 36.0f, 0},	// Layer3
	{26.12245f, 24.0f, 36.0f, 0},	// Layer2		
	{8.707483f,  8.0f, 12.0f, 0}	// Layer1
    };

    if (h[0] != 0xFF || (h[1] & 0xE0) != 0xE0) // SYNC TAG
	return -1;

    int hv = (h[1] & 0x18) >> 3;
    int hl = (h[1] & 0x06) >> 1;
    int hb = (h[2] & 0xF0) >> 4;
    int hf = (h[2] & 0x0C) >> 2;
    int hp = (h[2] & 0x02) >> 1;
    int hm = (h[3] & 0xC0) >> 6;

    if (hv == 1 || hl == 0 || hf == 3 ||
	hb == 0 || hb == 15)
    {
	return -1;
    }

    _version = hv;    
    _layer = hl;

    if (_version == Version10)
    {
	_bps = bitrates[1][hl - Layer3][hb];
    }
    else
    {
	_bps = bitrates[0][hl - Layer3][hb];
    }

    switch (_version)
    {
    case Version10:
	_freq = frequecies[0][hf];
	break;

    case Version20:
	_freq = frequecies[1][hf];
	break;

    case Version25:
	_freq = frequecies[2][hf];
	break;
    }

    _frm_duration = frameduration[hl - Layer3][hf];
    if (hl == Layer1)
    {
	_frm_size = (1200 * _bps / _freq + hp) * 4;
    }
    else
    {
	if (_version == Version10)
	    _frm_size = 144000 * _bps / _freq + hp;
	else if (_layer == Layer3)
	    _frm_size = 144000 * _bps / _freq / 2 + hp;
	else
	    _frm_size = 144000 * _bps / _freq + hp;
    }

    if (hm == 3)
	_chnum = 1;
    else
	_chnum = 2;

    return 0;
}

int MPAMediaStream::skip_id3v2_tag(FILE *f, const char *head)
{
    const char *p = head + 6;
    int len = p[3];
    int b;

    b = (p[2] << 7);
    len |= b;

    b = (p[1] << 15);
    len |= b;

    b = (p[0] << 23);
    len |= b;

    int ret = fseek(f, len, SEEK_CUR);
    if (ret != 0)
	return -1;

    return 0;
}

int MPAMediaStream::seek_frame(FILE *f)
{
    const int max_try_num = 2048;
    unsigned char cur_head[MPA_HEAD_SIZE] = {0};

    int ret = fread(cur_head, sizeof(cur_head), 1, f);
    if (ret != 1)
	return -1;

    for (int i = 0; i < max_try_num; ++i)
    {
	unsigned long v = (cur_head[3] << 24) | (cur_head[2] << 16) | 
	    (cur_head[1] << 8) | cur_head[0];
	
	ret = parse_head(cur_head);
	if (ret == 0)
	{
	    memcpy(_head, cur_head, sizeof(cur_head));
	    return 0;
	}

	if (strncmp((char*)cur_head, "TAG", 3) == 0)
	{
	    // ID3v1 tag, end file
	    return -1;
	}

	size_t j = 1;
	for (; j < sizeof(cur_head); ++j)
	{
	    if (cur_head[j] == 0xFF && (cur_head[j+1] & 0xE0) == 0xE0)
	    {
		break ;
	    }
	}

	if (j < sizeof(cur_head))
	{
	    memmove(cur_head, cur_head + j, sizeof(cur_head) - j);
	}

	ret = fread(cur_head + sizeof(cur_head) - j, 1, j, f);
	if (ret != (int)j)
	{
	    return -1;
	}
    }

    return -1;
}


//
// WavMediaStream
//

WavMediaStream::WavMediaStream() : _file(NULL), _frameSize(0)
{
    memset(&_fmt, 0, sizeof(_fmt));
}

WavMediaStream::~WavMediaStream()
{
    close();
}

int WavMediaStream::open(const void *arg, int arg_size)
{
    FILE *f = fopen((const char*)arg, "rb");
    if (f == NULL)
	return -1;

    char chunk_tag[5] = {0};
    int chunk_size = 0;

    int ret = fread(chunk_tag, 1, 4, f);
    if (strcmp(chunk_tag, "RIFF") != 0)
    {
	printf("%s\n", "invalid wave file header: RIFF absent");
	goto FAIL;
    }

    ret = fread(&chunk_size, 4, 1, f);
    if (ret != 1)
    {
	printf("%s\n", "invalid wav file header: wrong file size");
	goto FAIL;
    }

    ret = fread(chunk_tag, 1, 4, f);
    if (strcmp(chunk_tag, "WAVE") != 0)
    {
	printf("%s\n", "invalid wav file header: wrong format");
	goto FAIL;
    }

    for (;;)
    {
	ret = fread(chunk_tag, 1, 4, f);
	if (ret != 4)
	{
	    printf("%s\n", "invalid wav file header: data chunk not found");
	    goto FAIL;
	}

	ret = fread(&chunk_size, 4, 1, f);
	if (ret != 1)
	{
	    printf("invalid wav file header: wrong chunk '%s'\n", chunk_tag);
	    goto FAIL;
	}

	if (strcmp(chunk_tag, "fmt ") == 0)
	{
	    // read wave format
	    if (chunk_size > (int)sizeof(_fmt))
	    {
		printf("%s\n", "invalid wav file: wrong wav format size");
		goto FAIL;
	    }

	    fread(&_fmt, chunk_size, 1, f);
	    if (_fmt.wFormatTag != WAVE_FORMAT_PCM && _fmt.wFormatTag != WAVE_FORMAT_ALAW)
	    {
		printf("invalid wav file: unsupported format tag: %d\n", _fmt.wFormatTag);
		goto FAIL;
	    }
	}
	else if (strcmp(chunk_tag, "data") == 0)
	{
	    break;
	}
	else
	{
	    fseek(f, chunk_size, SEEK_CUR);
	}
    }

    setup_frame_size();
    goto SUCC;

FAIL:
    fclose(f);
    return -1;

SUCC:
    _file = f;
    return 0;
}

void WavMediaStream::close()
{
    if (_file != NULL)
    {
	fclose(_file);
	_file = NULL;
    }
}

int WavMediaStream::read_frame(unsigned char *buff, int size)
{
    if (size < _frameSize)
	return -1;

    int ret = fread(buff, 1, _frameSize, _file);
    if (ret < _frameSize)
    {
	return -1;
    }

    return _frameSize;
}

int WavMediaStream::get_media_attr(MediaAttr *attr)
{
    switch (_fmt.wFormatTag)
    {
    case WAVE_FORMAT_PCM:
	attr->fmt = FMT_WAV_PCM;
	break;

    case WAVE_FORMAT_ALAW:
	attr->fmt = FMT_WAV_ALAW;
	break;
    }

    attr->freq = _fmt.nSamplesPerSec;
    attr->channel_num = _fmt.nChannels;
    attr->sample_size = _fmt.wBitsPerSample;
    
    return 0;
}

void WavMediaStream::setup_frame_size()
{
    _frameSize = (int)(_fmt.nAvgBytesPerSec *  get_frame_duration() / 1000.0);
}
