#ifndef AUDIO_SPECTROGRAM_WINDOW_H_
#define AUDIO_SPECTROGRAM_WINDOW_H_

#include "LayeredWindow\LayeredWindowBase.h"

struct IWICImagingFactory;
struct IWICBitmapLock;
struct IWICBitmap;

struct IDWriteFactory;
struct IDWriteTextFormat;

class AudioSpectrogramWindow : public LayeredWindowBase
{
public:
	enum AudioSpectrogramWindowDisplayMode
	{
		FFT_Pitch = 0, //Log2
		FFT_Waterfall, //Linear
		FFT_NumDisplayMode
	};

	AudioSpectrogramWindow(int width,int height,LayeredWindowTechType techType,int imgBufferWidth = 512,int imgBufferHeight = 512);
	~AudioSpectrogramWindow();
	
	bool Initialize();
	void Uninitialize();
	
	void BeforeRender();
	void Render();
	void AfterRender();

	bool SetSize(int w,int h);

	void OnMouse(UINT msg,int x,int y,UINT vkey,short wheel);
	void OnKeyboard(int key,bool isKeydown);


	//Generate spectrogram
	BYTE* LockSpecPixel();
	void UnlockSpecPixel();
	inline int GetSpecWidth(){return m_iImgBufferWidth;}
	inline int GetSpecHeight(){return m_iImgBufferHeight;}
	void Repaint();

	//FFT
	float *LockFFTBuffer(int nFFT,int nSamplePerSec);
	void UnlockFFTBuffer();

private:
	//D2D
	IWICImagingFactory* m_pWICImgFactory;
	IWICBitmap* m_pWICImgBitmap;
	IWICBitmapLock* m_pWICImgLock;
	ID2D1Bitmap *m_pD2D1ImgBitmap;
	ID2D1SolidColorBrush *m_pBrushGreen;
    ID2D1SolidColorBrush *m_pBrushWhite;
	ID2D1SolidColorBrush *m_pBrushBlackAlpha;

	IDWriteFactory* m_pDWriteFactory;
	IDWriteTextFormat* m_pWriteTextFormat;
	float m_fDPIScaleX;
	float m_fDPIScaleY;
	//ID2D1Bitmap1 
	//D2D

	//GDI

	//GDI
	
	int m_iImgBufferWidth;
	int m_iImgBufferHeight;

	HRESULT CreateDeviceResources();

	int m_nFFT;
	int m_nSamplesPerSec;
	float* m_afFFT2Pitch;
	float* m_afFFTBuffer;

	float m_fFFTPitchMax;
	float m_fFFTPitchMin;

	float m_fFFTPitchSmoothMax;
	float m_fFFTPitchSmoothMin;

	float m_fFFTFreqMax;
	float m_fFFTFreqMin;

	float m_fdBMax;
	float m_fdBMin;


	AudioSpectrogramWindowDisplayMode m_FFTDisplayMode;

	HANDLE m_hFFTMutex;
	bool m_isNewData;

	bool m_isMouseMDown;
	int m_MouseMovePos[2];
};


#endif