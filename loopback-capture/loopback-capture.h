// silence.h

// call CreateThread on this function
// feed it the address of a LoopbackCaptureThreadFunctionArguments
// it will capture via loopback from the IMMDevice
// and dump output to the HMMIO
// until the stop event is set
// any failures will be propagated back via hr

#ifndef LOOPBACK_CAPTURE_H_
#define LOOPBACK_CAPTURE_H_

#include "..\ThreadBase.h"

class AudioBuffer;
#include <mmsystem.h>
#include <mmdeviceapi.h>


//struct LoopbackCaptureThreadFunctionArguments {
//    IMMDevice *pMMDevice;
//    bool bInt16;
//    HMMIO *phFile;
//    HANDLE hStartedEvent;
//    HANDLE hStopEvent;
//    UINT32 nFrames;
//    HRESULT hr;
//
//	SampleBuffer* pBuffer;
//};


class LoopbackCapture : public ThreadBase
{
public:
	LoopbackCapture(AudioBuffer *pBuffer);
	~LoopbackCapture();

	void Start();
	bool WaitCapture();

private:
	IMMDevice *pMMDevice;
    bool bInt16;
    HMMIO hFile;
    HANDLE hStartedEvent;
    HANDLE hStopEvent;
	HANDLE hDeviceEvent;
	HANDLE hGlobalCloseEvent;
    UINT nFrames;
    HRESULT hr;

	AudioBuffer* pBuffer;


	static DWORD WINAPI LoopbackCaptureThreadFunction(LPVOID pContext);
	HRESULT Process();
};

#endif

