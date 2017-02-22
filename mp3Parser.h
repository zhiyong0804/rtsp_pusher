/*/////////////////////////////////////////////////////////////////////////

	FILE:		FrameParser.h
	PURPOSE:	Mp3 Frame Parse
	DATE:		2003-12-15
	MODIFY:		2005-08-04

/*/////////////////////////////////////////////////////////////////////////

#ifndef __FRAMEPARSER_H__
#define __FRAMEPARSER_H__
#include <assert.h>
#include <stdio.h>

#ifndef _WIN32
#define DWORD unsigned long 
#endif

#define BYTE unsigned char

// mp3相关类型声明
typedef enum
{
	UnknChannel = -1,
	MonoChannel = 0,
	DualChannel = 1
} EnumChannelMode;

typedef enum
{
	Version25	= 0,
	VerReserved	= 1,
	Version20	= 2,
	Version10	= 3
} EnumMpegVersion;

typedef enum
{
	LayReserved	= 0,
	Layer3		= 1,
	Layer2		= 2,
	Layer1		= 3
} EnumMpegLayer;


//////////////////////////////////////////////////////////////////////////
/**
 *
 *	Mpeg Audio 帧解析
 *
 */
class FrameParser
{
public:
	FrameParser()
	{
		m_eVersion	= VerReserved;
		m_eLayer	= LayReserved;
		m_nBps		= 0;
		m_nSampleRate = 0;
		m_nFrameSize  = 0;
		m_dblDuration = 0.0;
		m_eChannelMode = UnknChannel;		
	}

	// 解析帧
	bool ParseFrame(const BYTE* pHeader);

	// 定位帧头
	int SeekHeader(const BYTE* pData, DWORD nDataSize);

	inline EnumMpegVersion Version()const
	{
		return m_eVersion;
	}
	
	inline EnumMpegLayer Layer()const
	{
		return m_eLayer;
	}

	inline int Bps()const
	{
		return m_nBps;
	}

	inline int SampleRate()const
	{
		return m_nSampleRate;
	}
	
	inline int FrameSize()const
	{
		return m_nFrameSize;
	}

	inline double Duration()const
	{
		return m_dblDuration;
	}
	
	inline EnumChannelMode ChannelMode()const
	{
		return m_eChannelMode;
	}	

protected:	
	int		m_nBps;
	int		m_nSampleRate;
	int		m_nFrameSize;
	double	m_dblDuration;
	EnumMpegVersion m_eVersion;
	EnumMpegLayer	m_eLayer;
	EnumChannelMode m_eChannelMode;
};

inline bool FrameParser::ParseFrame(const BYTE* pHeader)
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
	
	assert(pHeader != NULL);

	long nHeader =	(pHeader[0] << 24) | 
					(pHeader[1] << 16) | 
					(pHeader[2] <<  8) | 
					pHeader[3];
	
	// Check Sync Word
	if ((nHeader & 0xFFE00000) != 0xFFE00000 || // INVALID Sync Work
		(nHeader & 0x0000FC00) == 0x0000FC00 ||	// Reserved bps & frequency
		(nHeader & 0x0000F000) == 0x00000000 )	// Unsoupport free bps
		return false;

	// Check Version
	m_eVersion = (EnumMpegVersion)((nHeader & 0x00180000) >> 19);

	// Check Layer
	m_eLayer = (EnumMpegLayer)((nHeader & 0x00060000) >> 17);
	if (m_eLayer < Layer3) return false;

	// Bitrate
	int i;
	i = (nHeader & 0x0000F000) >> 12;

	if (m_eVersion == Version10)
	{
		m_nBps = bitrates[1][m_eLayer-Layer3][i];
	}
	else
	{
		m_nBps = bitrates[0][m_eLayer-Layer3][i];
	}

	// SampleRate
	i = (nHeader & 0x00000C00) >> 10;
	switch(m_eVersion) 
	{
	case Version10:
		m_nSampleRate = frequecies[0][i];
		break;

	case Version20:
		m_nSampleRate = frequecies[1][i];
		break;

	case Version25:
		m_nSampleRate = frequecies[2][i];
		break;
	default:
		break;
	}

	if (m_nSampleRate == 0 || m_nBps == 0)
		return false;

	// Play Duration
	m_dblDuration = frameduration[m_eLayer-Layer3][i];
	
	// Frame Length
	i = (nHeader & 0x00000200) >> 9;
	if (m_eLayer == Layer1)
	{
		m_nFrameSize = (12000 * m_nBps / m_nSampleRate + i) * 4;
	}
	else 
	{
		if (m_eVersion == Version10)
			m_nFrameSize = 144000 * m_nBps / m_nSampleRate + i;
		else if (m_eLayer == Layer3)
			m_nFrameSize = 144000 * m_nBps / m_nSampleRate / 2 + i;
		else
			m_nFrameSize = 144000 * m_nBps / m_nSampleRate + i;
	}	

	// Channel Mode
	i = (nHeader & 0x000000C0) >> 6;
	m_eChannelMode = (i == 3)?MonoChannel:DualChannel;

	assert(m_nFrameSize > 0);
	return true;
}


//////////////////////////////////////////////////////////////////////////

inline int FrameParser::SeekHeader(const BYTE* pData, DWORD nDataSize)
{
	for (DWORD i = 0; i <= (nDataSize - 4); i++)
	{
		if (ParseFrame(pData+i))
			return i;
	}

	return -1;
}


//////////////////////////////////////////////////////////////////////////


#endif	// __FRAMEPARSER_H__
