#ifndef LAYEREDWINDOW_D2DtoDXGI_H_
#define LAYEREDWINDOW_D2DtoDXGI_H_

#include "LayeredWindowInfo.h"
//#include "DIBWrapper.h"

#include <d3d10_1.h>
#include <d2d1.h>

#include <Wincodec.h>

class LayeredWindowD2DtoDXGI : public LayeredWindowD2DBase
{
public:
	LayeredWindowD2DtoDXGI(
		__in UINT width,
		__in UINT height) :
	LayeredWindowD2DBase(width, height),
	m_pDevice(NULL),
	m_pTexture(NULL),
	m_pSurface(NULL),
	m_pGdiInteropRenderTarget(NULL)
	{
		CreateDeviceIndependentResources();
	}

	~LayeredWindowD2DtoDXGI()
	{
		DiscardDeviceResources();
		
		SafeRelease(&m_pTexture);
		SafeRelease(&m_pSurface);
		SafeRelease(&m_pDevice);
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
		CreateDeviceResources();
		m_pRenderTarget->BeginDraw();
		return hr;
	}

	HRESULT EndDraw()
	{
		HRESULT hr = S_OK;
		//HDC hdc = 0;
		//hr = m_pGdiInteropRenderTarget->GetDC(
		//	D2D1_DC_INITIALIZE_MODE_COPY,
		//	&hdc);
		//assert(hr>=0);
		//UpdateLayeredWindow(hdc);
		//RECT rect = {};
		//m_pGdiInteropRenderTarget->ReleaseDC(&rect);

		hr = m_pRenderTarget->EndDraw();

		if (D2DERR_RECREATE_TARGET == hr) {
			DiscardDeviceResources();
		}
		return hr;
	}

	HDC GetDC()
	{
		HRESULT hr = S_OK;
		HDC hdc = 0;
		hr = m_pGdiInteropRenderTarget->GetDC(
			D2D1_DC_INITIALIZE_MODE_COPY,
			&hdc);
		assert(hr>=0);
		return hdc;
	}

	HRESULT ReleaseDC()
	{
		HRESULT hr = S_OK;
		RECT rect = {};
		hr = m_pGdiInteropRenderTarget->ReleaseDC(&rect);
		return hr;
	}

	bool OnSize(int w,int h)
	{
		if (w<=0 || h<=0) return false;
		m_width = w;
		m_height = h;

		HRESULT hr = S_OK;
		//LayeredWindow::OnSize(w,h);
		SafeRelease(&m_pTexture);
		SafeRelease(&m_pSurface);

		D3D10_TEXTURE2D_DESC description = {};
		description.ArraySize = 1;
		description.BindFlags = 
			D3D10_BIND_RENDER_TARGET;
		description.Format = 
			DXGI_FORMAT_B8G8R8A8_UNORM;
		description.Width = m_width;
		description.Height = m_height;
		description.MipLevels = 1;
		description.SampleDesc.Count = 1;
		description.MiscFlags = 
			D3D10_RESOURCE_MISC_GDI_COMPATIBLE;

		hr = m_pDevice->CreateTexture2D(
			&description,
			0, // no initial data
			&m_pTexture);
		assert(hr>=0);

		hr = m_pTexture->QueryInterface(&m_pSurface);
		assert(hr>=0);
		DiscardDeviceResources();
		return true;

	}

private:

	ID3D10Device1 *m_pDevice;
	ID3D10Texture2D *m_pTexture;
	IDXGISurface *m_pSurface;

	ID2D1GdiInteropRenderTarget *m_pGdiInteropRenderTarget;
	
	//IWICImagingFactory *m_pWICFactory;
	//IWICBitmap *m_pWICBitmap;

	//ID2D1RenderTarget *m_pRenderTarget;
	//
	HRESULT CreateDeviceIndependentResources()
	{
		HRESULT hr;

		hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			&m_pD2DFactory
			);
		assert(hr>=0);

		hr = D3D10CreateDevice1(
			0, // adapter
			D3D10_DRIVER_TYPE_HARDWARE,
			0, // reserved
			D3D10_CREATE_DEVICE_BGRA_SUPPORT,
			D3D10_FEATURE_LEVEL_10_0,
			D3D10_1_SDK_VERSION,
			&m_pDevice);
		assert(hr>=0);
		D3D10_TEXTURE2D_DESC description = {};
		description.ArraySize = 1;
		description.BindFlags = 
			D3D10_BIND_RENDER_TARGET;
		description.Format = 
			DXGI_FORMAT_B8G8R8A8_UNORM;
		description.Width = m_width;
		description.Height = m_height;
		description.MipLevels = 1;
		description.SampleDesc.Count = 1;
		description.MiscFlags = 
			D3D10_RESOURCE_MISC_GDI_COMPATIBLE;

		hr = m_pDevice->CreateTexture2D(
			&description,
			0, // no initial data
			&m_pTexture);
		assert(hr>=0);

		hr = m_pTexture->QueryInterface(&m_pSurface);
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

			hr = m_pD2DFactory->CreateDxgiSurfaceRenderTarget(
				m_pSurface,
				&properties,
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