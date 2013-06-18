#include "AudioBuffer.h"
#include <string>
#include <assert.h>
#include "loopback-capture\loopback-capture.h"
#include "AudioSpectrogram.h"
#include "AudioSpectrogramWindow.h"

//#ifndef _DEBUG
//#pragma comment(linker, "/subsystem:windows /entry:wmainCRTStartup")
//#endif
#ifdef _DEBUG
int _cdecl wmain(int argc, LPCWSTR argv[])
#else
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#endif
//int _cdecl wmain(int argc, LPCWSTR argv[])
{
	//AudioSpectrogramWindow lwin(400,600,LayeredWindowBase::LayeredWindow_TechType_D2DtoWIC);
	//Sleep(4000);
	//lwin.Repaint();
	//printf("RedrawWindow\n");
	////RedrawWindow(lwin.m_hWnd, NULL, NULL, RDW_INTERNALPAINT) ;
	////printf("SendMessage : WM_PAINT\n");
	//
	//while(lwin.CheckWindowState()) 
	//{
	//	//printf("%d\n",);
	//	Sleep(1000);
	//	
	//	//SendMessage(, WM_SETREDRAW, false, 0);
	//}
	//return 0;
	
	HRESULT hr = S_OK;
	hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        printf("CoInitialize failed: hr = 0x%08x", hr);
        return -__LINE__;
    }

	AudioBuffer audioBuffer;
	AudioSpectrogram audioSpec(4096*2,&audioBuffer,800,400);
	LoopbackCapture *pLoopbackCap = NULL;
	
	bool isDone = false;
	
	audioSpec.Start();

	while(!isDone)
	{
		if (pLoopbackCap==NULL)
		{
			pLoopbackCap = new LoopbackCapture(&audioBuffer);
			pLoopbackCap->Start();
		}
		
		isDone = pLoopbackCap->WaitCapture();
		if (!isDone)
		{
			delete pLoopbackCap;
			pLoopbackCap = NULL;
		}
	}
	if (pLoopbackCap)
		delete pLoopbackCap;
	CoUninitialize();
	return 0;
}