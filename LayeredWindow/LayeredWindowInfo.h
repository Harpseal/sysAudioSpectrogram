#ifndef LAYEREDWINDOWINFO_H_
#define LAYEREDWINDOWINFO_H_

//#include <atlbase.h>
//#include <atlwin.h>
#include <Windows.h>
#include <assert.h>
#include <vector>
#include <tchar.h>

template <class T> inline void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

class LayeredWindowInfo {
public:
	const POINT m_sourcePosition;
	POINT m_windowPosition;
	UINT m_width;
	SIZE m_size;
	BLENDFUNCTION m_blend;
	UPDATELAYEREDWINDOWINFO m_info;
	unsigned char m_border;
	SIZE m_minsize;

	LayeredWindowInfo(
		__in UINT width,
		__in UINT height) :
	m_sourcePosition(),
		m_windowPosition(),
		m_blend(),
		m_info() {

			m_size.cx = width;
			m_size.cy = height;

			m_info.cbSize = sizeof(UPDATELAYEREDWINDOWINFO);
			m_info.pptSrc = &m_sourcePosition;
			m_info.pptDst = &m_windowPosition;
			m_info.psize = &m_size;
			m_info.pblend = &m_blend;
			m_info.dwFlags = ULW_ALPHA;

			m_blend.SourceConstantAlpha = 255;
			m_blend.AlphaFormat = AC_SRC_ALPHA;

			m_border = 5;
			m_minsize.cx = 200;
			m_minsize.cy = 100;
	}

	void Update(
		__in HWND hwnd,
		__in HDC hdc) {

			if (hdc == 0)
			{
				printf("LayeredWindowInfo::Update Error: hdc == 0\n");
			}

			m_info.hdcSrc = hdc;

			bool res = UpdateLayeredWindowIndirect(hwnd, &m_info);
			if (!res)
			{

				LPVOID lpMsgBuf;

				FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					GetLastError(), // 這裡也可以改成你想看的代碼值，例如直接打8L可得"空間不足"
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL);
				_tprintf(_T("錯誤訊息：%s\n錯誤代碼：0x%x"), lpMsgBuf, GetLastError());

				//strMsg.Format(_T("錯誤訊息：%s\n錯誤代碼：0x%x"), lpMsgBuf, GetLastError());

				LocalFree(lpMsgBuf); // 記得free掉空間

			}

	}

	UINT GetWidth() const { return m_size.cx; }
	UINT GetHeight() const { return m_size.cy; }
};

//struct LayeredWindowPoint2Df
//{
//	float x;
//	float y;
//};
//
//struct LayeredWindowColor
//{
//	unsigned int r;
//	unsigned int g;
//	unsigned int b;
//	unsigned int a;
//};


class LayeredWindowAPIBase
{
public:
	LayeredWindowAPIBase(
		__in UINT width,
		__in UINT height) :
	m_width(width),m_height(height){}
	~LayeredWindowAPIBase(){}
	
	virtual HRESULT BeginDraw() = 0;
	virtual HRESULT EndDraw() = 0;
	
	virtual HDC GetDC() = 0;
	virtual HRESULT ReleaseDC(){return S_OK;}

	virtual bool OnSize(int width,int height) = 0;

	//Draw API
	//virtual DrawString(TCHAR* pFontName,TCHAR* pString,
	//	LayeredWindowColor lwColor,
	//	unsigned int uiDrawFlag);

	//virtual DrawLine(LayeredWindowPoint2Df pt0,LayeredWindowPoint2Df pt1,
	//	LayeredWindowColor lwColor,
	//	float fLineWidth,
	//	unsigned int uiDrawFlag);
	int GetWidth() {return m_width;}
	int GetHeight() {return m_height;}

protected:
	int m_width;
	int m_height;

	virtual HRESULT CreateDeviceIndependentResources() = 0;
	virtual HRESULT CreateDeviceResources() = 0;
	virtual void DiscardDeviceResources() = 0;
};

#include <d2d1.h>
class LayeredWindowD2DBase : public LayeredWindowAPIBase
{
public:
	LayeredWindowD2DBase(int width,int height):
	  LayeredWindowAPIBase(width,height),
	  m_pD2DFactory(NULL),
	  m_pRenderTarget(NULL)
	  {}
	  ~LayeredWindowD2DBase(){}
	  const ID2D1RenderTarget *GetRenderTarget(){return m_pRenderTarget;}
//protected:
	ID2D1Factory* m_pD2DFactory;
	ID2D1RenderTarget *m_pRenderTarget;
};

#endif