#ifndef LAYEREDWINDOW_D2DtoWIC_H_
#define LAYEREDWINDOW_D2DtoWIC_H_

#include "LayeredWindowInfo.h"
//#include "DIBWrapper.h"

#include <d2d1.h>

#include <Wincodec.h>

class LayeredWindowD2DtoWIC : public LayeredWindowD2DBase
{
public:
	LayeredWindowD2DtoWIC(
		__in UINT width,
		__in UINT height) :
	LayeredWindowD2DBase(width, height),
	m_pWICFactory(NULL),
	m_pWICBitmap(NULL),
	//m_pRenderTarget(NULL),
	m_pGdiInteropRenderTarget(NULL)
	//m_pD2DFactory(NULL)
	//m_pRenderTarget(NULL)
	{
		CreateDeviceIndependentResources();
	}

	~LayeredWindowD2DtoWIC()
	{
		DiscardDeviceResources();
		SafeRelease(&m_pWICFactory);
		SafeRelease(&m_pWICBitmap);
		SafeRelease(&m_pD2DFactory);
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
		hr = CreateDeviceResources();
		m_pRenderTarget->BeginDraw();
		return hr;
	}

	HRESULT EndDraw()
	{
		HRESULT hr = S_OK;
		hr = m_pRenderTarget->EndDraw();
		assert(hr>=0);

		if (D2DERR_RECREATE_TARGET == hr) {
			DiscardDeviceResources();
		}
		return hr;
	}

	HDC GetDC()
	{
		HRESULT hr = S_OK;

		HDC hdc;
		hr = m_pGdiInteropRenderTarget->GetDC(
			D2D1_DC_INITIALIZE_MODE_COPY,
			//D2D1_DC_INITIALIZE_MODE_CLEAR,
			&hdc);
		//if (hr<0)printf("GetDC Error  0x%X\n",hr);
		//else
		//	printf("GetDC Success!!!\n");
		assert(hr>=0);
		return hdc;
	}

	HRESULT ReleaseDC()
	{
		HRESULT hr = S_OK;
		RECT rect = {};
		m_pGdiInteropRenderTarget->ReleaseDC(&rect);
		return hr;
	}

	bool OnSize(int w,int h)
	{
		HRESULT hr = S_OK;
		if (w<=0 || h<=0) return false;
		m_width = w;
		m_height = h;

		SafeRelease(&m_pWICBitmap);
		hr = m_pWICFactory->CreateBitmap(
			m_width,
			m_height,
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapCacheOnLoad,
			&m_pWICBitmap);
		DiscardDeviceResources();
		assert(hr>=0);
		return true;
	}

private:

	IWICImagingFactory *m_pWICFactory;
	IWICBitmap *m_pWICBitmap;

	//ID2D1RenderTarget *m_pRenderTarget;
	//ID2D1Factory* m_pD2DFactory;
	ID2D1GdiInteropRenderTarget *m_pGdiInteropRenderTarget;

	HRESULT CreateDeviceIndependentResources()
	{
		HRESULT hr;

		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory,
			(LPVOID*)&m_pWICFactory
			);
		S_OK;
		assert(hr>=0);

		hr = m_pWICFactory->CreateBitmap(
			m_width,
			m_height,
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapCacheOnLoad,
			&m_pWICBitmap);
		assert(hr>=0);

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
		if (m_pRenderTarget == NULL)
		{
			const D2D1_PIXEL_FORMAT format = 
				D2D1::PixelFormat(
				DXGI_FORMAT_B8G8R8A8_UNORM,
				D2D1_ALPHA_MODE_PREMULTIPLIED);

			const D2D1_RENDER_TARGET_PROPERTIES properties = 
				D2D1::RenderTargetProperties(
				D2D1_RENDER_TARGET_TYPE_DEFAULT,
				format,
				0.0f, // default dpi
				0.0f, // default dpi
				D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE);

			hr = m_pD2DFactory->CreateWicBitmapRenderTarget(
				m_pWICBitmap,
				properties,
				&m_pRenderTarget);
			assert(hr>=0);

			hr = m_pRenderTarget->QueryInterface(&m_pGdiInteropRenderTarget);
			assert(hr>=0);
		}
		return hr;
	}

	void DiscardDeviceResources()
	{
		SafeRelease(&m_pRenderTarget);
		SafeRelease(&m_pGdiInteropRenderTarget);
	}
};

#endif