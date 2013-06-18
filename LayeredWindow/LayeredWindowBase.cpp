#include "LayeredWindowBase.h"
#include "LayeredWindowD2DtoDXGI.h"
#include "LayeredWindowD2DtoGDI.h"
#include "LayeredWindowD2DtoWIC.h"
#include "LayeredWindowGDI.h"

#include <WinDef.h>
#include <tchar.h>

LayeredWindowBase::LayeredWindowBase(int width,int height,LayeredWindowTechType techType):m_info(width,height)
{
	m_techType = techType;
	m_hWnd = 0;
	m_hThreadMsg = 0;

	m_pWin.pAPI = NULL;

	m_hThreadMsg = ::CreateThread( NULL, 0, StartMsgThread, this, 0, NULL );
}

LayeredWindowBase::~LayeredWindowBase()
{
	SendMessage(m_hWnd, WM_DESTROY, 0, 0);
	WaitForSingleObject(m_hWnd, INFINITE);


}

bool LayeredWindowBase::CheckWindowState()
{
	return m_hThreadMsg != NULL && WaitForSingleObject(m_hThreadMsg, 0) == WAIT_TIMEOUT;
}

void LayeredWindowBase::Render()
{
	wchar_t pszbuf[] = L"1.Text Designer中文字也可顯示\n2.Text Designer中文字也可顯示\n3.Text Designer中文字也可顯示\n4.Text Designer中文字也可顯示\n5.Text Designer中文字也可顯示\n6.Text Designer中文字也可顯示 Text Designer中文字也可顯示\n7.Text Designer中文字也可顯示\n8.Text Designer中文字也可顯示\n9.Text Designer中文字也可顯示\n10.Text Designer中文字也可顯示";
	int nStrLen = wcslen(pszbuf);

	if (m_techType & LayeredWindow_TechType_D2D)
	{
		//m_pWin.pD2D->BeginDraw();
		m_pWin.pD2D->m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White,0.5));
		//m_pWin.pD2D->m_pRenderTarget->DrawText(pszbuf,nStrLen,
		//
		//	IFR(m_spDWriteFactory->CreateTextFormat(
		//	msc_fontName,
		//	NULL,
		//	DWRITE_FONT_WEIGHT_NORMAL,
		//	DWRITE_FONT_STYLE_NORMAL,
		//	DWRITE_FONT_STRETCH_NORMAL,
		//	msc_fontSize,
		//	L"", //locale
		//	&m_spTextFormat
		//	));

		//// Center the text both horizontally and vertically.
		//m_spTextFormat->SetTextAlignment(
		//	DWRITE_TEXT_ALIGNMENT_CENTER
		//	);

		//m_spTextFormat->SetParagraphAlignment(
		//	DWRITE_PARAGRAPH_ALIGNMENT_CENTER
		//	);

		//m_pWin.pD2D->m_pRenderTarget->DrawText(
		//	pszbuf,nStrLen,
		//	m_spTextFormat,
		//	D2D1::RectF(
		//	0,0,rtSize.width, rtSize.height
		//	),
		//	m_spBlackBrush
		//	);)
		//m_pWin.pD2D->EndDraw();
	}
	else if (m_techType & LayeredWindow_TechType_GDI)
	{
		//m_pWin.pGDI->BeginDraw();
		//Gdiplus::Graphics graphics( m_pBitmap->GetDC() ); //#MOD
		m_pWin.pGDI->m_pGraphics->Clear( Gdiplus::Color( 128, 255, 255, 255 ) );


		Gdiplus::FontFamily fontFamily(L"微軟正黑體");
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

		//SelectObject(m_bitmap.m_hdc, m_bitmap.m_bitmap);
		//m_pWin.pGDI->EndDraw();
	}

}

bool LayeredWindowBase::SetSize(int w,int h)
{
	if (w<m_info.m_minsize.cx) w=m_info.m_minsize.cx;
	if (h<m_info.m_minsize.cy) h=m_info.m_minsize.cy;
	if (m_info.m_size.cx == w && m_info.m_size.cy == h) return false;
	m_info.m_size.cx = w;
	m_info.m_size.cy = h;
	return true;
}

void LayeredWindowBase::UpdateLayeredWindow()
{
	BeforeRender();
	m_pWin.pAPI->BeginDraw();
	Render();
	if (m_techType == LayeredWindow_TechType_D2DtoGDI)
	{
		m_pWin.pAPI->EndDraw();
		m_info.Update(m_hWnd,m_pWin.pAPI->GetDC());
		m_pWin.pAPI->ReleaseDC();
		
	}
	else
	{
		m_info.Update(m_hWnd,m_pWin.pAPI->GetDC());
		m_pWin.pAPI->ReleaseDC();
		m_pWin.pAPI->EndDraw();
	}
	AfterRender();
}

LRESULT CALLBACK LayeredWindowBase::WndProcStatic(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LayeredWindowBase* pParent;
	// Get pointer to window
	if(uMsg == WM_CREATE)// || uMsg == WM_NCCREATE)
	{
		pParent = (LayeredWindowBase*)((LPCREATESTRUCT)lParam)->lpCreateParams;
		SetWindowLongPtr(hWnd,GWL_USERDATA,(LONG_PTR)pParent);
	}
	else
	{
		pParent = (LayeredWindowBase*)GetWindowLongPtr(hWnd,GWL_USERDATA);
		if(!pParent) return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}

	//printf("WndProcStatic : %X\n",uMsg);
	return pParent->WndProc(pParent->m_hWnd,uMsg,wParam,lParam);
}

LRESULT LayeredWindowBase::WndProc(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CREATE:
		//SetTimer(hWnd,1,100,NULL);
		if (Initialize())
			return NULL;
		else
			return -1;

	case WM_SIZE:
		{
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);
			
			//phLyricsScrollWindow->OnResize(width, height);
			if (width!=0 && height!=0)
			{
				if (SetSize(width,height))
				{
					m_pWin.pAPI->OnSize(width,height);
					RECT rect;
					GetWindowRect(m_hWnd,&rect);
					m_info.m_windowPosition.x = rect.left;
					m_info.m_windowPosition.y = rect.top;
					RedrawWindow(m_hWnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE);
				}
				printf("WM_SIZE : %d %d -> %d %d\n",width,height,m_pWin.pAPI->GetWidth(),m_pWin.pAPI->GetHeight());

			}

			


			//printf("onSize %d %d\n",width,height);
		}
		return 0;

	case WM_GETMINMAXINFO: //window size/position is going to change
		{
			MINMAXINFO* pInfo = (MINMAXINFO*)lParam;
			pInfo->ptMinTrackSize.x = (long)m_info.m_minsize.cx; //apply custom min width/height
			pInfo->ptMinTrackSize.y = (long)m_info.m_minsize.cy;
		}

		return 0;

	case WM_WINDOWPOSCHANGING:
	case WM_WINDOWPOSCHANGED:
		{
			RECT rect;
			GetWindowRect(m_hWnd,&rect);
			m_info.m_windowPosition.x = rect.left;
			m_info.m_windowPosition.y = rect.top;
		}
		return 0;

	case WM_ACTIVATE:
		UpdateLayeredWindow();
		if (hWnd == m_hWnd)
		{
			
			
				//BeforeRender();
				//m_pWin.pAPI->BeginDraw();
				//Render();
				//m_info.Update(m_hWnd,m_pWin.pAPI->GetDC());
				//m_pWin.pAPI->ReleaseDC();
				//m_pWin.pAPI->EndDraw();
				//AfterRender();
		}
		break;

	case WM_TIMER:
	case WM_PAINT:
	case WM_DISPLAYCHANGE:
		{
			//printf("Render!\n");
			//if (m_vRenderCBList.size())
			//{
			//	//((*m_pRanderCBClass).*m_pRanderCBFunc)();
			//	for (int i=0;i<m_vRenderCBList.size();i++)
			//	{
			//		(m_vRenderCBList[i].pClass->*(m_vRenderCBList[i].pCBFunc))();
			//		//(m_pRanderCBClass->*m_pRanderCBFunc)();
			//	}

			//	
			//}
			//else
			if (hWnd == m_hWnd)
			{
				BeforeRender();
				m_pWin.pAPI->BeginDraw();
				Render();
				m_info.Update(m_hWnd,m_pWin.pAPI->GetDC());
				m_pWin.pAPI->ReleaseDC();
				m_pWin.pAPI->EndDraw();
				AfterRender();
			}


		}
		return 0;

	case WM_DESTROY:
		{
			Uninitialize();
			PostQuitMessage(0);
		}
		return 0;
	case WM_COMMAND:
		break;
	case WM_KEYDOWN:
		//printf("%X key down\n",wParam);
		OnKeyboard(wParam,true);
		break;

	case WM_KEYUP:
		//printf("%X key up\n",wParam);
		OnKeyboard(wParam,false);
		break;

	case WM_MOUSEWHEEL:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
		{
			
			int x=(short)LOWORD(lParam);
			int y=(short)HIWORD(lParam);
			OnMouse(uMsg,x,y,GET_KEYSTATE_WPARAM(wParam),GET_WHEEL_DELTA_WPARAM(wParam));
			//printf("mouse event : %x @ (%d,%d)\n",uMsg,x,y);
		}
		
		break;

	case WM_NCMOUSEMOVE     :
	case WM_NCLBUTTONDOWN   :
	case WM_NCLBUTTONUP     :
	case WM_NCLBUTTONDBLCLK :
	case WM_NCRBUTTONDOWN   :
	case WM_NCRBUTTONUP     :
	case WM_NCRBUTTONDBLCLK :
	case WM_NCMBUTTONDOWN   :
	case WM_NCMBUTTONUP     :
	case WM_NCMBUTTONDBLCLK :
		{
			int x=(short)LOWORD(lParam);
			int y=(short)HIWORD(lParam);
			OnMouse(uMsg,x,y,0,0);
			//printf("NCmouse event : %x @ (%d,%d)\n",uMsg,x,y);
		}

		break;

	case WM_NCHITTEST:
		{
			//printf("Hit Test : %d\n",DefWindowProc(
			//	//hwnd,
			//	message,
			//	wParam,
			//	lParam
			//	//dwMsgMapID
			//	));
			//break;
			/*
			#define BF_LEFT         0x0001
			#define BF_TOP          0x0002
			#define BF_RIGHT        0x0004
			#define BF_BOTTOM       0x0008
			*/
			
			int x=(short)LOWORD(lParam);
			int y=(short)HIWORD(lParam);
			unsigned char type = 0;
			if (x-m_info.m_windowPosition.x<m_info.m_border)
				type|=BF_LEFT;
			if (y-m_info.m_windowPosition.y<m_info.m_border)
				type|=BF_TOP;
			if (m_info.m_windowPosition.x+m_info.m_size.cx-x<m_info.m_border)
				type|=BF_RIGHT;
			if (m_info.m_windowPosition.y+m_info.m_size.cy-y<m_info.m_border)
				type|=BF_BOTTOM;
			//printf("WM_NCHITTEST (%d,%d): pos (%d,%d)  size (%d,%d)  border %d\n",x,y,m_info.m_windowPosition.x,m_info.m_windowPosition.y,m_info.m_size.cx,m_info.m_size.cy,m_info.m_border);

			switch (type)
			{
			case BF_LEFT:
				return HTLEFT;
				break;
			case BF_RIGHT:
				return HTRIGHT;
				break;
			case BF_TOP:
				return HTTOP;
				break;
			case BF_BOTTOM:
				return HTBOTTOM;
				break;

			case BF_LEFT|BF_TOP:
				return HTTOPLEFT;
				break;
			case BF_LEFT|BF_BOTTOM:
				return HTBOTTOMLEFT;
				break;
			case BF_RIGHT|BF_TOP:
				return HTTOPRIGHT;
				break;
			case BF_RIGHT|BF_BOTTOM:
				return HTBOTTOMRIGHT;
				break;

			default:
				return HTCAPTION;


			}


			
			//printf("hit test : %d\n",res);
			
		}
		return 0;

	case WM_MOVING:
		{
			RECT *pRect = (RECT*)lParam;
			//printf("%d %d\n",pRect->left,pRect->top);
			m_info.m_windowPosition.x = pRect->left;
			m_info.m_windowPosition.y = pRect->top;
		}
		return 0;

	case WM_SIZING:
		{
			RECT *pRect = (RECT*)lParam;
			UINT width = pRect->right-pRect->left;
			UINT height = pRect->bottom-pRect->top;
			bool isMove = true;
			printf("%d %d\n",width,height);
			if (SetSize(width,height))
			{
				m_pWin.pAPI->OnSize(width,height);
				m_info.m_windowPosition.x = pRect->left;
				m_info.m_windowPosition.y = pRect->top;
				printf("WM_SIZING : %d %d -> %d %d\n",width,height,m_pWin.pAPI->GetWidth(),m_pWin.pAPI->GetHeight());
				//Render();
//				m_info.Update(m_hWnd,m_pWin.pAPI->GetDC());
				//m_pWin.pAPI->ReleaseDC();

				UpdateLayeredWindow();
				//m_pWin.pAPI->BeginDraw();
				//Render();
				//m_info.Update(m_hWnd,m_pWin.pAPI->GetDC());
				//m_pWin.pAPI->ReleaseDC();
				//m_pWin.pAPI->EndDraw();
				//RedrawWindow(m_hWnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE);
			}
			
		}
		return 0;

	default:
		break;
			//printf("msg %X\n",message);
	}
	// Call default window proc if we haven't handled the message
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

#include <locale.h>

DWORD WINAPI LayeredWindowBase::StartMsgThread( void* pParam )
{
	LayeredWindowBase* pWindow = reinterpret_cast<LayeredWindowBase*>(pParam );
	if ( !pWindow )
		return (DWORD)-1;
	HRESULT hr = S_OK;
	hr = CoInitialize(NULL);

	if (!SUCCEEDED(hr))
		return 0;

	::setlocale(LC_ALL, "cht"); 

	switch (pWindow->m_techType)
	{
	case LayeredWindow_TechType_GDI:
		pWindow->m_pWin.pGDI = new LayeredWindowGDI(pWindow->m_info.m_size.cx, pWindow->m_info.m_size.cy);
		break;
	case LayeredWindow_TechType_D2D:
		pWindow->m_techType = LayeredWindow_TechType_D2DtoDXGI;
	case LayeredWindow_TechType_D2DtoDXGI:
		pWindow->m_pWin.pD2DtoDXGI = new LayeredWindowD2DtoDXGI(pWindow->m_info.m_size.cx, pWindow->m_info.m_size.cy);
		break;
	case LayeredWindow_TechType_D2DtoGDI:
		pWindow->m_pWin.pD2DtoGDI = new LayeredWindowD2DtoGDI(pWindow->m_info.m_size.cx, pWindow->m_info.m_size.cy);
		break;
	case LayeredWindow_TechType_D2DtoWIC:
		pWindow->m_pWin.pD2DtoWIC = new LayeredWindowD2DtoWIC(pWindow->m_info.m_size.cx, pWindow->m_info.m_size.cy);
		break;
	default:
		pWindow->m_pWin.pAPI = NULL;
		break;
	}

	//RegisterClassEx
	WNDCLASSEX wcex = { 0 };

	wcex.cbSize          = sizeof(WNDCLASSEX);
	wcex.style           = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc     = WndProcStatic;
	wcex.cbClsExtra      = 0;
	wcex.cbWndExtra      = 8; // 8 bytes, to allow for 64-bit architecture
	wcex.hInstance       = NULL; // CHECK
	wcex.hIcon           = (HICON)LoadImageW(NULL, _T("sysAudioSpectrogram_icon.ico"), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE|LR_LOADFROMFILE);
	wcex.hCursor         = ::LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground   = (HBRUSH)NULL_BRUSH; // CHECK
	wcex.lpszMenuName    = NULL;
	wcex.lpszClassName   = _T("LayeredWindowBaseClass");
	wcex.hIconSm         = NULL;

	::RegisterClassEx(&wcex);


	//CreateWindow
	pWindow->m_hWnd = ::CreateWindowEx( WS_EX_LAYERED | WS_EX_TOPMOST, _T("LayeredWindowBaseClass"), _T("LayeredWindow"), WS_POPUP, 
		pWindow->m_info.m_windowPosition.x, pWindow->m_info.m_windowPosition.y,
		pWindow->m_info.m_size.cx, pWindow->m_info.m_size.cy, NULL, NULL, NULL, pWindow ); 

	//InitDraw
	ShowWindow(pWindow->m_hWnd,SW_SHOW);
	//RedrawWindow(pWindow->m_hWnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN) ;
	//RedrawWindow(pWindow->m_hWnd, NULL, NULL, RDW_INVALIDATE) ;

	//Msg Loop
	MSG msg = { 0 };
	while ( ::GetMessage( &msg, NULL, 0, 0 ) )
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	//Closing window
	::UnregisterClass( _T("LayeredWindowBaseClass"), NULL );


	switch (pWindow->m_techType)
	{
	case LayeredWindow_TechType_GDI:
		delete pWindow->m_pWin.pGDI;
		break;
	case LayeredWindow_TechType_D2DtoDXGI:
		delete pWindow->m_pWin.pD2DtoDXGI;
		break;
	case LayeredWindow_TechType_D2DtoGDI:
		delete pWindow->m_pWin.pD2DtoGDI;
		break;
	case LayeredWindow_TechType_D2DtoWIC:
		delete pWindow->m_pWin.pD2DtoWIC;
		break;
	default:
		pWindow->m_pWin.pAPI = NULL;
		break;
	}
	CoUninitialize();

	printf("LayeredWindowBase thread closed.\n");
	return 0;
}

