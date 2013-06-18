#include "AudioSpectrogramWindow.h"
#include "LayeredWindow/LayeredWindowD2DtoDXGI.h"
#include "LayeredWindow/LayeredWindowD2DtoGDI.h"
#include "LayeredWindow/LayeredWindowD2DtoWIC.h"
#include "LayeredWindow/LayeredWindowGDI.h"

#include <Wincodec.h>
#include <assert.h>

#include "AudioFFTUtility.h"
#include <Dwrite.h>


AudioSpectrogramWindow::AudioSpectrogramWindow(int width,int height,LayeredWindowTechType techType,int imgBufferWidth,int imgBufferHeight) : 
	LayeredWindowBase(width,height,techType),
	m_iImgBufferWidth(imgBufferWidth),
	m_iImgBufferHeight(imgBufferHeight),
	m_pWICImgFactory(NULL), m_pWICImgBitmap(NULL), m_pWICImgLock(NULL),m_pBrushGreen(NULL),m_pBrushWhite(NULL),m_pBrushBlackAlpha(NULL),
	m_pD2D1ImgBitmap(NULL),
	m_pDWriteFactory(NULL),m_pWriteTextFormat(NULL),
	m_nFFT(0),m_nSamplesPerSec(0),m_afFFT2Pitch(NULL),m_afFFTBuffer(NULL),
	m_FFTDisplayMode(FFT_Pitch)
{
	m_hFFTMutex = CreateMutex(NULL,false,NULL);

	m_fFFTFreqMax = m_fFFTFreqMin = -1;

	m_fdBMax = 50;
	m_fdBMin = -30;
	m_isNewData = false;

	m_isMouseMDown = false;
	m_MouseMovePos[0] = m_MouseMovePos[1] = 0;

	m_fFFTPitchSmoothMax = m_fFFTPitchMax = 12*(7+1); //C7
	m_fFFTPitchSmoothMin = m_fFFTPitchMin = 12*(2+1); //C2

	m_fFFTFreqMax = AudioFFTUtility::Pitch2Freq(m_fFFTPitchMax);
	m_fFFTFreqMin = AudioFFTUtility::Pitch2Freq(m_fFFTPitchMin);

	m_fDPIScaleX = m_fDPIScaleY = 0;
}

AudioSpectrogramWindow::~AudioSpectrogramWindow()
{
	CloseHandle(m_hFFTMutex);
	if (m_afFFT2Pitch)
		delete [] m_afFFT2Pitch;
	if (m_afFFTBuffer)
		delete [] m_afFFTBuffer;
}

bool AudioSpectrogramWindow::Initialize()
{
	if (m_pWICImgFactory) return true;
	HRESULT hr = CoInitialize(0);
	if (!SUCCEEDED(hr)) return false;
	
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory,
		(LPVOID*)&m_pWICImgFactory
		);
	if (!SUCCEEDED(hr)) return false;

	hr = m_pWICImgFactory->CreateBitmap(m_iImgBufferWidth,m_iImgBufferHeight,GUID_WICPixelFormat32bppPBGRA,WICBitmapCacheOnDemand,&m_pWICImgBitmap);

	if (!SUCCEEDED(hr)) return false;
	
	WICRect rcLock;
	IWICBitmapLock *pILock;
	rcLock.X = rcLock.Y = 0;
	rcLock.Width = m_iImgBufferWidth;
	rcLock.Height = m_iImgBufferHeight;

	hr = m_pWICImgBitmap->Lock(&rcLock, WICBitmapLockWrite, &pILock);

	if (!SUCCEEDED(hr)) return false;

	UINT cbBufferSize = 0;
    BYTE *pv = NULL;

      // Retrieve a pointer to the pixel data.
    if (SUCCEEDED(hr))
		hr = pILock->GetDataPointer(&cbBufferSize, &pv);

	if (SUCCEEDED(hr))
		memset(pv,0,sizeof(BYTE)*4*rcLock.Width*rcLock.Height);
	
	for (int i=0;i<rcLock.Width*rcLock.Height;i++)
	{
		pv[0] = 255;
		pv[1] = 0;
		pv[2] = 0;
		pv[3] = 125;
		pv+=4;
	}

	SafeRelease(&pILock);


	if (SUCCEEDED(hr))
	{
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&m_pDWriteFactory)
			);
	}

	if (SUCCEEDED(hr))
	{
		hr = m_pDWriteFactory->CreateTextFormat(
			L"Gabriola",
			NULL,
			DWRITE_FONT_WEIGHT_REGULAR,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			14.0f,
			L"en-us",
			&m_pWriteTextFormat
			);
	}

	m_pWin.pD2D->m_pD2DFactory->GetDesktopDpi(&m_fDPIScaleX,&m_fDPIScaleY);
	m_fDPIScaleX /= 96.0f;
    m_fDPIScaleY /= 96.0f;

	//SetWindowPos(m_hWnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_SHOWWINDOW);

	return true;
}

void AudioSpectrogramWindow::Uninitialize()
{
	SafeRelease(&m_pWICImgLock);
	SafeRelease(&m_pWICImgBitmap);
	SafeRelease(&m_pWICImgFactory);
	SafeRelease(&m_pD2D1ImgBitmap);

	SafeRelease(&m_pBrushGreen);
	SafeRelease(&m_pBrushWhite);

	SafeRelease(&m_pDWriteFactory);
	CoUninitialize();
}


void AudioSpectrogramWindow::BeforeRender()
{

	if (m_techType & LayeredWindow_TechType_D2D)
	{
		if (m_pWin.pD2D->m_pRenderTarget == NULL)
		{
			SafeRelease(&m_pD2D1ImgBitmap);
			SafeRelease(&m_pBrushGreen);
			SafeRelease(&m_pBrushWhite);
		}
	}
}

void AudioSpectrogramWindow::AfterRender()
{
	
}

void AudioSpectrogramWindow::Render()
{
	WaitForSingleObject(m_hFFTMutex, 10000);
	if (m_nFFT == 0)
	{
		__super::Render();
		ReleaseMutex(m_hFFTMutex);
		return;
	}

	if (m_fFFTPitchMin != m_fFFTPitchSmoothMin || m_fFFTPitchMax != m_fFFTPitchSmoothMax)
	{
		m_fFFTPitchSmoothMin = m_fFFTPitchMin*0.2+m_fFFTPitchSmoothMin*0.8;
		m_fFFTPitchSmoothMax = m_fFFTPitchMax*0.2+m_fFFTPitchSmoothMax*0.8;

		if (abs(m_fFFTPitchMin - m_fFFTPitchSmoothMin)<0.1)
			m_fFFTPitchSmoothMin = m_fFFTPitchMin;

		if (abs(m_fFFTPitchMax - m_fFFTPitchSmoothMax)<0.1)
			m_fFFTPitchSmoothMax = m_fFFTPitchMax;

		m_fFFTFreqMax = AudioFFTUtility::Pitch2Freq(m_fFFTPitchSmoothMax);
		m_fFFTFreqMin = AudioFFTUtility::Pitch2Freq(m_fFFTPitchSmoothMin);
	}
	
	const int ciMargin = 10;
	int rectX,rectY,rectW,rectH;

	rectX = rectY = ciMargin;
	rectW = m_info.m_size.cx - ciMargin*2;
	rectH = m_info.m_size.cy - ciMargin*2;

	
	const int ciStringMinPx = 50;
	float fFreqGap = ((float)m_nSamplesPerSec)/m_nFFT;
	float fDataStart,fDataRange;
	int iMag[2];
	iMag[0] = floor(m_fFFTFreqMin/fFreqGap);
	iMag[1] = ceil(m_fFFTFreqMax/fFreqGap);
	
	if (iMag[0]<0) iMag[0] = 0;
	while (m_afFFT2Pitch[ iMag[0] ]<0) iMag[0]++;
	
	if (iMag[1]>=m_nFFT/2+1) iMag[1] = m_nFFT/2;

	if (m_FFTDisplayMode == FFT_Pitch)
	{
		fDataStart = m_fFFTPitchSmoothMin;//m_afFFT2Pitch[ iMag[0] ];
		fDataRange = m_fFFTPitchSmoothMax - m_fFFTPitchSmoothMin;//m_afFFT2Pitch[ iMag[1] ] - fDataStart;
	}
	else
	{
		fDataStart = m_fFFTFreqMin;
		fDataRange = m_fFFTFreqMax - m_fFFTFreqMin;
	}
	
	if (rectX+rectW>m_info.m_size.cx) rectX = m_info.m_size.cx-rectW;
	if (rectX<0)
	{
		rectX = 0;
		rectW = m_info.m_size.cx;
	}

	if (rectY+rectH>m_info.m_size.cy) rectY = m_info.m_size.cy-rectH;
	if (rectY<0)
	{
		rectY = 0;
		rectH = m_info.m_size.cy;
	}

	const char ccNotes[] = {'C','D','E','F','G','A','B'};
	const char ccCents[] = {   2 , 2 , 1 , 2 , 2 , 2 , 1 };
	const char ccCentsWhiteKey[] = {  0, 2 , 4 , 5 , 7 , 9 , 11};

	//cvSetZero(pImg);

	float fVSum,fVAvgPre;
	float fVCount;
	int iImgPosPre,iImgPosCur,iImgPosNext;
	float fVCur,fVNext;
	//float fMagGap = ((float)rectW)/(iMag[1]-iMag[0]+1);
	//float fV,fVPre,fVBase;
	//fVBase = GetFFTOutputMagnitude(0);
	//fVPre = Mag2dB(GetFFTOutputMagnitude(iMag[0]));
	//char buffer[64];
	//for (int p=0,c=-1;p<fDataStart+fDataRange;p+=ccCents[c])
	//{
	//	c=(c+1)%7;
	//	if (p<fDataStart) continue;
	//	iImgPosCur = ((float)p - fDataStart)/fDataRange*rectW;
	//	cvLine(pImg,cvPoint(rectX+iImgPosCur,rectY),cvPoint(rectX+iImgPosCur,rectY+rectH),(c==0)?CV_RGB(255,255,255):CV_RGB(128,128,128));
	//	if (c==0)
	//	{
	//		sprintf_s(buffer,64,"C%d",p/12-1);
	//		//cvPutText(pImg,buffer,cvPoint(rectX+iImgPosCur,rectY+rectH+20),&cvFont(1,1),CV_RGB(255,255,255));
	//	}

	//	//printf("%d[%d],",p,iImgPosCur);
	//}

	//printf("\n\n");








	if (m_techType & LayeredWindow_TechType_D2D)
	{
		//HRESULT hr S_OK;
		
		HRESULT hr = S_OK;


		if (m_pBrushBlackAlpha == NULL)
			hr = m_pWin.pD2D->m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0,0,0,0.5),&m_pBrushBlackAlpha);
		if (m_pBrushGreen == NULL)
			hr = m_pWin.pD2D->m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LimeGreen),&m_pBrushGreen);
		if (m_pBrushWhite == NULL)
			hr = m_pWin.pD2D->m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),&m_pBrushWhite);

		m_pWin.pD2D->m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White,0.5));
		
		const float cfPitchC00 = 12;//AudioFFTUtility::Freq2Pitch(AudioFFTUtility::cfNoteCFreq[0]);
		const float cfPitchC10 = 132;//AudioFFTUtility::Freq2Pitch(AudioFFTUtility::cfNoteCFreq[10]);

		if (m_isNewData)
		{
			m_isNewData = false;

			WICRect rcLock;
			IWICBitmapLock *pILock;
			rcLock.X = rcLock.Y = 0;
			rcLock.Width = m_iImgBufferWidth;
			rcLock.Height = m_iImgBufferHeight;

			hr = m_pWICImgBitmap->Lock(&rcLock, WICBitmapLockWrite, &pILock);

			UINT cbBufferSize = 0;
			BYTE *pv = NULL;

			  // Retrieve a pointer to the pixel data.
			if (SUCCEEDED(hr))
				hr = pILock->GetDataPointer(&cbBufferSize, &pv);

			if (SUCCEEDED(hr))
			{

				int nHalfFFT = m_nFFT/2+1;
				memmove((void*)(pv+4*rcLock.Width),(void*)pv,sizeof(BYTE)*4*rcLock.Width*(rcLock.Height-1));
				memset(pv,0,sizeof(BYTE)*4*rcLock.Width);
				if (m_FFTDisplayMode == FFT_Pitch)
					iImgPosNext = (m_afFFT2Pitch[ 1 ] - cfPitchC00)/(cfPitchC10 - cfPitchC00)*(rcLock.Width-1); 
				else //FFT_Waterfall
					;//iImgPosNext = 0; 
				fVNext = m_afFFTBuffer[ 1 ];
				fVNext = fVNext>=0.0001? MAX(AudioFFTUtility::Mag2dB(fVNext),m_fdBMin) : m_fdBMin;
				fVNext = 1-(fVNext-m_fdBMax)/(m_fdBMin-m_fdBMax);
				for (int i=1;i<nHalfFFT;i++)
				{

					iImgPosCur = iImgPosNext;
					fVCur = fVNext;
					if (m_FFTDisplayMode == FFT_Pitch)
						iImgPosNext = (m_afFFT2Pitch[ i+1 ] - cfPitchC00)/(cfPitchC10 - cfPitchC00)*(rcLock.Width-1); 
						//iImgPosNext = (m_afFFT2Pitch[ i+1 ] - m_afFFT2Pitch[ 1 ])/(m_afFFT2Pitch[ nHalfFFT-1 ] - m_afFFT2Pitch[ 1 ])*(rcLock.Width-1); 
					else //FFT_Waterfall
						;//iImgPosNext = (float(i+1)*fFreqGap - 1)/float(nHalfFFT)*(rcLock.Width-1); 

					//fVNext = Mag2dB(GetFFTOutputMagnitude(i+1));
					//fVNext = fVNext>=0.0001? Mag2dB(fVNext) : fdBLow;
					fVNext = m_afFFTBuffer[ i+1 ];
					fVNext = fVNext>=0.0001? MAX(AudioFFTUtility::Mag2dB(fVNext),m_fdBMin) : m_fdBMin;
					fVNext = 1-(fVNext-m_fdBMax)/(m_fdBMin-m_fdBMax);
					if (iImgPosNext == iImgPosCur)
						fVNext = MAX(fVCur,fVNext);
					else if (iImgPosCur>=0 && iImgPosNext<rcLock.Width)
					{
						for (int p=iImgPosCur;p<iImgPosNext;p++)
						{
							pv[p*4+0] = pv[p*4+1] = pv[p*4+2] = 254*MIN(((fVCur-fVNext)*(1.f-float(p-iImgPosCur)/(iImgPosNext-iImgPosCur))+fVNext),1);
							pv[p*4+3] = 255;
						}
					}

				}

				SafeRelease(&pILock);
				SafeRelease(&m_pD2D1ImgBitmap);
			}
		}

		if (m_pD2D1ImgBitmap==NULL)
		{
			hr = m_pWin.pD2D->m_pRenderTarget->CreateBitmapFromWicBitmap(m_pWICImgBitmap,NULL,&m_pD2D1ImgBitmap);
		}

		D2D1_MATRIX_3X2_F m32;
		m32._12 = m32._21 = 0;
		if (m_FFTDisplayMode == FFT_Pitch)
		{
			m32._31 = (m_fFFTPitchSmoothMin - cfPitchC00)/(cfPitchC10 - cfPitchC00)*(m_iImgBufferWidth-1); 
			m32._32 = 0;

			m32._11 = (m_fFFTPitchSmoothMax - cfPitchC00)/(cfPitchC10 - cfPitchC00)*(m_iImgBufferWidth-1) - m32._31; 
			m32._11 = ((float)rectW) / m32._11;
			m32._22 = m32._11;

			m32._31 *= -m32._11;
			m32._31 += rectX; 

			if (m32._22 * m_iImgBufferHeight < m_info.m_size.cy)
			{
				m32._22 *= float(m_info.m_size.cy)/(m32._22 * m_iImgBufferHeight);
			}

		}
		else
		{
			;
		}
		m_pWin.pD2D->m_pRenderTarget->SetTransform(m32);
		m_pWin.pD2D->m_pRenderTarget->DrawBitmap(
            m_pD2D1ImgBitmap,D2D1::RectF(0,0,m_iImgBufferWidth,m_iImgBufferHeight)
            );
		m_pWin.pD2D->m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());


		TCHAR tbuffer[64];
		for (int p=0;p<fDataStart+fDataRange;p++)//=ccCents[c])
		{
			//c=(c+1)%7;
			//c=(c+1)%12;
			if (p<fDataStart) continue;
			iImgPosCur = ((float)p - fDataStart)/fDataRange*rectW;

			D2D1_POINT_2F point0,point1;
			point0.x = rectX+iImgPosCur;
			point0.y = rectY;
			point1.x = rectX+iImgPosCur;
			point1.y = rectY+rectH-10;
			
			int note = p%12;
			if (note==0)
			{
				m_pWin.pD2D->m_pRenderTarget->DrawLine(point0,point1,m_pBrushBlackAlpha,3);
				m_pWin.pD2D->m_pRenderTarget->DrawLine(point0,point1,m_pBrushWhite,1);
				int slen = _stprintf_s(tbuffer,64,_T("C%d"),p/12-1);
				D2D1_RECT_F layoutRect = D2D1::RectF((point1.x-3)*m_fDPIScaleX,(point1.y-5)*m_fDPIScaleY,(point1.x+100)*m_fDPIScaleX,(point1.y+100)*m_fDPIScaleY);
				m_pWin.pD2D->m_pRenderTarget->DrawText(tbuffer,slen,m_pWriteTextFormat,layoutRect,m_pBrushWhite);
				
				//cvPutText(pImg,buffer,cvPoint(rectX+iImgPosCur,rectY+rectH+20),&cvFont(1,1),CV_RGB(255,255,255));
			}
			else if (note == 2 || note == 4 || note ==  5 || note == 7 || note == 9 || note == 11)
			{
				//m_pWin.pD2D->m_pRenderTarget->DrawLine(point0,point1,m_pBrushBlackAlpha,3);
				m_pWin.pD2D->m_pRenderTarget->DrawLine(point0,point1,m_pBrushWhite,0.5);
			}
			else
				m_pWin.pD2D->m_pRenderTarget->DrawLine(point0,point1,m_pBrushBlackAlpha,1);

			//printf("%d[%d],",p,iImgPosCur);
		}

		
		
		fVSum = fVCount = 0;
		iImgPosNext = 0;

		fVNext = m_afFFTBuffer[ iMag[0] ];
		fVNext = fVNext>=0.0001? MAX(AudioFFTUtility::Mag2dB(fVNext),m_fdBMin) : m_fdBMin;
	
		fVAvgPre = 0;
		iImgPosPre = -1;

		for (int i=iMag[0];i<iMag[1];i++)
		{
			iImgPosCur = iImgPosNext;
			fVCur = fVNext;
			if (m_FFTDisplayMode == FFT_Pitch)
				iImgPosNext = (m_afFFT2Pitch[ i+1 ] - fDataStart)/fDataRange*rectW; 
			else //FFT_Waterfall
				iImgPosNext = (float(i+1) - fDataStart)/fDataRange*rectW; 

			//fVNext = Mag2dB(GetFFTOutputMagnitude(i+1));
			//fVNext = fVNext>=0.0001? Mag2dB(fVNext) : fdBLow;
			fVNext = m_afFFTBuffer[ i+1 ];
			fVNext = fVNext>=0.0001? MAX(AudioFFTUtility::Mag2dB(fVNext),m_fdBMin) : m_fdBMin;

			if (fVCur != m_fdBMin || fVNext != m_fdBMin)
			{
				D2D1_POINT_2F point0,point1;
				point0.x = rectX+iImgPosCur;
				point0.y = rectY+((fVCur-m_fdBMax)/(m_fdBMin-m_fdBMax))*rectH;
				point1.x = rectX+iImgPosNext;
				point1.y = rectY+(fVNext-m_fdBMax)/(m_fdBMin-m_fdBMax)*rectH;
				m_pWin.pD2D->m_pRenderTarget->DrawLine(point0,point1,m_pBrushBlackAlpha,4);
				m_pWin.pD2D->m_pRenderTarget->DrawLine(point0,point1,m_pBrushGreen,2);

				point0.x = point0.x*m_iImgBufferWidth/m_info.m_size.cx;
				point0.y = point0.y*m_iImgBufferHeight/m_info.m_size.cy;
				point1.x = point1.x*m_iImgBufferWidth/m_info.m_size.cx;
				point1.y = point1.y*m_iImgBufferHeight/m_info.m_size.cy;

				//cvLine(pImg,cvPoint(rectX+iImgPosCur,rectY+((fVCur-fdBHigh)/(fdBLow-fdBHigh))*rectH),cvPoint(rectX+iImgPosNext,rectY+(fVNext-fdBHigh)/(fdBLow-fdBHigh)*rectH),CV_RGB(0,255,0));
			}
				
		}

		
		
		
	}
	else if (m_techType & LayeredWindow_TechType_GDI)
	{
		//m_pWin.pGDI->BeginDraw();
		//Gdiplus::Graphics graphics( m_pBitmap->GetDC() ); //#MOD

		wchar_t pszbuf[] = L"Spectrogram for pure GDI mode is not implemented yet.";
		int nStrLen = wcslen(pszbuf);

		m_pWin.pGDI->m_pGraphics->Clear( Gdiplus::Color( 128, 255, 255, 255 ) );

		Gdiplus::FontFamily fontFamily(L"Arial");
		Gdiplus::Font oMS( &fontFamily, 32, Gdiplus::FontStyle::FontStyleRegular, Gdiplus::Unit::UnitPixel );
		Gdiplus::StringFormat strformat;


		Gdiplus::GraphicsPath *Path;
		Gdiplus::GraphicsPath path;
		//		Region *aRegion;
		Gdiplus::RectF rectf;
		
		rectf.X = 10;
		rectf.Y = 10;
		rectf.Width = 500;
		rectf.Height = 500;


		path.AddString(pszbuf, nStrLen, &fontFamily, 
			Gdiplus::FontStyleRegular, 32,rectf,&strformat );

		Gdiplus::SolidBrush brush(Gdiplus::Color(255,100,255));
		m_pWin.pGDI->m_pGraphics->FillPath(&brush, &path);
		//m_pWin.pGDI->EndDraw();
	}


	ReleaseMutex(m_hFFTMutex);
}

bool AudioSpectrogramWindow::SetSize(int w,int h)
{
	printf("AudioSpectrogramWindow::SetSize\n");
	if (m_techType == LayeredWindow_TechType_D2DtoGDI)
		SafeRelease(&m_pD2D1ImgBitmap);
	return __super::SetSize(w,h);
}


BYTE* AudioSpectrogramWindow::LockSpecPixel()
{
	HRESULT hr;
	WICRect rcLock;
	rcLock.X = rcLock.Y = 0;
	rcLock.Width = m_iImgBufferWidth;
	rcLock.Height = m_iImgBufferHeight;

	hr = m_pWICImgBitmap->Lock(&rcLock, WICBitmapLockWrite, &m_pWICImgLock);

	if (!SUCCEEDED(hr)) return false;

	UINT cbBufferSize = 0;
    BYTE *pv = NULL;

      // Retrieve a pointer to the pixel data.
    if (SUCCEEDED(hr))
		hr = m_pWICImgLock->GetDataPointer(&cbBufferSize, &pv);
	return pv;
}

void AudioSpectrogramWindow::UnlockSpecPixel()
{
	SafeRelease(&m_pWICImgLock);
	SafeRelease(&m_pD2D1ImgBitmap);
}


void AudioSpectrogramWindow::Repaint()
{
	SendMessage(m_hWnd, WM_PAINT, false, 0);
}


float *AudioSpectrogramWindow::LockFFTBuffer(int nFFT,int nSamplePerSec)
{
	WaitForSingleObject(m_hFFTMutex, 10000);
	if (nFFT == m_nFFT && nSamplePerSec == m_nSamplesPerSec)
		return m_afFFTBuffer;
	bool isNewSamplePreSec = false;
	if (nFFT != m_nFFT)
	{
		if (m_afFFT2Pitch)
			delete [] m_afFFT2Pitch;
		if (m_afFFTBuffer)
			delete [] m_afFFTBuffer;
		m_nFFT = nFFT;
		m_afFFT2Pitch = new float [m_nFFT/2+1];
		m_afFFTBuffer = new float [m_nFFT/2+1];
		isNewSamplePreSec = true;
		memset(m_afFFTBuffer,0,sizeof(float)*(m_nFFT/2+1));

	}
	if (nSamplePerSec != m_nSamplesPerSec)
	{
		m_nSamplesPerSec = nSamplePerSec;
		isNewSamplePreSec = true;
	}
	
	if (isNewSamplePreSec)
	{
		int nHalfFFT = nFFT/2+1;
		float fFreqGap = ((float)nSamplePerSec)/nFFT;

		m_afFFT2Pitch[0] = AudioFFTUtility::Freq2Pitch(0.0001);
		for (int i=1;i<nHalfFFT;i++)
			m_afFFT2Pitch[i] = AudioFFTUtility::Freq2Pitch(i*fFreqGap);	
	}

	return m_afFFTBuffer;
}

void AudioSpectrogramWindow::UnlockFFTBuffer()
{
	m_isNewData = true;
	ReleaseMutex(m_hFFTMutex);
}

void AudioSpectrogramWindow::OnKeyboard(int key,bool isKeydown)
{
	if (key == 27) DestroyWindow(m_hWnd);
}

void AudioSpectrogramWindow::OnMouse(UINT msg,int x,int y,UINT vkey,short wheel)
{
	switch (msg)
	{
	case WM_MOUSEWHEEL:
		//printf("\t\t\t\t\tWheel  : %d\n",wheel);

		//Up: Zoom in
		//Down: Zoon out
		
		if (m_FFTDisplayMode == FFT_Pitch)
		{
			if (wheel>0)
			{
				m_fFFTPitchMax -=0.5;
				m_fFFTPitchMin +=0.5;
			}
			else if (wheel<0)
			{
				m_fFFTPitchMax +=0.5;
				m_fFFTPitchMin -=0.5;
			}

			if (m_fFFTPitchMax-m_fFFTPitchMin<14)
			{
				float center = (m_fFFTPitchMax+m_fFFTPitchMin)/2;
				m_fFFTPitchMax = center + 7;
				m_fFFTPitchMin = center - 7;
			}

			if (m_fFFTPitchMax-m_fFFTPitchMin>12*(10+1))
			{
				m_fFFTPitchMin = 0;
				m_fFFTPitchMax = 12*(10+1);
			}
			if (m_fFFTPitchMin<0)
			{
				m_fFFTPitchMax -= m_fFFTPitchMin;
				m_fFFTPitchMin = 0;
			}
			if (m_fFFTPitchMax>12*(10+1))
			{
				m_fFFTPitchMin -= (m_fFFTPitchMax-12*(10+1));
				m_fFFTPitchMax = 12*(10+1);
			}
		}
		else if (m_FFTDisplayMode == FFT_Waterfall)
		{
		}



		break;
	case WM_NCMOUSEMOVE:
	case WM_MOUSEMOVE:
		//printf("\t\t\t\t\tMove   : %d , %d\n",x,y);
		if (m_isMouseMDown)
		{
			float disx,disy;
			disx = x - m_MouseMovePos[0];
			disy = y - m_MouseMovePos[1];
			if (m_FFTDisplayMode == FFT_Pitch)
			{
				float disdB,disPitch;
				disdB = disy/float(m_info.m_size.cy)*(m_fdBMax-m_fdBMin);
				disPitch = disx/float(m_info.m_size.cx)*(m_fFFTPitchMax-m_fFFTPitchMin);
				m_fdBMax += disdB;
				m_fdBMin += disdB;


				if (m_fdBMax<10)
				{
					m_fdBMin+=(10-m_fdBMax);
					m_fdBMax = 10;
				}

				if (m_fdBMin>-10)
				{
					m_fdBMax+=(m_fdBMin - (-10));
					m_fdBMin = -10;
				}


				m_fFFTPitchMax -= disPitch;
				m_fFFTPitchMin -= disPitch;
				
				if (m_fFFTPitchMin<0)
				{
					m_fFFTPitchMax -= m_fFFTPitchMin;
					m_fFFTPitchMin = 0;
				}
				if (m_fFFTPitchMax>12*(10+1))
				{
					m_fFFTPitchMin -= (m_fFFTPitchMax-12*(10+1));
					m_fFFTPitchMax = 12*(10+1);
				}

			}
			else if (m_FFTDisplayMode == FFT_Waterfall)
			{
			}
		}
		
		m_MouseMovePos[0] = x;
		m_MouseMovePos[1] = y;
		break;
	case WM_NCMBUTTONDOWN:
	case WM_MBUTTONDOWN:
		m_isMouseMDown = true;
		//printf("\t\t\t\t\tM Down : %d , %d\n",x,y);
		break;
	case WM_NCMBUTTONUP:
	case WM_MBUTTONUP:
		m_isMouseMDown = false;
		//printf("\t\t\t\t\tM Up   : %d , %d\n",x,y);
		break;

	//case WM_NCRBUTTONDOWN:
	//case WM_RBUTTONDOWN:
	//	printf("\t\t\t\t\tL Down : %d , %d\n",x,y);
	//	break;
	//case WM_NCRBUTTONUP:
	//case WM_RBUTTONUP:
	//	printf("\t\t\t\t\tL Up   : %d , %d\n",x,y);
	//	break;
	//case WM_LBUTTONDBLCLK:
	//case WM_RBUTTONDOWN:
	//case WM_RBUTTONUP:
	//case WM_RBUTTONDBLCLK:
	//case WM_MBUTTONDOWN:
	//case WM_MBUTTONUP:
	//case WM_MBUTTONDBLCLK:
			
	
	
	//case WM_NCLBUTTONDBLCLK :
	//case WM_NCRBUTTONDOWN   :
	//case WM_NCRBUTTONUP     :
	//case WM_NCRBUTTONDBLCLK :
	//case WM_NCMBUTTONDOWN   :
	//case WM_NCMBUTTONUP     :
	//case WM_NCMBUTTONDBLCLK :

	}
}