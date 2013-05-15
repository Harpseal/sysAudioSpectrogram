#include "AudioBuffer.h"
#include <string>
#include <assert.h>
#include "loopback-capture\loopback-capture.h"
#include "AudioSpectrogram.h"

//#ifndef _DEBUG
//#pragma comment(linker, "/subsystem:windows /entry:wmainCRTStartup")
//#endif
#ifdef _DEBUG
int _cdecl wmain(int argc, LPCWSTR argv[])
#else
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#endif

{
	HRESULT hr = S_OK;
	hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        printf("CoInitialize failed: hr = 0x%08x", hr);
        return -__LINE__;
    }

	AudioBuffer audioBuffer;
	AudioSpectrogram audioSpec(4096*2,&audioBuffer,400,800);
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