#include "AudioBuffer.h"
#include <string>
#include <assert.h>

AudioBuffer::AudioBuffer(int defaultOutputMaxSize)
{
	m_hDataMutex = CreateMutex(NULL,false,NULL);
	m_pBuffer.pi64 = NULL;
	m_nBufferSize = 0;
	m_nBuffer = 0;
    m_iBuffer = 0;
	m_nBufferNewData = 0;
	m_nBufferExtraCapacity = 4;

	m_nSamplesPerSec = m_nBlockAlign = m_nChannels = m_nBitsPerSample = 0;
	m_bIsFloat = m_bIsSetAudioInfo = false;

	EnsureCapacity(defaultOutputMaxSize);
}

AudioBuffer::AudioBuffer()
{
	m_hDataMutex = CreateMutex(NULL,false,NULL);
	m_pBuffer.pi64 = NULL;
	m_nBufferSize = 0;
	m_nBuffer = 0;
    m_iBuffer = 0;
	m_nBufferNewData = 0;
	m_nBufferExtraCapacity = 4;

	m_nSamplesPerSec = m_nBlockAlign = m_nChannels = m_nBitsPerSample = 0;
	m_bIsFloat = m_bIsSetAudioInfo = false;
}

AudioBuffer::~AudioBuffer()
{
	CloseHandle(m_hDataMutex);
	if (m_pBuffer.pi64)
		delete [] m_pBuffer.pi64;
}


void AudioBuffer::EnsureCapacity(int capacityRequirementInBytes)
{
	int nNewSizeInInt64,nNewSizeInByte;
	nNewSizeInInt64 = (capacityRequirementInBytes/sizeof(INT64) + 1)*m_nBufferExtraCapacity;
	nNewSizeInByte = nNewSizeInInt64*sizeof(INT64);

    if (nNewSizeInByte > m_nBufferSize) 
    {
		INT64* pNewBuffer;
		pNewBuffer = new INT64[nNewSizeInInt64];

#ifdef _DEBUG
		memset(pNewBuffer,0,sizeof(INT64)*nNewSizeInInt64);
#endif

		if (m_pBuffer.pi64)
		{
			if (m_nBuffer>0)
			{
				if (m_nBuffer<m_nBufferSize)
				{
					assert(m_iBuffer == m_nBuffer);
					memcpy(pNewBuffer,m_pBuffer.pByte,sizeof(BYTE)*m_nBuffer);
				}
				else
				{
					memcpy((BYTE*)pNewBuffer,m_pBuffer.pByte+m_iBuffer,sizeof(BYTE)*(m_nBuffer-m_iBuffer));
					memcpy((BYTE*)pNewBuffer+(m_nBuffer-m_iBuffer),m_pBuffer.pByte,sizeof(BYTE)*(m_iBuffer));
					m_iBuffer = m_nBuffer;
				}
			}
			delete [] m_pBuffer.pi64;
		}
		m_pBuffer.pi64 = pNewBuffer;
		m_nBufferSize = nNewSizeInByte;
    } 
}

void AudioBuffer::PushBuffer(const BYTE *pData,int nBytes)
{
	WaitForSingleObject(m_hDataMutex, INFINITE);
	EnsureCapacity(nBytes);
	if (m_iBuffer+nBytes>m_nBufferSize)
	{
		memcpy(m_pBuffer.pByte+m_iBuffer,pData,sizeof(BYTE)*(m_nBufferSize-m_iBuffer));
		memcpy(m_pBuffer.pByte,pData+(m_nBufferSize-m_iBuffer),sizeof(BYTE)*(nBytes - (m_nBufferSize-m_iBuffer)));
		m_nBuffer = m_nBufferSize;
		m_iBuffer = nBytes - (m_nBufferSize-m_iBuffer);
	}
	else
	{
		memcpy(m_pBuffer.pByte+m_iBuffer,pData,sizeof(BYTE)*(nBytes));
		if (m_nBuffer<m_nBufferSize)
			m_nBuffer+=nBytes;
		m_iBuffer+=nBytes;
		if (m_iBuffer>=m_nBufferSize)
			m_iBuffer-=m_nBufferSize;
	}
	m_nBufferNewData += nBytes;
	if (m_nBufferNewData > m_nBufferSize)
		m_nBufferNewData = m_nBufferSize;
	ReleaseMutex(m_hDataMutex);
}

void AudioBuffer::GetBuffer(int iStartIdx,int nCopyedSize,BYTE *pData,int nBytes)
{
	assert(nCopyedSize<=nBytes);
	if (iStartIdx+nCopyedSize>m_nBufferSize)
	{
		memcpy(pData,m_pBuffer.pByte+iStartIdx,sizeof(BYTE)*(m_nBufferSize-iStartIdx));
		memcpy(pData+(m_nBufferSize-iStartIdx),m_pBuffer.pByte,sizeof(BYTE)*(nCopyedSize - (m_nBufferSize-iStartIdx)));
	}
	else
	{
		memcpy(pData,m_pBuffer.pByte+iStartIdx,sizeof(BYTE)*(nCopyedSize));
	}

}

int AudioBuffer::GetBufferFront(BYTE *pData,int nBytes)
{
	int nCopyedSize,iStartIdx;
	WaitForSingleObject(m_hDataMutex, INFINITE);

	EnsureCapacity(nBytes);

	nCopyedSize = min(nBytes,m_nBuffer);
	//if (m_nBuffer<m_nBufferSize)
	//	iStartIdx=0;
	//else
	//	iStartIdx = m_iBuffer;

	iStartIdx = m_iBuffer-max(m_nBufferNewData,nCopyedSize);
	if (iStartIdx<0)
		iStartIdx+=m_nBufferSize;
	GetBuffer(iStartIdx,nCopyedSize,pData,nBytes);
	ReleaseMutex(m_hDataMutex);

	if (m_nBufferNewData<nCopyedSize)
		m_nBufferNewData=0;
	else
		m_nBufferNewData-=nCopyedSize;


	return nCopyedSize;
}

int AudioBuffer::GetBufferBack(BYTE *pData,int nBytes)
{
	int nCopyedSize,iStartIdx;
	WaitForSingleObject(m_hDataMutex, INFINITE);

	EnsureCapacity(nBytes);

	nCopyedSize = min(nBytes,m_nBuffer);
	iStartIdx = m_iBuffer-nCopyedSize;
	if (iStartIdx<0)
		iStartIdx+=m_nBufferSize;
	GetBuffer(iStartIdx,nCopyedSize,pData,nBytes);

	ReleaseMutex(m_hDataMutex);

	m_nBufferNewData=0;
	return nCopyedSize;
}

int AudioBuffer::GetNewDataSize()
{
	int res;
	WaitForSingleObject(m_hDataMutex, INFINITE);
	res = m_nBufferNewData;
	ReleaseMutex(m_hDataMutex);
	return res;
}

int AudioBuffer::GetCapacity()
{
	return m_nBufferSize;
}

void AudioBuffer::ClearNewData()
{
	WaitForSingleObject(m_hDataMutex, INFINITE);
	m_nBufferNewData=0;
	ReleaseMutex(m_hDataMutex);
}

void AudioBuffer::SetAudioInfo(int nSamplesPerSec,int nBlockAlign,int nChannels,int nBitsPerSample,bool isFloat)
{
	m_nSamplesPerSec = nSamplesPerSec;
	m_nBlockAlign = nBlockAlign;
	m_nChannels = nChannels;
	m_nBitsPerSample = nBitsPerSample;
	m_bIsFloat = isFloat;
	m_bIsSetAudioInfo = true;
}

//void AudioBuffer::SetWaveInfo(LPCWAVEFORMATEX pInfo)
//{
//	if (pWaveInfo==NULL)
//		pWaveInfo = new WAVEFORMATEX;
//	memcpy(pWaveInfo,pInfo,sizeof(WAVEFORMATEX));
//}
//
//LPCWAVEFORMATEX AudioBuffer::GetWaveInfo()
//{
//	return pWaveInfo;
//}

void AudioBuffer::PrintBuffer()
{
	printf("B ");
	for (int i=0;i<m_nBufferSize;i++)
	{
		if (i==m_iBuffer && i==m_nBuffer)
			printf("{%2d} ",((unsigned char*)m_pBuffer.pByte)[i]);
		else if (i==m_iBuffer)
			printf("[%2d] ",((unsigned char*)m_pBuffer.pByte)[i]);
		else
			printf(" %2d  ",((unsigned char*)m_pBuffer.pByte)[i]);
	}
	printf("\nNew Data: %d\n",m_nBufferNewData);
		
}


