#ifndef SAMPLE_BUFFER_H_
#define SAMPLE_BUFFER_H_

#include <Windows.h>
//struct WAVEFORMATEX;

class AudioBuffer
{
public:
	AudioBuffer();
	AudioBuffer(int defaultOutputMaxSize);
	~AudioBuffer();

	void PushBuffer(const BYTE *pData,int nBytes);
	int GetBufferFront(BYTE *pData,int nBytes);
	int GetBufferBack(BYTE *pData,int nBytes);

	int GetNewDataSize();
	int GetCapacity();
	void ClearNewData();

	void SetAudioInfo(int nBlockAlign,int nChannels,int nBitsPerSample,bool isFloat = false);
	
	//Audio Info
	int m_nBlockAlign;
	int m_nChannels;
	int m_nBitsPerSample;
	bool m_bIsFloat;
	bool m_bIsSetAudioInfo;

	void PrintBuffer();

private:
	HANDLE m_hDataMutex;
	union
    {
		BYTE *pByte;
		INT64 *pi64;
    } m_pBuffer;
	


    //following varialbe are counted in byte.
	int m_nBufferSize;
	int m_nBuffer;
    int m_iBuffer;
	int m_nBufferNewData;
	int m_nBufferExtraCapacity;

	void EnsureCapacity(int capacityRequirementInBytes);
	void GetBuffer(int iStartIdx,int nCopyedSize,BYTE *pData,int nBytes);
};

#endif