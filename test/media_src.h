//
// media_src.h
// interface definition of media stream source
//
// S.Y.W
// 2010/09/06
//

#ifndef __MEDIA_SRC_H__
#define __MEDIA_SRC_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <string>

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_

typedef struct tWAVEFORMATEX
{
    uint16_t   wFormatTag;        /* format type */
    uint16_t   nChannels;         /* number of channels (i.e. mono, stereo...) */
    uint32_t   nSamplesPerSec;    /* sample rate */
    uint32_t   nAvgBytesPerSec;   /* for buffer estimation */
    uint16_t   nBlockAlign;       /* block size of data */
    uint16_t   wBitsPerSample;    /* Number of bits per sample of mono data */
    uint16_t   cbSize;            /* The count in bytes of the size of */
} WAVEFORMATEX;

typedef WAVEFORMATEX       *PWAVEFORMATEX;

#endif //_WAVEFORMATEX_


#define MAX_SESS_TITLE_SIZE	255
#define MAX_APP_DATA_SIZE	255
#define MAX_SP_UIN_SIZE		63
#define MAX_SP_ADDR_SIZE	31

// 一次 INVITE 请求最大目标终端数目
#define MAX_ONCE_INVITE_NUM	100

struct MediaAttr
{
	int priority;
	int category;
	char title[MAX_SESS_TITLE_SIZE + 1];
	char appData[MAX_APP_DATA_SIZE + 1];

	int fmt;
	int freq;
	int bitrate; // kpbs
	int sample_size; // byte
	int channel_num;
};


///////////////////////////////////////////////////////////////////////////////

// current supportted stream type
enum { MS_MPA_File = 1, MS_WAV_File, MS_ALaw_File };
enum { FMT_UNKNOWN = 0, FMT_WAV_PCM = 0x01, FMT_MPA, FMT_WAV_ALAW };

class MediaStream
{
public:
    virtual ~MediaStream()
    {}

    virtual int open(const void *arg, int arg_size) = 0;
    virtual void close() = 0;

    virtual int read_frame(unsigned char *buff, int size) = 0;
    virtual int get_media_attr(MediaAttr *attr) = 0;

    // get current frame size in bytes
    virtual int get_frame_size() = 0;
    // milliseconds
    virtual double get_frame_duration() = 0;

	virtual int get_total_seconds() = 0;
	virtual int get_current_seconds() = 0;
	virtual int set_current_seconds(int v) = 0;

    static MediaStream *getStream(int ms_type);
};


//
// concret stream classes 
//

#define MPA_HEAD_SIZE	    4

class MPAMediaStream : public MediaStream
{
public:
    enum {Version25 = 0, Version20 = 2,	Version10 = 3};
    enum {Layer3 = 1, Layer2 = 2, Layer1 = 3};

    MPAMediaStream();
    ~MPAMediaStream();

    virtual int open(const void *arg, int arg_size);
    virtual void close();

    virtual int read_frame(unsigned char *buff, int size);
    virtual int get_media_attr(MediaAttr *attr);

    virtual int get_frame_size() {
	return _frm_size;
    }

    virtual double get_frame_duration() {
	return _frm_duration;
    }

	virtual int get_total_seconds(){
		return _total_seconds;
	}

	virtual int get_current_seconds(){
		return (int)(_cur_frame * _frm_duration / 1000);
	}

	virtual int set_current_seconds(int v){
		if (v > _total_seconds)
		{
			v = _total_seconds;
		}
		if (v < 0)
		{
			v = 0;
		}
		_cur_frame = (int)(v * 1000 / _frm_duration);
		int offset = _cur_frame * get_frame_size();
		fseek(_file, offset, SEEK_SET);
		return v;
	}

protected:
    int parse(FILE *f);
    int parse_head(const unsigned char *head);
    int skip_id3v2_tag(FILE *f, const char *head);
    int seek_frame(FILE *f);

private:
    std::string _filePath;
    FILE *_file;

    // mpa info
    int _cur_frame;
    int _frm_size;
    double _frm_duration;
	int _total_seconds;
	int _total_bytes;
	int _total_frames;
	
    int _version;
    int _layer;
    int _freq;
    int _bps;
    int _chnum;    

    char _head[MPA_HEAD_SIZE];
    bool _is_first;
};


class WavMediaStream : public MediaStream
{
public:
    WavMediaStream();
    ~WavMediaStream();

    virtual int open(const void *arg, int arg_size);
    virtual void close();

    virtual int read_frame(unsigned char *buff, int size);
    virtual int get_media_attr(MediaAttr *attr);

    virtual int get_frame_size() {
	return _frameSize;
    }

    virtual double get_frame_duration() {
	return 20.0;
	}
	virtual int get_total_seconds(){
		return 0;
	}

	virtual int get_current_seconds(){
		return 0;
	}
	virtual int set_current_seconds(int v){
		return 0;
	}

private:
    void setup_frame_size();

private:
    FILE *_file;
    int _frameSize;
    WAVEFORMATEX _fmt;
};



///////////////////////////////////////////////////////////////////////////////

#endif // __MEDIA_SRC_H__
