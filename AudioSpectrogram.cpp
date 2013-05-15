#include "AudioSpectrogram.h"
#include <stdio.h>

#include <assert.h>
#include "iCV.h"

#if ENABLE_FFTW
#include "fftw3.h"
#endif

#if ENABLE_KISSFFT
#include <stdint.h>
#define FIXED_POINT 16
#include ".\KissFFT\kiss_fftr.h"
#endif
//#define EnableDB

#include "AudioSpectrogramDebug.cpp"

class AudioFFT
{
public:

	AudioFFT(int nFFT):m_fFrqMax(0),m_fRawMax(0),m_fFrqMaxCur(0),m_fRawMaxCur(0),m_fFFTAvgMS(0),m_nFFT(nFFT),m_iFrqImgCounter(0){}
	~AudioFFT(){}

	int m_nFFT;
	int m_iFrqImgCounter;

	float m_fFrqMax;
	float m_fRawMax;
	float m_fFrqMaxCur;
	float m_fRawMaxCur;

	float m_fFFTAvgMS;

	//virtual double* GetFFTInputBuffer(){assert(false);return NULL;}
	//virtual void SetFFTInputBuffer(int ifft,void* pValue){assert(false);}

	virtual void ShowDebugInfo(){assert(false);}

	virtual void GenFFTForward32f(int nNewData,int nChannels,float* pRawData){assert(false);}
	virtual void GenFFTForward32i(int nNewData,int nChannels,int* pRawData){assert(false);}
	virtual void GenFFTForward16(int nNewData,int nChannels,short* pRawData){assert(false);}
	virtual void GenFFTForward8(int nNewData,int nChannels,unsigned char* pRawData){assert(false);}

	virtual void GenFFTInputBufferShift(int nShift){assert(false);}

	virtual int GetFFTInputBufferSize() = 0;
	//virtual const void* GetFFTOutputBuffer(){assert(false);return NULL;}
	virtual float GetFFTOutputMagnitude(int idx) = 0;

	virtual void GenFFTForward(int nNewData=0) = 0;
	virtual void GenFFTInverse(int nNewData=0){assert(false);}

	virtual const char* GetFFTName() = 0;

	virtual void UpdateSpectrogram(IplImage*pImg,int* pFrqRemap,bool bIsShiftingSpectrogram = true);
};

void AudioFFT::UpdateSpectrogram(IplImage*pImg,int* pFrqRemap,bool bIsShiftingSpectrogram)
{
	int i,n;
	float v,vMax,vMin;
	vMax = 0;vMin=99999999;
	n = m_nFFT/2+1;
	for (i=1; i<n; ++i) {
		//printf("%.4f+%.4f*I%s", fft_out[i][0], fft_out[i][1], 
		//	i == (n-1) ? "\n":",");
		v = GetFFTOutputMagnitude(i);
		//v = abs(fft_out[i][0]);
#ifdef EnableDB
		if (v==0)
			v =0.0001;
		v = (20*log10(v));
#endif

		//printf("%.4f%s", v, 
		//	i == (n-1) ? "\n":",");
		if (vMax<v)
			vMax = v;
		if (vMin>v)
			vMin=v;
	} 

	m_fFrqMaxCur = vMax;
	if (m_fFrqMax<vMax)
		m_fFrqMax = vMax;

	if (m_fFrqMaxCur<0.00001 || m_fRawMaxCur<0.00001)
	{
		if (!bIsShiftingSpectrogram)
		{
			cvLine(pImg,cvPoint(m_iFrqImgCounter,0),cvPoint(m_iFrqImgCounter,pImg->height-1),CV_RGB(128,128,255),1);
			cvNamedWindow(GetFFTName());
			cvShowImage(GetFFTName(),pImg);
		}
		return;
	}


	//if (m_FFTW.fFrqMax == 0)
	//	m_FFTW.fFrqMax = vMax;
	//else
	//	m_FFTW.fFrqMax = m_FFTW.fFrqMax*0.5+ vMax*0.5;
#ifdef EnableDB
	vMin = 0;
#endif
	unsigned char* pImgData;
	pImgData = (unsigned char*)pImg->imageData + m_iFrqImgCounter*pImg->widthStep;
	float scaleColorSum;
	int scaleCount,updateXPos;
	scaleCount = 0;
	scaleColorSum = 0;

	if (bIsShiftingSpectrogram)
		updateXPos = pImg->width-1;
	else
		updateXPos = m_iFrqImgCounter;

	for (int ifft=0,y=0,c=0;ifft<n&&y<pImg->height;ifft++,c++)
	{
		v = GetFFTOutputMagnitude(ifft);

		scaleCount++;
		scaleColorSum+=v;
		if (c>=pFrqRemap[y]-1)
		{
			c=-1;
			if (bIsShiftingSpectrogram)
				memmove((unsigned char*)pImg->imageData + ((pImg->height-1-y)*pImg->width+0)*pImg->nChannels,
						(unsigned char*)pImg->imageData + ((pImg->height-1-y)*pImg->width+1)*pImg->nChannels,
						sizeof(unsigned char)*(pImg->width-1)*pImg->nChannels);
			pImgData = (unsigned char*)pImg->imageData + ((pImg->height-1-y)*pImg->width+updateXPos)*pImg->nChannels;
			v = scaleColorSum/scaleCount;
#ifdef EnableDB
			//if (v==0)
			//	v =0.0001;
			vMin = -70;
			v = (20*log10(v));
			//if (v<-70) v=0;
			//else if (v>=0) v=0;
			//else
			//	v = (70.f+v)/(70)*254.0;
			v=(MIN((v-vMin)/(m_fFrqMax-vMin),1))*254.0;
#else
			v=(MIN((v-vMin)/(m_fFrqMax*0.4-vMin),1))*254.0;//*(m_FFTW.fRawMaxCur/MAX(m_FFTW.fRawMax,0.1));
#endif
			


			pImgData[0] = pImgData[1] = pImgData[2] = v;
			y++;
			scaleCount = 0;
			scaleColorSum = 0;
		}
	}

	//cvLine(pImg,cvPoint(updateXPos,0),cvPoint(updateXPos,4),CV_RGB((float)m_pBuffer->GetNewDataSize()/m_pBuffer->GetCapacity()*254.f,0,0),1);


	if (!bIsShiftingSpectrogram && m_iFrqImgCounter+1<pImg->width)
		cvLine(pImg,cvPoint(m_iFrqImgCounter+1,0),cvPoint(m_iFrqImgCounter+1,pImg->height-1),CV_RGB(255,0,0),1);

	m_iFrqImgCounter++;
	if (m_iFrqImgCounter>=pImg->width)
	{
		m_iFrqImgCounter = 0;
	}

	cvNamedWindow(GetFFTName());
	cvShowImage(GetFFTName(),pImg);
}


#ifdef _BIG_ENDIAN_
    // big-endian CPU, swap bytes in 16 & 32 bit words

    // helper-function to swap byte-order of 32bit integer
    static inline int _swap32(int &dwData)
    {
        dwData = ((dwData >> 24) & 0x000000FF) | 
               ((dwData >> 8)  & 0x0000FF00) | 
               ((dwData << 8)  & 0x00FF0000) | 
               ((dwData << 24) & 0xFF000000);
        return dwData;
    }   

    // helper-function to swap byte-order of 16bit integer
    static inline short _swap16(short &wData)
    {
        wData = ((wData >> 8) & 0x00FF) | 
                ((wData << 8) & 0xFF00);
        return wData;
    }

    // helper-function to swap byte-order of buffer of 16bit integers
    static inline void _swap16Buffer(short *pData, int numWords)
    {
        int i;

        for (i = 0; i < numWords; i ++)
        {
            pData[i] = _swap16(pData[i]);
        }
    }

#else   // BIG_ENDIAN
    // little-endian CPU, WAV file is ok as such

    // dummy helper-function
    static inline int _swap32(int &dwData)
    {
        // do nothing
        return dwData;
    }   

    // dummy helper-function
    static inline short _swap16(short &wData)
    {
        // do nothing
        return wData;
    }

    // dummy helper-function
    static inline void _swap16Buffer(short *pData, int numBytes)
    {
        // do nothing
    }

#endif  // BIG_ENDIAN

#if ENABLE_FFTW
class AudioFFTW : public AudioFFT
{
public:
	AudioFFTW(int nfft);
	~AudioFFTW();

	void GenFFTForward32f(int nNewData,int nChannels,float* pRawData);
	void GenFFTForward32i(int nNewData,int nChannels,int* pRawData);
	void GenFFTForward16(int nNewData,int nChannels,short* pRawData);
	void GenFFTForward8(int nNewData,int nChannels,unsigned char* pRawData);
	void GenFFTInputBufferShift(int nShift);

	int GetFFTInputBufferSize();
	float GetFFTOutputMagnitude(int idx);

	void GenFFTForward(int nNewData=0);

	const char* GetFFTName();

	void ShowDebugInfo();

private:
	double *m_pdFFT_in;
	double *m_pdFFT_in_weighted;
	fftw_complex *m_pcFFT_out;
	fftw_plan m_FFT_plan;
};

AudioFFTW::AudioFFTW(int nfft):AudioFFT(nfft)
{
	m_nFFT = nfft;
	m_pdFFT_in = (double *)fftw_malloc(sizeof(double)*m_nFFT);
	m_pdFFT_in_weighted =  (double *)fftw_malloc(sizeof(double)*m_nFFT);
	memset(m_pdFFT_in,0,sizeof(double)*m_nFFT);
	m_pcFFT_out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex)*(m_nFFT/2+1));
	/* if you are here, you definitely need a plan */
	m_FFT_plan = fftw_plan_dft_r2c_1d(m_nFFT, m_pdFFT_in_weighted, m_pcFFT_out,
		FFTW_ESTIMATE);
}

AudioFFTW::~AudioFFTW()
{
	fftw_free(m_pdFFT_in);
	fftw_free(m_pdFFT_in_weighted);
	fftw_free(m_pcFFT_out);
}

inline void AudioFFTW::ShowDebugInfo()
{
	ShowPCM(m_pdFFT_in,m_nFFT,1,1024,60,"AudioFFTW");
	ShowPCM(m_pdFFT_in_weighted,m_nFFT,1,1024,60,"AudioFFTW_weight");
}

inline void AudioFFTW::GenFFTForward32f(int nNewData,int nChannels,float* pRawData)
{
	int nOldData;
	double value;
	const double conv = 1.0 / 2147483648.0;

	GenFFTInputBufferShift(nNewData);
	m_fRawMaxCur = 0;
	nOldData = m_nFFT - nNewData;

	assert(sizeof(int) == 4);
	for (int i = 0; i < nNewData; i ++)
	{
		value = 0;
		for (int c = 0; c < nChannels; c ++ )
		{
			value += pRawData[i*nChannels+c];
		}
		value/=nChannels;
		value = pRawData[i*nChannels+0];
		m_pdFFT_in[nOldData+i] = value;

		if (m_fRawMaxCur<abs(value))
			m_fRawMaxCur=abs(value);
	}

	if (m_fRawMax<m_fRawMaxCur)
		m_fRawMax=m_fRawMaxCur;	


	GenFFTForward(nNewData);

}

inline void AudioFFTW::GenFFTForward32i(int nNewData,int nChannels,int* pRawData)
{
	int nOldData;
	double value;
	const double conv = 1.0 / 2147483648.0;

	GenFFTInputBufferShift(nNewData);
	m_fRawMaxCur = 0;
	nOldData = m_nFFT - nNewData;

	assert(sizeof(int) == 4);
	for (int i = 0; i < nNewData; i ++)
	{
		value = 0;
		for (int c = 0; c < nChannels; c ++ )
		{
			value += (double)(_swap32(pRawData[i*nChannels+c]) * conv);
		}
		value/=nChannels;
		value = (double)(_swap32(pRawData[i*nChannels+0]) * conv);
		m_pdFFT_in[nOldData+i] = value;

		if (m_fRawMaxCur<abs(value))
			m_fRawMaxCur=abs(value);
	}

	if (m_fRawMax<m_fRawMaxCur)
		m_fRawMax=m_fRawMaxCur;	


	GenFFTForward(nNewData);

}
inline void AudioFFTW::GenFFTForward16(int nNewData,int nChannels,short* pRawData)
{
	int nOldData;
	double value;
	const double conv = 1.0 / 32768.0;

	GenFFTInputBufferShift(nNewData);
	m_fRawMaxCur = 0;
	nOldData = m_nFFT - nNewData;

	for (int i = 0; i < nNewData; i ++)
	{
		value = 0;
		for (int c = 0; c < nChannels; c ++ )
		{
			value += (double)(_swap16(pRawData[i*nChannels+c]) * conv);
		}
		value/=nChannels;
		value = (double)(_swap16(pRawData[i*nChannels+0]) * conv);
		m_pdFFT_in[nOldData+i] = value;
					
		if (m_fRawMaxCur<abs(value))
			m_fRawMaxCur=abs(value);
	}

	if (m_fRawMax<m_fRawMaxCur)
		m_fRawMax=m_fRawMaxCur;

	GenFFTForward(nNewData);
}
inline void AudioFFTW::GenFFTForward8(int nNewData,int nChannels,unsigned char* pRawData)
{
	int nOldData;
	double value;
	const double conv = 1.0 / 128.0;

	GenFFTInputBufferShift(nNewData);
	m_fRawMaxCur = 0;
	nOldData = m_nFFT - nNewData;

	for (int i = 0; i < nNewData; i ++)
	{
		value = 0;
		for (int c = 0; c < nChannels; c ++ )
		{
			value += (double)(pRawData[i*nChannels+c]) * conv - 1.0;
		}
		value/=nChannels;
		value = (double)(pRawData[i*nChannels+0]) * conv - 1.0; 

		m_pdFFT_in[nOldData+i] = value;

		if (m_fRawMaxCur<abs(value))
			m_fRawMaxCur=abs(value);
	}

	if (m_fRawMax<m_fRawMaxCur)
		m_fRawMax=m_fRawMaxCur;

	GenFFTForward(nNewData);
}

inline void AudioFFTW::GenFFTInputBufferShift(int nShift)
{
	if (nShift>0 && m_nFFT-nShift>0)
		memmove(m_pdFFT_in,m_pdFFT_in+nShift,sizeof(double)*(m_nFFT-nShift));
}

inline int AudioFFTW::GetFFTInputBufferSize()
{
	return m_nFFT;
}

inline float AudioFFTW::GetFFTOutputMagnitude(int ifft)
{
	assert(ifft>=0 && ifft<m_nFFT/2+1);
	return sqrt(m_pcFFT_out[ifft][0]*m_pcFFT_out[ifft][0] + m_pcFFT_out[ifft][1]*m_pcFFT_out[ifft][1]);
}

inline void AudioFFTW::GenFFTForward(int nNewData)
{
	double timeMS = cvGetTickCount();
	float weight;
	for (int i=0;i<m_nFFT;i++)
	{
		const float cfCutOffRatio = 0.4;
		weight = 0.5*(1-cos(3.1415926535897932384626433832795*2*double(i)/double(m_nFFT)));
		//weight = double(i)/double(m_nFFT-nNewData);//0.5*(1-cos(3.1415926535897932384626433832795*2*double(i)/double(m_nFFT)));
		//if (weight>1)weight=1;
		//if (weight<cfCutOffRatio) weight=0;
		//else
		//	weight=(weight-cfCutOffRatio)/(1-cfCutOffRatio);
		m_pdFFT_in_weighted[i] = m_pdFFT_in[i]*weight;
	}
	fftw_execute(m_FFT_plan);

	timeMS = (cvGetTickCount()-timeMS)/(cvGetTickFrequency()*1000.0);
	if (m_fFFTAvgMS==0)
		m_fFFTAvgMS = timeMS;
	else
		m_fFFTAvgMS = timeMS*0.1 + m_fFFTAvgMS*0.9;
}

inline const char* AudioFFTW::GetFFTName()
{
	return "FFTW";
}
#endif


#if ENABLE_KISSFFT
class AudioKissFFT : public AudioFFT
{
public:
	AudioKissFFT(int nfft);
	~AudioKissFFT();

	void GenFFTForward32f(int nNewData,int nChannels,float* pRawData);
	void GenFFTForward32i(int nNewData,int nChannels,int* pRawData);
	void GenFFTForward16(int nNewData,int nChannels,short* pRawData);
	void GenFFTForward8(int nNewData,int nChannels,unsigned char* pRawData);
	void GenFFTInputBufferShift(int nShift);

	int GetFFTInputBufferSize();
	float GetFFTOutputMagnitude(int idx);

	void GenFFTForward(int nNewData=0);

	const char* GetFFTName();

	void ShowDebugInfo();

private:
	kiss_fftr_cfg config_forward;
	kiss_fftr_cfg config_inverse;

	kiss_fft_scalar* samples;
	kiss_fft_cpx* spectrum;

};

AudioKissFFT::AudioKissFFT(int nfft):AudioFFT(nfft)
{
	m_nFFT = nfft;
	config_forward = kiss_fftr_alloc(nfft,0,NULL,NULL);
	config_inverse = kiss_fftr_alloc(nfft,1,NULL,NULL);
	spectrum = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * nfft);
	samples = new kiss_fft_scalar[nfft];
	memset(spectrum,0,sizeof(kiss_fft_cpx) * nfft);
	memset(samples,0,sizeof(kiss_fft_scalar)*nfft);
}

AudioKissFFT::~AudioKissFFT()
{
	free(config_forward);
	free(config_inverse);
	free(spectrum);
	delete [] samples;
}

inline void AudioKissFFT::ShowDebugInfo()
{
	ShowPCM(samples,m_nFFT,1,1024,60,"AudioKissFFT");
}

inline void AudioKissFFT::GenFFTForward32f(int nNewData,int nChannels,float* pRawData)
{
	//int nOldData;
	//int value;
	////const double conv = 1.0 / 2147483648.0;

	//GenFFTInputBufferShift(nNewData);
	//m_fRawMaxCur = 0;
	//nOldData = m_nFFT - nNewData;

	//for (int i = 0; i < nNewData; i ++)
	//{
	//	value = 0;
	//	for (int c = 0; c < nChannels; c ++ )
	//	{
	//		value += _swap32(pRawData[i*nChannels+c])/65536;
	//	}
	//	value/=nChannels;
	//	value = _swap32(pRawData[i*nChannels+0])/65536;
	//	samples[nOldData+i] = value;

	//	if (m_fRawMaxCur<abs(value))
	//		m_fRawMaxCur=abs(value);
	//}

	//if (m_fRawMax<m_fRawMaxCur)
	//	m_fRawMax=m_fRawMaxCur;	

	int nOldData;
	double value;
	const double conv = 1.0 / 2147483648.0;

	GenFFTInputBufferShift(nNewData);
	m_fRawMaxCur = 0;
	nOldData = m_nFFT - nNewData;

	assert(sizeof(int) == 4);
	for (int i = 0; i < nNewData; i ++)
	{
		value = 0;
		for (int c = 0; c < nChannels; c ++ )
		{
			value += pRawData[i*nChannels+c];
		}
		value/=nChannels;
		value = pRawData[i*nChannels+0];
		value *= 32767;
		samples[nOldData+i] = value;

		if (m_fRawMaxCur<abs(value))
			m_fRawMaxCur=abs(value);
	}

	if (m_fRawMax<m_fRawMaxCur)
		m_fRawMax=m_fRawMaxCur;	


	GenFFTForward(nNewData);
}

inline void AudioKissFFT::GenFFTForward32i(int nNewData,int nChannels,int* pRawData)
{
	//int nOldData;
	//int value;
	////const double conv = 1.0 / 2147483648.0;

	//GenFFTInputBufferShift(nNewData);
	//m_fRawMaxCur = 0;
	//nOldData = m_nFFT - nNewData;

	//for (int i = 0; i < nNewData; i ++)
	//{
	//	value = 0;
	//	for (int c = 0; c < nChannels; c ++ )
	//	{
	//		value += _swap32(pRawData[i*nChannels+c])/65536;
	//	}
	//	value/=nChannels;
	//	value = _swap32(pRawData[i*nChannels+0])/65536;
	//	samples[nOldData+i] = value;

	//	if (m_fRawMaxCur<abs(value))
	//		m_fRawMaxCur=abs(value);
	//}

	//if (m_fRawMax<m_fRawMaxCur)
	//	m_fRawMax=m_fRawMaxCur;	

	int nOldData;
	double value;
	const double conv = 1.0 / 2147483648.0;

	GenFFTInputBufferShift(nNewData);
	m_fRawMaxCur = 0;
	nOldData = m_nFFT - nNewData;

	assert(sizeof(int) == 4);
	for (int i = 0; i < nNewData; i ++)
	{
		value = 0;
		for (int c = 0; c < nChannels; c ++ )
		{
			value += (double)(_swap32(pRawData[i*nChannels+c]) * conv);
		}
		value/=nChannels;
		value = (double)(_swap32(pRawData[i*nChannels+0]) * conv);
		value *= 32767;
		samples[nOldData+i] = value;

		if (m_fRawMaxCur<abs(value))
			m_fRawMaxCur=abs(value);
	}

	if (m_fRawMax<m_fRawMaxCur)
		m_fRawMax=m_fRawMaxCur;	


	GenFFTForward(nNewData);
}
inline void AudioKissFFT::GenFFTForward16(int nNewData,int nChannels,short* pRawData)
{
	int nOldData;
	int value;
	//const double conv = 1.0 / 32768.0;

	GenFFTInputBufferShift(nNewData);
	m_fRawMaxCur = 0;
	nOldData = m_nFFT - nNewData;

	for (int i = 0; i < nNewData; i ++)
	{
		value = 0;
		for (int c = 0; c < nChannels; c ++ )
		{
			value += _swap16(pRawData[i*nChannels+c]);
		}
		value/=nChannels;
		value = _swap16(pRawData[i*nChannels+0]);
		samples[nOldData+i] = value;
					
		if (m_fRawMaxCur<abs(value))
			m_fRawMaxCur=abs(value);
	}

	if (m_fRawMax<m_fRawMaxCur)
		m_fRawMax=m_fRawMaxCur;

	GenFFTForward(nNewData);
}
inline void AudioKissFFT::GenFFTForward8(int nNewData,int nChannels,unsigned char* pRawData)
{
	int nOldData;
	int value;
	const double conv = 1.0 / 128.0;

	GenFFTInputBufferShift(nNewData);
	m_fRawMaxCur = 0;
	nOldData = m_nFFT - nNewData;

	for (int i = 0; i < nNewData; i ++)
	{
		value = 0;
		for (int c = 0; c < nChannels; c ++ )
		{
			value += ((char)(pRawData[i*nChannels+c])-127)*256;
		}
		value/=nChannels;
		value = ((char)(pRawData[i*nChannels+0])-127)*256;

		samples[nOldData+i] = value;

		if (m_fRawMaxCur<abs(value))
			m_fRawMaxCur=abs(value);
	}

	if (m_fRawMax<m_fRawMaxCur)
		m_fRawMax=m_fRawMaxCur;

	GenFFTForward(nNewData);
}

inline void AudioKissFFT::GenFFTInputBufferShift(int nShift)
{
	if (nShift>0 && m_nFFT-nShift>0)
		memmove(samples,samples+nShift,sizeof(kiss_fft_scalar)*(m_nFFT-nShift));
}

inline int AudioKissFFT::GetFFTInputBufferSize()
{
	return m_nFFT;
}

inline float AudioKissFFT::GetFFTOutputMagnitude(int ifft)
{
	assert(ifft>=0 && ifft<m_nFFT/2+1);
	return sqrt(spectrum[ifft].r*spectrum[ifft].r + spectrum[ifft].i*spectrum[ifft].i);
}

inline void AudioKissFFT::GenFFTForward(int nNewData)
{
	double timeMS = cvGetTickCount();
	//float weight;
	//for (int i=0;i<m_nFFT;i++)
	//{
	//	const float cfCutOffRatio = 0.4;
	//	//weight = 0.5*(1-cos(3.1415926535897932384626433832795*2*double(i)/double(m_nFFT)));
	//	weight = double(i)/double(m_nFFT-nNewData);//0.5*(1-cos(3.1415926535897932384626433832795*2*double(i)/double(m_nFFT)));
	//	if (weight>1)weight=1;
	//	if (weight<cfCutOffRatio) weight=0;
	//	else
	//		weight=(weight-cfCutOffRatio)/(1-cfCutOffRatio);
	//	m_pdFFT_in_weighted[i] = m_pdFFT_in[i];//*weight;
	//}
	kiss_fftr( config_forward, samples, spectrum );

	timeMS = (cvGetTickCount()-timeMS)/(cvGetTickFrequency()*1000.0);
	if (m_fFFTAvgMS==0)
		m_fFFTAvgMS = timeMS;
	else
		m_fFFTAvgMS = timeMS*0.1 + m_fFFTAvgMS*0.9;
}

inline const char* AudioKissFFT::GetFFTName()
{
	return "KissFFT";
}
#endif


AudioSpectrogram::AudioSpectrogram(int nFFT,AudioBuffer *pBuffer,int width,int height)
{
	m_pBuffer = pBuffer;
	m_nFFT = nFFT;

	AudioFFTPackage fftPackage;
	memset(&fftPackage,0,sizeof(fftPackage));

#if ENABLE_FFTW
	fftPackage.pFFT = new AudioFFTW(nFFT);
	fftPackage.pFrqImg = cvCreateImage(cvSize(width,height),IPL_DEPTH_8U,3);
	cvSetZero(fftPackage.pFrqImg);
	m_vFFT.push_back(fftPackage);
#endif

#if ENABLE_KISSFFT
	fftPackage.pFFT = new AudioKissFFT(nFFT);
	fftPackage.pFrqImg = cvCreateImage(cvSize(width,height),IPL_DEPTH_8U,3);
	cvSetZero(fftPackage.pFrqImg);
	m_vFFT.push_back(fftPackage);
#endif

	m_iFrqImgCounter = 0;
	m_fOverlapRatio = 0.04;
	m_piFrqRemap = NULL;
	m_pRawDataBuffer.p64 = NULL;
	m_bIsShiftingSpectrogram = true;
#ifdef EnableDB
	GenFrqRemap(nFFT/2+1,height,true);
#else
	GenFrqRemap(nFFT/2+1,height,false);
#endif

	hGlobalCloseEvent = CreateEvent(NULL, FALSE, FALSE, L"sysAudioSpectrogram_GlobalCloseEvent");
    if (NULL == hGlobalCloseEvent) {
        printf("hGlobalCloseEvent failed: last error is %u\n", GetLastError());
        //return -__LINE__;
    }
}


AudioSpectrogram::~AudioSpectrogram()
{
	Termiate();
	for (unsigned int i=0;i<m_vFFT.size();i++)
	{
		delete m_vFFT[i].pFFT;
		cvReleaseImage(&m_vFFT[i].pFrqImg);
	}

	if (m_piFrqRemap)
		delete [] m_piFrqRemap;
	if (m_pRawDataBuffer.p64)
		delete [] m_pRawDataBuffer.p64;
}


DWORD WINAPI AudioSpectrogram::AudioSpectrogramThreadFunction(LPVOID pContext)
{
	AudioSpectrogram *pSpec = (AudioSpectrogram *)pContext;

	while (!pSpec->m_pBuffer->m_bIsSetAudioInfo) Sleep(10);

	int nDataSizeInBytes,nDataSizeInInt64,nElems,nDataBlockAlign;
	

	for (unsigned int i=0;i<pSpec->m_vFFT.size();i++)
	{
		cvNamedWindow(pSpec->m_vFFT[i].pFFT->GetFFTName());
		cvShowImage(pSpec->m_vFFT[i].pFFT->GetFFTName(),pSpec->m_vFFT[i].pFrqImg);
	}
	
	nElems = pSpec->m_nFFT;
	nDataBlockAlign = pSpec->m_pBuffer->m_nBlockAlign;
	nDataSizeInBytes = pSpec->m_pBuffer->m_nBlockAlign * nElems;
	nDataSizeInInt64 = nDataSizeInBytes/sizeof(INT64)+1;
	pSpec->m_pRawDataBuffer.p64 = new INT64[nDataSizeInInt64];

	pSpec->m_pBuffer->GetBufferFront(pSpec->m_pRawDataBuffer.p8,nDataSizeInBytes);

	int nNewDataInByte,nGotDataInByte;
	int nOldElems;
	int nNewElems;
	int key;
	while ((key=cvWaitKey(10))!=27)
	{
		if (pSpec->m_pBuffer->m_nBlockAlign != nDataBlockAlign)
		{
			delete [] pSpec->m_pRawDataBuffer.p64;
			nDataBlockAlign = pSpec->m_pBuffer->m_nBlockAlign;
			nDataSizeInBytes = pSpec->m_pBuffer->m_nBlockAlign * nElems;
			nDataSizeInInt64 = nDataSizeInBytes/sizeof(INT64)+1;
			pSpec->m_pRawDataBuffer.p64 = new INT64[nDataSizeInInt64];
		}
		nNewDataInByte = pSpec->m_pBuffer->GetNewDataSize();
		nNewElems = nElems * pSpec->m_fOverlapRatio;
		nGotDataInByte = pSpec->m_pBuffer->m_nBlockAlign * nNewElems;

		if (nNewDataInByte < nGotDataInByte) {printf(".");Sleep(10);continue;}
		printf("nNewData : %d  %.2f\n",nNewDataInByte,(float)pSpec->m_pBuffer->GetNewDataSize()/pSpec->m_pBuffer->GetCapacity()*100);
		nGotDataInByte = pSpec->m_pBuffer->GetBufferFront(pSpec->m_pRawDataBuffer.p8,nGotDataInByte);

		nOldElems = nElems-nNewElems;

		for (unsigned int i=0;i<pSpec->m_vFFT.size();i++)
		{

		
			switch (pSpec->m_pBuffer->m_nBitsPerSample)
			{
				case 8:
					if (key == ' ')
					ShowPCM((unsigned char*)pSpec->m_pRawDataBuffer.p8,nNewElems,pSpec->m_pBuffer->m_nChannels,1024,60,"NewData");
					pSpec->m_vFFT[i].pFFT->GenFFTForward8(nNewElems,pSpec->m_pBuffer->m_nChannels,(unsigned char*)pSpec->m_pRawDataBuffer.p8);
					break;

				case 16:
					if (key == ' ')
					ShowPCM((short*)pSpec->m_pRawDataBuffer.p8,nNewElems,pSpec->m_pBuffer->m_nChannels,1024,60,"NewData");
					pSpec->m_vFFT[i].pFFT->GenFFTForward16(nNewElems,pSpec->m_pBuffer->m_nChannels,(short*)pSpec->m_pRawDataBuffer.p8);
					break;

				case 32:
					if (pSpec->m_pBuffer->m_bIsFloat)
					{
						if (key == ' ')
						ShowPCM((float*)pSpec->m_pRawDataBuffer.p8,nNewElems,pSpec->m_pBuffer->m_nChannels,1024,60,"NewData");
						pSpec->m_vFFT[i].pFFT->GenFFTForward32f(nNewElems,pSpec->m_pBuffer->m_nChannels,(float*)pSpec->m_pRawDataBuffer.p8);
					}
					else
					{
						if (key == ' ')
						ShowPCM((int*)pSpec->m_pRawDataBuffer.p8,nNewElems,pSpec->m_pBuffer->m_nChannels,1024,60,"NewData");
						pSpec->m_vFFT[i].pFFT->GenFFTForward32i(nNewElems,pSpec->m_pBuffer->m_nChannels,(int*)pSpec->m_pRawDataBuffer.p8);
					}
					break;

				default:
					assert(false);
				//case 3:
				//{
				//	char *temp2 = (char *)temp;
				//	double conv = 1.0 / 8388608.0;
				//	for (int i = 0; i < numElems; i ++)
				//	{
				//		int value = *((int*)temp2);
				//		value = _swap32(value) & 0x00ffffff;             // take 24 bits
				//		value |= (value & 0x00800000) ? 0xff000000 : 0;  // extend minus sign bits
				//		buffer[i] = (float)(value * conv);
				//		temp2 += 3;
				//	}
				//	break;
				//}


			}

			//if (pSpec->m_vFFT[i].pFFT->m_fRawMaxCur<0.00001)
				//pSpec->m_pBuffer->ClearNewData();
			if (key == ' ')
				pSpec->m_vFFT[i].pFFT->ShowDebugInfo();
			pSpec->m_vFFT[i].pFFT->UpdateSpectrogram(pSpec->m_vFFT[i].pFrqImg,pSpec->m_piFrqRemap);

		}

		//pSpec->Update();
	}

	cvDestroyAllWindows();
	SetEvent(pSpec->hGlobalCloseEvent);


	delete [] pSpec->m_pRawDataBuffer.p64;
	pSpec->m_pRawDataBuffer.p64 = NULL;
	return 0;
}

void AudioSpectrogram::Start()
{
	//Termiate();
	if ((m_hThread != NULL) && (::WaitForSingleObject(m_hThread, 0) != WAIT_OBJECT_0)) return;
	m_hThread  = CreateThread(
        NULL, 0,
        AudioSpectrogram::AudioSpectrogramThreadFunction, this,
        0, NULL
    );
}

void AudioSpectrogram::Update(AudioFFTPackage* pFFTPackage)
{
}


void AudioSpectrogram::GenFrqRemap(int nIn,int nOut,bool bLinear)
{
	if (m_piFrqRemap == NULL)
		m_piFrqRemap = new int[nOut];
	if (nIn<=nOut)
	{
		for (int i=0;i<nOut;i++)
			m_piFrqRemap[i] = 1;
		return;
	}
	else if (bLinear)
	{
		int nInCountDown = nIn;
		int nCurHeight = nIn/nOut;
		for (int i=0;i<nOut-1;i++)
		{
			nInCountDown -= nCurHeight;
			m_piFrqRemap[i] = nCurHeight;
		}
		m_piFrqRemap[nOut-1] = nInCountDown;
		return;
	}
	else
	{
		int nInCountDown = nIn;
		int nCurHeight;
		float fMaxHeight = (nIn*2)/nOut;
		for (int i=0;i<nOut;i++)
		{
			nCurHeight = MAX(ceil(fMaxHeight*float(i)/float(nOut)),1);
			nInCountDown -= nCurHeight;
			m_piFrqRemap[i] = nCurHeight;
		}
		int debugCount=0;
		for (int i=0;i<nOut;i++)
			debugCount += m_piFrqRemap[i];
		printf("%d / %d - %d  nInCountDown %d\n",debugCount,nIn,nOut,nInCountDown);
		if (nInCountDown!=0)
		{
			int nIncrease = (nInCountDown>0)?1:-1;
			int nIdx = nOut-1;
			nInCountDown = abs(nInCountDown);
			while (nInCountDown>0)
			{
				if (m_piFrqRemap[nIdx]>0)
				{
					nInCountDown--;
					m_piFrqRemap[nIdx] += nIncrease;
				}
				nIdx--;
				if (nIdx<0) nIdx = nOut-1;
			}
		}
		debugCount=0;
		for (int i=0;i<nOut;i++)
			debugCount += m_piFrqRemap[i];
		printf("%d / %d - %d\n",debugCount,nIn,nOut);
		return;
	}

}



