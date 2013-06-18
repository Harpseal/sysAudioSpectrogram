#ifndef DIBWrapper_H_
#define DIBWrapper_H_

#include <Windows.h>

class GdiBitmap {
	const UINT m_width;
	const UINT m_height;
	const UINT m_stride;
	void* m_bits;
	HGDIOBJ m_oldBitmap;
public:
	HDC m_hdc;
	HBITMAP m_bitmap;

public:

	GdiBitmap(__in UINT width,
		__in UINT height) :
	m_width(width),
		m_height(height),
		m_stride((width * 32 + 31) / 32 * 4),
		m_bits(0),
		m_oldBitmap(0) {

			BITMAPINFO bitmapInfo = { };
			bitmapInfo.bmiHeader.biSize = 
				sizeof(bitmapInfo.bmiHeader);
			bitmapInfo.bmiHeader.biWidth = 
				width;
			bitmapInfo.bmiHeader.biHeight = 
				0 - height;
			bitmapInfo.bmiHeader.biPlanes = 1;
			bitmapInfo.bmiHeader.biBitCount = 32;
			bitmapInfo.bmiHeader.biCompression = 
				BI_RGB;

			m_bitmap = CreateDIBSection(
				0, // device context
				&bitmapInfo,
				DIB_RGB_COLORS,
				&m_bits,
				0, // file mapping object
				0);

			assert (0 != m_bits);

			m_hdc = CreateCompatibleDC(NULL);

			assert(0 != m_hdc);

			m_oldBitmap = SelectObject(m_hdc, m_bitmap);
	}

	~GdiBitmap() {

		SelectObject(m_hdc, m_oldBitmap);

		DeleteObject(m_bitmap);
		DeleteDC(m_hdc);
		m_hdc = 0;
	}

	UINT GetWidth() const {
		return m_width;
	}

	UINT GetHeight() const {
		return m_height;
	}

	UINT GetStride() const {
		return m_stride;
	}

	void* GetBits() const {
		return m_bits;
	}

	HDC GetDC() const {
		return m_hdc;
	}
};


#endif