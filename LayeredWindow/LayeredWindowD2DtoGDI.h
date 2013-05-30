#ifndef LAYEREDWINDOW_D2DtoGDI_H_
#define LAYEREDWINDOW_D2DtoGDI_H_

#include "LayeredWindowInfo.h"
#include "DIBWrapper.h"

#include <d2d1.h>

class LayeredWindowD2DtoGDI : public LayeredWindowD2DBase
{
public:
	LayeredWindowD2DtoGDI(
		__in UINT width,
		__in UINT height) :
	LayeredWindowD2DBase(width, height)
	//m_bitmap(width, height),
	{
		CreateDeviceIndependentResources();
	}

	~LayeredWindowD2DtoGDI()
	{
		delete m_pBitmap;
		DiscardDeviceResources();
		SafeRelease(&m_pD2DFactory);
	}

	bool OnSize(int w,int h)
	{
		//if (!__super::OnSize(w,h)) return false;
		if (w<=0 || h<=0) return false;
		m_width = w;
		m_height = h;

		delete m_pBitmap;
		m_pBitmap = new GdiBitmap(w,h);
		DiscardDeviceResources();
		return true;
	}

	void Render()
	{
		BeginDraw();
		m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White,0.5));
		EndDraw();
	}

	HRESULT BeginDraw()
	{
		HRESULT hr = S_OK;
		CreateDeviceResources();
		m_pRenderTarget->BeginDraw();
		return hr;
	}

	HRESULT EndDraw()
	{
		HRESULT hr = S_OK;
		//UpdateLayeredWindow(m_pBitmap->GetDC());

		hr = m_pRenderTarget->EndDraw();

		if (D2DERR_RECREATE_TARGET == hr) {
			DiscardDeviceResources();
		}
		return hr;
	}

	HDC GetDC()
	{
		return m_pBitmap->GetDC();
	}

private:
	GdiBitmap *m_pBitmap;

	HRESULT CreateDeviceIndependentResources()
	{
		HRESULT hr;
		m_pBitmap = new GdiBitmap(m_width,m_height);
		// Create Direct2D factory.
		hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			&m_pD2DFactory
			);
		assert(hr>=0);
		return hr;
	}

	HRESULT CreateDeviceResources()
	{
		HRESULT hr = S_OK;
		if (m_pRenderTarget==NULL)
		{
			const D2D1_PIXEL_FORMAT format = 
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
				D2D1_ALPHA_MODE_PREMULTIPLIED);

			const D2D1_RENDER_TARGET_PROPERTIES properties = 
				D2D1::RenderTargetProperties(
				D2D1_RENDER_TARGET_TYPE_DEFAULT,
				format);
			
			ID2D1DCRenderTarget *pDCRenderTarget;
			hr = m_pD2DFactory->CreateDCRenderTarget(
				&properties,
				&pDCRenderTarget);
			assert(hr>=0);

			const RECT rect = {0, 0, m_pBitmap->GetWidth(), m_pBitmap->GetHeight()};

			hr = pDCRenderTarget->BindDC(m_pBitmap->GetDC(), &rect);
			m_pRenderTarget = (ID2D1RenderTarget*)pDCRenderTarget;
			assert(hr>=0);
		}
		return hr;

	}
	void DiscardDeviceResources()
	{
		SafeRelease(&(ID2D1RenderTarget*)m_pRenderTarget);
	}
};

#endif