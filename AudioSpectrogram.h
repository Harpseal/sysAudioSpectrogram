#ifndef AudioSpectrogram_H_
#define AudioSpectrogram_H_

#include <vector>
#include "AudioBuffer.h"
#include "ThreadBase.h"

#define ENABLE_KISSFFT 0
#define ENABLE_FFTW 1

struct _IplImage;
typedef struct _IplImage IplImage;
class AudioFFT;
class AudioSpectrogramWindow;

struct AudioFFTPackage
{
	AudioFFT *pFFT;
	IplImage *pFrqImg;
};

class AudioSpectrogram : public ThreadBase
{
public:
	AudioSpectrogram(int nFFT,AudioBuffer *pBuffer,int width=800,int height=480);
	~AudioSpectrogram();

	static DWORD WINAPI AudioSpectrogramThreadFunction(LPVOID pContext);
	void Start();
	void Update(AudioFFTPackage* pFFT);
	void GenFrqRemap(int nIn,int nOut,bool bLinear);

private:
	AudioBuffer *m_pBuffer;
	int m_nFFT;

	std::vector<AudioFFTPackage> m_vFFT;

	int m_iFrqImgCounter;
	float m_fOverlapRatio;

	float *m_pfFFT2Pitch;
	void GenFFT2Pitch(int nFFT,int nSamplePerSec);
	int* m_piFrqRemap;
	bool m_bIsShiftingSpectrogram;
	
	HANDLE hGlobalCloseEvent;
	union 
	{
		BYTE *p8;
		INT64 *p64;
	} m_pRawDataBuffer;

	AudioSpectrogramWindow *m_pFFTWindow;
	
};

#endif