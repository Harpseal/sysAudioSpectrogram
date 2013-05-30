#ifndef LAYEREDWINDOW_GDI_H_
#define LAYEREDWINDOW_GDI_H_

#include "LayeredWindowInfo.h"
#include "DIBWrapper.h"
#include <Gdiplus.h>


class LayeredWindowGDI : public LayeredWindowAPIBase
{
public:
	LayeredWindowGDI(
		__in UINT width,
		__in UINT height) :
	LayeredWindowAPIBase(width, height),
	m_pGraphics(NULL)
	//m_graphics(m_bitmap.GetDC())
	{
		CreateDeviceIndependentResources();
	}

	~LayeredWindowGDI()
	{
		delete m_pBitmap;
		if (m_pGraphics)
			delete m_pGraphics;
		Gdiplus::GdiplusShutdown( m_pGdiPlusToken );
	}

	bool OnSize(int w,int h)
	{
		//if (!__super::OnSize(w,h))return false;
		if (w<=0 || h<=0) return false;
		m_width = w;
		m_height = h;

		delete m_pBitmap;
		m_pBitmap = new GdiBitmap(w,h);
		DiscardDeviceResources();
		
		return true;
	}


	HRESULT BeginDraw()
	{
		CreateDeviceResources();
		return S_OK;
	}
	HRESULT EndDraw()
	{
		//if (m_pGraphics) delete m_pGraphics;
		//m_pGraphics = NULL;
		//UpdateLayeredWindow(m_pBitmap->GetDC());
		return S_OK;
	}

	HDC GetDC()
	{
		return m_pBitmap->GetDC();
	}

	const Gdiplus::Graphics *GetGraphics(){return m_pGraphics;}

	void Render()
	{
		BeginDraw();
		//Gdiplus::Graphics graphics( m_pBitmap->GetDC() ); //#MOD
		m_pGraphics->Clear( Gdiplus::Color( 128, 255, 255, 255 ) );


		Gdiplus::FontFamily fontFamily(L"微軟正黑體");
		Gdiplus::Font oMS( &fontFamily, 32, Gdiplus::FontStyle::FontStyleRegular, Gdiplus::Unit::UnitPixel );
		Gdiplus::StringFormat strformat;
		wchar_t pszbuf[] = L"1.Text Designer中文字也可顯示\n2.Text Designer中文字也可顯示\n3.Text Designer中文字也可顯示\n4.Text Designer中文字也可顯示\n5.Text Designer中文字也可顯示\n6.Text Designer中文字也可顯示 Text Designer中文字也可顯示\n7.Text Designer中文字也可顯示\n8.Text Designer中文字也可顯示\n9.Text Designer中文字也可顯示\n10.Text Designer中文字也可顯示";


		Gdiplus::GraphicsPath *Path;
		Gdiplus::GraphicsPath path;
//		Region *aRegion;
		Gdiplus::RectF rectf;
		int nStrLen = wcslen(pszbuf);
		rectf.X = 10;
		rectf.Y = 10;
		rectf.Width = 500;
		rectf.Height = 500;


		path.AddString(pszbuf, nStrLen, &fontFamily, 
			Gdiplus::FontStyleRegular, 32,rectf,&strformat );

		Gdiplus::SolidBrush brush(Gdiplus::Color(255,100,255));
		m_pGraphics->FillPath(&brush, &path);

		//SelectObject(m_bitmap.m_hdc, m_bitmap.m_bitmap);
		EndDraw();
	}

//private:
	GdiBitmap *m_pBitmap;
	Gdiplus::GdiplusStartupInput m_oGdiPlusStartupInput;
	ULONG_PTR m_pGdiPlusToken;
	Gdiplus::Graphics *m_pGraphics;

	HRESULT CreateDeviceIndependentResources()
	{
		m_pBitmap = new GdiBitmap(m_width,m_height);
		Gdiplus::GdiplusStartup( &m_pGdiPlusToken, &m_oGdiPlusStartupInput, NULL );
		return S_OK;
		//setRanderClassCallback(this,&LayeredWindow::Render);
	}
	HRESULT CreateDeviceResources()
	{
		if (m_pGraphics==NULL)
		{//delete m_pGraphics;
			m_pGraphics = new Gdiplus::Graphics(m_pBitmap->GetDC());
			m_pGraphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
			m_pGraphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		}
		return S_OK;
	}
	void DiscardDeviceResources()
	{
		if (m_pGraphics)
			delete m_pGraphics;
		m_pGraphics = NULL;
	}
};

#endif