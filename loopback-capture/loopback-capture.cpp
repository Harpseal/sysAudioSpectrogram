// silence.cpp

#include <windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdio.h>
#include <avrt.h>

#include "loopback-capture.h"
#include "..\AudioBuffer.h"
#include <string>


#include "..\AudioDeviceNotificationClient.h"
#include "..\AudioSpectrogramDebug.cpp"

template<class T> void ShowPCM(T* pData,int nData,int nChannels,int maxWidth,int rowHeight,char* name);

HRESULT WriteWaveHeader(HMMIO hFile, LPCWAVEFORMATEX pwfx, MMCKINFO *pckRIFF, MMCKINFO *pckData);
HRESULT FinishWaveFile(HMMIO hFile, MMCKINFO *pckRIFF, MMCKINFO *pckData);

HRESULT get_specific_device(LPCWSTR szLongName, IMMDevice **ppMMDevice);
HRESULT get_default_device(IMMDevice **ppMMDevice);
HRESULT list_devices();
HRESULT register_notification_callback(IMMNotificationClient *pClient);
HRESULT unregister_notification_callback(IMMNotificationClient *pClient);
HRESULT get_specific_device(LPCWSTR szLongName, IMMDevice **ppMMDevice);

LoopbackCapture::LoopbackCapture(AudioBuffer *pBuffer)
{
	//memset(this,0,sizeof(this));
	this->pBuffer = pBuffer;

	//hr = CoInitialize(NULL);

	// create a "loopback capture has started" event
    hStartedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == hStartedEvent) {
        printf("CreateEvent failed: last error is %u\n", GetLastError());
        //return -__LINE__;
    }

    // create a "stop capturing now" event
    hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == hStopEvent) {
        printf("CreateEvent failed: last error is %u\n", GetLastError());
        CloseHandle(hStartedEvent);
        //return -__LINE__;
    }

	hDeviceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == hDeviceEvent) {
        printf("CreateEvent failed: last error is %u\n", GetLastError());
        CloseHandle(hStartedEvent);
		CloseHandle(hStopEvent);
        //return -__LINE__;
    }

	hGlobalCloseEvent = CreateEvent(NULL, FALSE, FALSE, L"sysAudioSpectrogram_GlobalCloseEvent");
    if (NULL == hGlobalCloseEvent) {
        printf("CreateEvent failed: last error is %u\n", GetLastError());
        CloseHandle(hStartedEvent);
		CloseHandle(hStopEvent);
		CloseHandle(hDeviceEvent);
        //return -__LINE__;
    }

    hr = E_UNEXPECTED; // thread will overwrite this
	get_default_device(&pMMDevice);
    //pMMDevice = prefs.m_pMMDevice;
    bInt16 = false;//prefs.m_bInt16;
    hFile = NULL;//prefs.m_hFile;
    nFrames = 0;

	//CoUninitialize();

}


LoopbackCapture::~LoopbackCapture()
{
	Termiate();
}

//HRESULT LoopbackCapture(
//    IMMDevice *pMMDevice,
//    bool bInt16,
//    HANDLE hStartedEvent,
//    HANDLE hStopEvent,
//    PUINT32 pnFrames,
//	HMMIO hFile,
//	AudioBuffer *pBuffer
//);


void LoopbackCapture::Start()
{
	Termiate();
	m_hThread  = CreateThread(
        NULL, 0,
        LoopbackCapture::LoopbackCaptureThreadFunction, this,
        0, NULL
    );

	if (NULL == m_hThread) {
        printf("CreateThread failed: last error is %u\n", GetLastError());
        CloseHandle(hStopEvent);
        CloseHandle(hStartedEvent);
        //return -__LINE__;
    }

    // wait for either capture to start or the thread to end
    HANDLE waitArray[2] = { hStartedEvent, m_hThread };
    DWORD dwWaitResult;
    dwWaitResult = WaitForMultipleObjects(
        ARRAYSIZE(waitArray), waitArray,
        FALSE, INFINITE
    );

    if (WAIT_OBJECT_0 + 1 == dwWaitResult) {
        printf("Thread aborted before starting to loopback capture: hr = 0x%08x\n", hr);
        CloseHandle(hStartedEvent);
        CloseHandle(m_hThread);m_hThread=NULL;
        CloseHandle(hStopEvent);
        //return -__LINE__;
    }

    if (WAIT_OBJECT_0 != dwWaitResult) {
        printf("Unexpected WaitForMultipleObjects return value %u", dwWaitResult);
        CloseHandle(hStartedEvent);
        CloseHandle(m_hThread);m_hThread=NULL;
        CloseHandle(hStopEvent);
        //return -__LINE__;
    }

    CloseHandle(hStartedEvent);


}


bool LoopbackCapture::WaitCapture()
{
	if (m_hThread == NULL) return false;

//	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwWaitResult;

  //  if (INVALID_HANDLE_VALUE == hStdIn) {
  //      printf("GetStdHandle returned INVALID_HANDLE_VALUE: last error is %u\n", GetLastError());
  //      SetEvent(hStopEvent);
  //      WaitForSingleObject(m_hThread, INFINITE);
  //      CloseHandle(hStartedEvent);
  //      CloseHandle(m_hThread);m_hThread=NULL;
  //      CloseHandle(hStopEvent);
		//return false;
  //      //return -__LINE__;
  //  }

	// wait for the thread to terminate early
    // or for the user to press (and release) Enter
	//HANDLE rhHandles[4] = { m_hThread, hStdIn , hDeviceEvent , hGlobalCloseEvent};
	HANDLE rhHandles[3] = { m_hThread, hDeviceEvent , hGlobalCloseEvent};

    bool bKeepWaiting = true;
	bool bIsCloseCapture = true;
    while (bKeepWaiting) {

        dwWaitResult = WaitForMultipleObjects(3, rhHandles, FALSE, INFINITE);

        switch (dwWaitResult) {

            case WAIT_OBJECT_0: // hThread
                printf("The thread terminated early - something bad happened\n");
                bKeepWaiting = false;
				bIsCloseCapture = false;
                break;

            //case WAIT_OBJECT_0 + 1: // hStdIn
            //    // see if any of them was an Enter key-up event
            //    INPUT_RECORD rInput[128];
            //    DWORD nEvents;
            //    if (!ReadConsoleInput(hStdIn, rInput, ARRAYSIZE(rInput), &nEvents)) {
            //        printf("ReadConsoleInput failed: last error is %u\n", GetLastError());
            //        SetEvent(hStopEvent);
            //        WaitForSingleObject(m_hThread, INFINITE);
            //        bKeepWaiting = false;
            //    } else {
            //        for (DWORD i = 0; i < nEvents; i++) {
            //            if (
            //                KEY_EVENT == rInput[i].EventType &&
            //                VK_RETURN == rInput[i].Event.KeyEvent.wVirtualKeyCode &&
            //                !rInput[i].Event.KeyEvent.bKeyDown
            //             ) {
            //                printf("Stopping capture...\n");
            //                SetEvent(hStopEvent);
            //                WaitForSingleObject(m_hThread, INFINITE);
            //                bKeepWaiting = false;
            //                break;
            //            }
            //        }
            //        // if none of them were Enter key-up events,
            //        // continue waiting
            //    }
            //    break;

			case WAIT_OBJECT_0 + 1:
				printf("DeviceChange!!\n");
                SetEvent(hStopEvent);
                WaitForSingleObject(m_hThread, INFINITE);
                bKeepWaiting = false;
				bIsCloseCapture = false;
                break;

			case WAIT_OBJECT_0 + 2:
                SetEvent(hStopEvent);
                WaitForSingleObject(m_hThread, INFINITE);
                bKeepWaiting = false;
				bIsCloseCapture = true;
                break;

            default:
                printf("WaitForMultipleObjects returned unexpected value 0x%08x\n", dwWaitResult);
                SetEvent(hStopEvent);
                WaitForSingleObject(m_hThread, INFINITE);
                bKeepWaiting = false;
				bIsCloseCapture = false;
                break;
        }
    }

    DWORD exitCode;
    if (!GetExitCodeThread(m_hThread, &exitCode)) {
        printf("GetExitCodeThread failed: last error is %u\n", GetLastError());
        CloseHandle(m_hThread);
        CloseHandle(hStopEvent);
		CloseHandle(hDeviceEvent);
		return false;
        //return -__LINE__;
    }

    if (0 != exitCode) {
        printf("Loopback capture thread exit code is %u; expected 0\n", exitCode);
        CloseHandle(m_hThread);
        CloseHandle(hStopEvent);
		CloseHandle(hDeviceEvent);
		return false;
        //return -__LINE__;
    }

    if (S_OK != hr) {
        printf("Thread HRESULT is 0x%08x\n", hr);
        CloseHandle(m_hThread);
        CloseHandle(hStopEvent);
		CloseHandle(hDeviceEvent);
		return false;
        //return -__LINE__;
    }

    CloseHandle(m_hThread);
    CloseHandle(hStopEvent);
	CloseHandle(hDeviceEvent);

	if (hFile!=NULL)
	{
		// everything went well... fixup the fact chunk in the file
		MMRESULT result = mmioClose(hFile, 0);
		hFile = NULL;
		if (MMSYSERR_NOERROR != result) {
			printf("mmioClose failed: MMSYSERR = %u\n", result);
			return false;//return -__LINE__;
		}

		// reopen the file in read/write mode
		MMIOINFO mi = {0};
		//hFile = mmioOpen(const_cast<LPWSTR>(prefs.m_szFilename), &mi, MMIO_READWRITE);
		hFile = mmioOpen(const_cast<LPWSTR>(L"Output.wav"), &mi, MMIO_READWRITE);
		if (NULL == hFile) {
			//printf("mmioOpen(\"%ls\", ...) failed. wErrorRet == %u\n", prefs.m_szFilename, mi.wErrorRet);
			return false;
			//return -__LINE__;
		}

		// descend into the RIFF/WAVE chunk
		MMCKINFO ckRIFF = {0};
		ckRIFF.ckid = MAKEFOURCC('W', 'A', 'V', 'E'); // this is right for mmioDescend
		result = mmioDescend(hFile, &ckRIFF, NULL, MMIO_FINDRIFF);
		if (MMSYSERR_NOERROR != result) {
			printf("mmioDescend(\"WAVE\") failed: MMSYSERR = %u\n", result);
			return false;//return -__LINE__;
		}

		// descend into the fact chunk
		MMCKINFO ckFact = {0};
		ckFact.ckid = MAKEFOURCC('f', 'a', 'c', 't');
		result = mmioDescend(hFile, &ckFact, &ckRIFF, MMIO_FINDCHUNK);
		if (MMSYSERR_NOERROR != result) {
			printf("mmioDescend(\"fact\") failed: MMSYSERR = %u\n", result);
			return false;//return -__LINE__;
		}

		// write the correct data to the fact chunk
		LONG lBytesWritten = mmioWrite(
			hFile,
			reinterpret_cast<PCHAR>(&nFrames),
			sizeof(nFrames)
		);
		if (lBytesWritten != sizeof(nFrames)) {
			printf("Updating the fact chunk wrote %u bytes; expected %u\n", lBytesWritten, (UINT32)sizeof(nFrames));
			return false;//return -__LINE__;
		}

		// ascend out of the fact chunk
		result = mmioAscend(hFile, &ckFact, 0);
		if (MMSYSERR_NOERROR != result) {
			printf("mmioAscend(\"fact\") failed: MMSYSERR = %u\n", result);
			return false;//return -__LINE__;
		}
	}
	return bIsCloseCapture;
}

DWORD WINAPI LoopbackCapture::LoopbackCaptureThreadFunction(LPVOID pContext) {
    LoopbackCapture *pCap =
        (LoopbackCapture*)pContext;

    pCap->hr = CoInitialize(NULL);
    if (FAILED(pCap->hr)) {
        printf("CoInitialize failed: hr = 0x%08x\n", pCap->hr);
        return 0;
    }

	CMMNotificationClient NClient;

	NClient.SetDeviceChangeEventHandle(pCap->hDeviceEvent);
	register_notification_callback(&NClient);

	pCap->hr = pCap->Process();

	unregister_notification_callback(&NClient);
  //  pArgs->hr = LoopbackCapture(
  //      pArgs->pMMDevice,
  //      pArgs->bInt16,
  //      pArgs->hStartedEvent,
  //      pArgs->hStopEvent,
  //      &pArgs->nFrames,
		//pArgs->phFile,
		//pArgs->pBuffer
  //  );

    CoUninitialize();
    return 0;
}

//HRESULT LoopbackCapture(
//    IMMDevice *pMMDevice,
//    bool bInt16,
//    HANDLE hStartedEvent,
//    HANDLE hStopEvent,
//    PUINT32 pnFrames,
//	HMMIO hFile,
//	AudioBuffer *pBuffer
//)
HRESULT LoopbackCapture::Process()
{
    HRESULT hr;

    // activate an IAudioClient
    IAudioClient *pAudioClient;
    hr = pMMDevice->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL, NULL,
        (void**)&pAudioClient
    );
    if (FAILED(hr)) {
        printf("IMMDevice::Activate(IAudioClient) failed: hr = 0x%08x", hr);
        return hr;
    }
    
    // get the default device periodicity
    REFERENCE_TIME hnsDefaultDevicePeriod;
    hr = pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, NULL);
    if (FAILED(hr)) {
        printf("IAudioClient::GetDevicePeriod failed: hr = 0x%08x\n", hr);
        pAudioClient->Release();
        return hr;
    }

    // get the default device format
    WAVEFORMATEX *pwfx;
    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) {
        printf("IAudioClient::GetMixFormat failed: hr = 0x%08x\n", hr);
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        return hr;
    }

	if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
		//pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
		printf("WAVE_FORMAT_EXTENSIBLE\n");
		if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat))
		{
			printf("float\n");
		}//
		else if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_PCM, pEx->SubFormat))
		{
			printf("PCM\n");
		}//KSDATAFORMAT_SUBTYPE_WAVEFORMATEX
		else if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_WAVEFORMATEX, pEx->SubFormat))
		{
			printf("WAVEFORMATEX\n");
		}
	}

    if (bInt16) {
        // coerce int-16 wave format
        // can do this in-place since we're not changing the size of the format
        // also, the engine will auto-convert from float to int for us
        switch (pwfx->wFormatTag) {
            case WAVE_FORMAT_IEEE_FLOAT:
                pwfx->wFormatTag = WAVE_FORMAT_PCM;
                pwfx->wBitsPerSample = 16;
                pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
                pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
                break;

            case WAVE_FORMAT_EXTENSIBLE:
                {
                    // naked scope for case-local variable
                    PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
                    if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat)) {
                        pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
                        pEx->Samples.wValidBitsPerSample = 16;
                        pwfx->wBitsPerSample = 16;
                        pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
                        pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
                    } else {
                        printf("Don't know how to coerce mix format to int-16\n");
                        CoTaskMemFree(pwfx);
                        pAudioClient->Release();
                        return E_UNEXPECTED;
                    }
                }
                break;

            default:
                printf("Don't know how to coerce WAVEFORMATEX with wFormatTag = 0x%08x to int-16\n", pwfx->wFormatTag);
                CoTaskMemFree(pwfx);
                pAudioClient->Release();
                return E_UNEXPECTED;
        }
    }

    MMCKINFO ckRIFF = {0};
    MMCKINFO ckData = {0};
	if (hFile!=NULL)
		hr = WriteWaveHeader(hFile, pwfx, &ckRIFF, &ckData);
	if (pBuffer)
	{
		bool isFloat = false;
		switch (pwfx->wFormatTag) {
            case WAVE_FORMAT_IEEE_FLOAT:
                isFloat = true;
                break;

            case WAVE_FORMAT_EXTENSIBLE:
                {
                    // naked scope for case-local variable
                    PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
                    if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat)) {
                        isFloat = true;
                    }
                }
                break;
            default:
                break;
        }
		pBuffer->SetAudioInfo(pwfx->nBlockAlign,pwfx->nChannels,pwfx->wBitsPerSample,isFloat);
	}

    if (FAILED(hr)) {
        // WriteWaveHeader does its own logging
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        return hr;
    }

    // create a periodic waitable timer
    HANDLE hWakeUp = CreateWaitableTimer(NULL, FALSE, NULL);
    if (NULL == hWakeUp) {
        DWORD dwErr = GetLastError();
        printf("CreateWaitableTimer failed: last error = %u\n", dwErr);
        CoTaskMemFree(pwfx);
        pAudioClient->Release();
        return HRESULT_FROM_WIN32(dwErr);
    }

    UINT32 nBlockAlign = pwfx->nBlockAlign;
	UINT32 nChannels = pwfx->nChannels;
    nFrames = 0;
    
    // call IAudioClient::Initialize
    // note that AUDCLNT_STREAMFLAGS_LOOPBACK and AUDCLNT_STREAMFLAGS_EVENTCALLBACK
    // do not work together...
    // the "data ready" event never gets set
    // so we're going to do a timer-driven loop
    hr = pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        0, 0, pwfx, 0
    );
    if (FAILED(hr)) {
        printf("IAudioClient::Initialize failed: hr = 0x%08x\n", hr);
        CloseHandle(hWakeUp);
        pAudioClient->Release();
        return hr;
    }
    CoTaskMemFree(pwfx);

    // activate an IAudioCaptureClient
    IAudioCaptureClient *pAudioCaptureClient;
    hr = pAudioClient->GetService(
        __uuidof(IAudioCaptureClient),
        (void**)&pAudioCaptureClient
    );
    if (FAILED(hr)) {
        printf("IAudioClient::GetService(IAudioCaptureClient) failed: hr 0x%08x\n", hr);
        CloseHandle(hWakeUp);
        pAudioClient->Release();
        return hr;
    }
    
    // register with MMCSS
    DWORD nTaskIndex = 0;
    HANDLE hTask = AvSetMmThreadCharacteristics(L"Capture", &nTaskIndex);
    if (NULL == hTask) {
        DWORD dwErr = GetLastError();
        printf("AvSetMmThreadCharacteristics failed: last error = %u\n", dwErr);
        pAudioCaptureClient->Release();
        CloseHandle(hWakeUp);
        pAudioClient->Release();
        return HRESULT_FROM_WIN32(dwErr);
    }    

    // set the waitable timer
    LARGE_INTEGER liFirstFire;
    liFirstFire.QuadPart = -hnsDefaultDevicePeriod / 2; // negative means relative time
    LONG lTimeBetweenFires = (LONG)hnsDefaultDevicePeriod / 2 / (10 * 1000); // convert to milliseconds
    BOOL bOK = SetWaitableTimer(
        hWakeUp,
        &liFirstFire,
        lTimeBetweenFires,
        NULL, NULL, FALSE
    );
    if (!bOK) {
        DWORD dwErr = GetLastError();
        printf("SetWaitableTimer failed: last error = %u\n", dwErr);
        AvRevertMmThreadCharacteristics(hTask);
        pAudioCaptureClient->Release();
        CloseHandle(hWakeUp);
        pAudioClient->Release();
        return HRESULT_FROM_WIN32(dwErr);
    }
    
    // call IAudioClient::Start
    hr = pAudioClient->Start();
    if (FAILED(hr)) {
        printf("IAudioClient::Start failed: hr = 0x%08x\n", hr);
        AvRevertMmThreadCharacteristics(hTask);
        pAudioCaptureClient->Release();
        CloseHandle(hWakeUp);
        pAudioClient->Release();
        return hr;
    }
    SetEvent(hStartedEvent);
    
    // loopback capture loop
    HANDLE waitArray[2] = { hStopEvent, hWakeUp };
    DWORD dwWaitResult;
	DWORD immdState;

    bool bDone = false;
    bool bFirstPacket = true;
    for (UINT32 nPasses = 0; !bDone; nPasses++) {
        dwWaitResult = WaitForMultipleObjects(
            ARRAYSIZE(waitArray), waitArray,
            FALSE, INFINITE
        );

        if (WAIT_OBJECT_0 == dwWaitResult) {
            //printf("Received stop event after %u passes and %u frames\n", nPasses, nFrames);
            bDone = true;
            continue; // exits loop
        }

        if (WAIT_OBJECT_0 + 1 != dwWaitResult) {
            printf("Unexpected WaitForMultipleObjects return value %u on pass %u after %u frames\n", dwWaitResult, nPasses, nFrames);
            pAudioClient->Stop();
            CancelWaitableTimer(hWakeUp);
            AvRevertMmThreadCharacteristics(hTask);
            pAudioCaptureClient->Release();
            CloseHandle(hWakeUp);
            pAudioClient->Release();
            return E_UNEXPECTED;
        }

		printf("'");

        // got a "wake up" event - see if there's data
        UINT32 nNextPacketSize;
        hr = pAudioCaptureClient->GetNextPacketSize(&nNextPacketSize);
        if (FAILED(hr)) {
			if (hr == AUDCLNT_E_SERVICE_NOT_RUNNING)
				printf("AUDCLNT_E_SERVICE_NOT_RUNNING : \n");
			else if (hr == AUDCLNT_E_DEVICE_INVALIDATED)
				printf("AUDCLNT_E_DEVICE_INVALIDATED : \n");
			else
				printf("UNKNOWN ERROR!!! : \n");

            printf("IAudioCaptureClient::GetNextPacketSize failed on pass %u after %u frames: hr = 0x%08x\n", nPasses, nFrames, hr);
            pAudioClient->Stop();
            CancelWaitableTimer(hWakeUp);
            AvRevertMmThreadCharacteristics(hTask);
            pAudioCaptureClient->Release();
            CloseHandle(hWakeUp);
            pAudioClient->Release();            
            return hr;
        }

        if (0 == nNextPacketSize) {
            // no data yet
            continue;
        }

        // get the captured data
        BYTE *pData;
        UINT32 nNumFramesToRead;
        DWORD dwFlags;

        hr = pAudioCaptureClient->GetBuffer(
            &pData,
            &nNumFramesToRead,
            &dwFlags,
            NULL,
            NULL
        );
        if (FAILED(hr)) {
            printf("IAudioCaptureClient::GetBuffer failed on pass %u after %u frames: hr = 0x%08x\n", nPasses, nFrames, hr);
            pAudioClient->Stop();
            CancelWaitableTimer(hWakeUp);
            AvRevertMmThreadCharacteristics(hTask);
            pAudioCaptureClient->Release();
            CloseHandle(hWakeUp);
            pAudioClient->Release();            
            return hr;            
        }

        if (bFirstPacket && AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY == dwFlags) {
            printf("Probably spurious glitch reported on first packet\n");
        } 
		else if (dwFlags & AUDCLNT_BUFFERFLAGS_SILENT)
		{
			printf("#");
		}
		else if (dwFlags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY)
		{
			printf("!");
		}
		else if (0 != dwFlags) {
            printf("IAudioCaptureClient::GetBuffer set flags to 0x%08x on pass %u after %u frames\n", dwFlags, nPasses, nFrames);
            pAudioClient->Stop();
            CancelWaitableTimer(hWakeUp);
            AvRevertMmThreadCharacteristics(hTask);
            pAudioCaptureClient->Release();
            CloseHandle(hWakeUp);
            pAudioClient->Release();            
            return E_UNEXPECTED;
        }

		if (0 == nNumFramesToRead) {
            // no data yet
            continue;
        }

        //if (0 == nNumFramesToRead) {
        //    printf("IAudioCaptureClient::GetBuffer said to read 0 frames on pass %u after %u frames\n", nPasses, nFrames);
        //    pAudioClient->Stop();
        //    CancelWaitableTimer(hWakeUp);
        //    AvRevertMmThreadCharacteristics(hTask);
        //    pAudioCaptureClient->Release();
        //    CloseHandle(hWakeUp);
        //    pAudioClient->Release();            
        //    return E_UNEXPECTED;            
        //}

        LONG lBytesToWrite = nNumFramesToRead * nBlockAlign;
#pragma prefast(suppress: __WARNING_INCORRECT_ANNOTATION, "IAudioCaptureClient::GetBuffer SAL annotation implies a 1-byte buffer")

		if (hFile!=NULL)
		{
			LONG lBytesWritten = mmioWrite(hFile, reinterpret_cast<PCHAR>(pData), lBytesToWrite);
			if (lBytesToWrite != lBytesWritten) {
				printf("mmioWrite wrote %u bytes on pass %u after %u frames: expected %u bytes\n", lBytesWritten, nPasses, nFrames, lBytesToWrite);
				pAudioClient->Stop();
				CancelWaitableTimer(hWakeUp);
				AvRevertMmThreadCharacteristics(hTask);
				pAudioCaptureClient->Release();
				CloseHandle(hWakeUp);
				pAudioClient->Release();            
				return E_UNEXPECTED;
			}
		}
		if (pBuffer)
		{
			//switch (nBlockAlign/nChannels)
			//{
			//case 1:
			//	ShowPCM((unsigned char*)pData,nNumFramesToRead,nChannels,1024,60,"SYS_Byte");
			//	break;
			//case 2:
			//	ShowPCM((short*)pData,nNumFramesToRead,nChannels,1024,60,"SYS_Short");
			//	break;
			//case 4:
			//	ShowPCM((int*)pData,nNumFramesToRead,nChannels,1024,60,"SYS_Int");
			//	//ShowPCM((float*)pData,nNumFramesToRead,nChannels,1024,60,"SYS_float");
			//	break;
			//}
		
			
			pBuffer->PushBuffer(pData,lBytesToWrite);
		}
        nFrames += nNumFramesToRead;
        
        hr = pAudioCaptureClient->ReleaseBuffer(nNumFramesToRead);
        if (FAILED(hr)) {
            printf("IAudioCaptureClient::ReleaseBuffer failed on pass %u after %u frames: hr = 0x%08x\n", nPasses, nFrames, hr);
            pAudioClient->Stop();
            CancelWaitableTimer(hWakeUp);
            AvRevertMmThreadCharacteristics(hTask);
            pAudioCaptureClient->Release();
            CloseHandle(hWakeUp);
            pAudioClient->Release();            
            return hr;            
        }
        
        bFirstPacket = false;
    } // capture loop

	if (hFile!=NULL)
		hr = FinishWaveFile(hFile, &ckData, &ckRIFF);


    if (FAILED(hr)) {
        // FinishWaveFile does it's own logging
        pAudioClient->Stop();
        CancelWaitableTimer(hWakeUp);
        AvRevertMmThreadCharacteristics(hTask);
        pAudioCaptureClient->Release();
        CloseHandle(hWakeUp);
        pAudioClient->Release();
        return hr;
    }
    
    pAudioClient->Stop();
    CancelWaitableTimer(hWakeUp);
    AvRevertMmThreadCharacteristics(hTask);
    pAudioCaptureClient->Release();
    CloseHandle(hWakeUp);
    pAudioClient->Release();

    return hr;
}

HRESULT WriteWaveHeader(HMMIO hFile, LPCWAVEFORMATEX pwfx, MMCKINFO *pckRIFF, MMCKINFO *pckData) {
    MMRESULT result;

    // make a RIFF/WAVE chunk
    pckRIFF->ckid = MAKEFOURCC('R', 'I', 'F', 'F');
    pckRIFF->fccType = MAKEFOURCC('W', 'A', 'V', 'E');

    result = mmioCreateChunk(hFile, pckRIFF, MMIO_CREATERIFF);
    if (MMSYSERR_NOERROR != result) {
        printf("mmioCreateChunk(\"RIFF/WAVE\") failed: MMRESULT = 0x%08x\n", result);
        return E_FAIL;
    }
    
    // make a 'fmt ' chunk (within the RIFF/WAVE chunk)
    MMCKINFO chunk;
    chunk.ckid = MAKEFOURCC('f', 'm', 't', ' ');
    result = mmioCreateChunk(hFile, &chunk, 0);
    if (MMSYSERR_NOERROR != result) {
        printf("mmioCreateChunk(\"fmt \") failed: MMRESULT = 0x%08x\n", result);
        return E_FAIL;
    }

    // write the WAVEFORMATEX data to it
    LONG lBytesInWfx = sizeof(WAVEFORMATEX) + pwfx->cbSize;
    LONG lBytesWritten =
        mmioWrite(
            hFile,
            reinterpret_cast<PCHAR>(const_cast<LPWAVEFORMATEX>(pwfx)),
            lBytesInWfx
        );
    if (lBytesWritten != lBytesInWfx) {
        printf("mmioWrite(fmt data) wrote %u bytes; expected %u bytes\n", lBytesWritten, lBytesInWfx);
        return E_FAIL;
    }

    // ascend from the 'fmt ' chunk
    result = mmioAscend(hFile, &chunk, 0);
    if (MMSYSERR_NOERROR != result) {
        printf("mmioAscend(\"fmt \" failed: MMRESULT = 0x%08x\n", result);
        return E_FAIL;
    }
    
    // make a 'fact' chunk whose data is (DWORD)0
    chunk.ckid = MAKEFOURCC('f', 'a', 'c', 't');
    result = mmioCreateChunk(hFile, &chunk, 0);
    if (MMSYSERR_NOERROR != result) {
        printf("mmioCreateChunk(\"fmt \") failed: MMRESULT = 0x%08x\n", result);
        return E_FAIL;
    }

    // write (DWORD)0 to it
    // this is cleaned up later
    DWORD frames = 0;
    lBytesWritten = mmioWrite(hFile, reinterpret_cast<PCHAR>(&frames), sizeof(frames));
    if (lBytesWritten != sizeof(frames)) {
        printf("mmioWrite(fact data) wrote %u bytes; expected %u bytes\n", lBytesWritten, (UINT32)sizeof(frames));
        return E_FAIL;
    }

    // ascend from the 'fact' chunk
    result = mmioAscend(hFile, &chunk, 0);
    if (MMSYSERR_NOERROR != result) {
        printf("mmioAscend(\"fact\" failed: MMRESULT = 0x%08x\n", result);
        return E_FAIL;
    }

    // make a 'data' chunk and leave the data pointer there
    pckData->ckid = MAKEFOURCC('d', 'a', 't', 'a');
    result = mmioCreateChunk(hFile, pckData, 0);
    if (MMSYSERR_NOERROR != result) {
        printf("mmioCreateChunk(\"data\") failed: MMRESULT = 0x%08x\n", result);
        return E_FAIL;
    }

    return S_OK;
}

HRESULT FinishWaveFile(HMMIO hFile, MMCKINFO *pckRIFF, MMCKINFO *pckData) {
    MMRESULT result;

    result = mmioAscend(hFile, pckData, 0);
    if (MMSYSERR_NOERROR != result) {
        printf("mmioAscend(\"data\" failed: MMRESULT = 0x%08x\n", result);
        return E_FAIL;
    }

    result = mmioAscend(hFile, pckRIFF, 0);
    if (MMSYSERR_NOERROR != result) {
        printf("mmioAscend(\"RIFF/WAVE\" failed: MMRESULT = 0x%08x\n", result);
        return E_FAIL;
    }

    return S_OK;    
}

#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

HRESULT register_notification_callback(IMMNotificationClient *pClient) {
    HRESULT hr = S_OK;
    IMMDeviceEnumerator *pMMDeviceEnumerator;

    // activate a device enumerator
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, 
        __uuidof(IMMDeviceEnumerator),
        (void**)&pMMDeviceEnumerator
    );
    if (FAILED(hr)) {
        printf("CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x\n", hr);
        return hr;
    }

    // get the default render endpoint
	hr = pMMDeviceEnumerator->RegisterEndpointNotificationCallback(pClient);
    pMMDeviceEnumerator->Release();
    if (FAILED(hr)) {
        printf("IMMDeviceEnumerator::RegisterEndpointNotificationCallback failed: hr = 0x%08x\n", hr);
        return hr;
    }
    return S_OK;
}

HRESULT unregister_notification_callback(IMMNotificationClient *pClient) {
    HRESULT hr = S_OK;
    IMMDeviceEnumerator *pMMDeviceEnumerator;

    // activate a device enumerator
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, 
        __uuidof(IMMDeviceEnumerator),
        (void**)&pMMDeviceEnumerator
    );
    if (FAILED(hr)) {
        printf("CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x\n", hr);
        return hr;
    }

    // get the default render endpoint
	hr = pMMDeviceEnumerator->UnregisterEndpointNotificationCallback(pClient);
    pMMDeviceEnumerator->Release();
    if (FAILED(hr)) {
        printf("IMMDeviceEnumerator::UnregisterEndpointNotificationCallback failed: hr = 0x%08x\n", hr);
        return hr;
    }
    return S_OK;
}


HRESULT get_default_device(IMMDevice **ppMMDevice) {
    HRESULT hr = S_OK;
    IMMDeviceEnumerator *pMMDeviceEnumerator;

    // activate a device enumerator
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, 
        __uuidof(IMMDeviceEnumerator),
        (void**)&pMMDeviceEnumerator
    );
    if (FAILED(hr)) {
        printf("CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x\n", hr);
        return hr;
    }

    // get the default render endpoint
    hr = pMMDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, ppMMDevice);
    pMMDeviceEnumerator->Release();
    if (FAILED(hr)) {
        printf("IMMDeviceEnumerator::GetDefaultAudioEndpoint failed: hr = 0x%08x\n", hr);
        return hr;
    }

    return S_OK;
}

HRESULT list_devices() {
    HRESULT hr = S_OK;

    // get an enumerator
    IMMDeviceEnumerator *pMMDeviceEnumerator;

    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, 
        __uuidof(IMMDeviceEnumerator),
        (void**)&pMMDeviceEnumerator
    );
    if (FAILED(hr)) {
        printf("CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x\n", hr);
        return hr;
    }

    IMMDeviceCollection *pMMDeviceCollection;

    // get all the active render endpoints
    hr = pMMDeviceEnumerator->EnumAudioEndpoints(
        eRender, DEVICE_STATE_ACTIVE, &pMMDeviceCollection
    );
    pMMDeviceEnumerator->Release();
    if (FAILED(hr)) {
        printf("IMMDeviceEnumerator::EnumAudioEndpoints failed: hr = 0x%08x\n", hr);
        return hr;
    }

    UINT count;
    hr = pMMDeviceCollection->GetCount(&count);
    if (FAILED(hr)) {
        pMMDeviceCollection->Release();
        printf("IMMDeviceCollection::GetCount failed: hr = 0x%08x\n", hr);
        return hr;
    }
    printf("Active render endpoints found: %u\n", count);

    for (UINT i = 0; i < count; i++) {
        IMMDevice *pMMDevice;

        // get the "n"th device
        hr = pMMDeviceCollection->Item(i, &pMMDevice);
        if (FAILED(hr)) {
            pMMDeviceCollection->Release();
            printf("IMMDeviceCollection::Item failed: hr = 0x%08x\n", hr);
            return hr;
        }

        // open the property store on that device
        IPropertyStore *pPropertyStore;
        hr = pMMDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
        pMMDevice->Release();
        if (FAILED(hr)) {
            pMMDeviceCollection->Release();
            printf("IMMDevice::OpenPropertyStore failed: hr = 0x%08x\n", hr);
            return hr;
        }

        // get the long name property
        PROPVARIANT pv; PropVariantInit(&pv);
        hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);
        pPropertyStore->Release();
        if (FAILED(hr)) {
            pMMDeviceCollection->Release();
            printf("IPropertyStore::GetValue failed: hr = 0x%08x\n", hr);
            return hr;
        }

        if (VT_LPWSTR != pv.vt) {
            printf("PKEY_Device_FriendlyName variant type is %u - expected VT_LPWSTR", pv.vt);

            PropVariantClear(&pv);
            pMMDeviceCollection->Release();
            return E_UNEXPECTED;
        }

        printf("    %ls\n", pv.pwszVal);
        
        PropVariantClear(&pv);
    }    
    pMMDeviceCollection->Release();
    
    return S_OK;
}

HRESULT get_specific_device(LPCWSTR szLongName, IMMDevice **ppMMDevice) {
    HRESULT hr = S_OK;

    *ppMMDevice = NULL;
    
    // get an enumerator
    IMMDeviceEnumerator *pMMDeviceEnumerator;

    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, 
        __uuidof(IMMDeviceEnumerator),
        (void**)&pMMDeviceEnumerator
    );
    if (FAILED(hr)) {
        printf("CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x\n", hr);
        return hr;
    }

    IMMDeviceCollection *pMMDeviceCollection;

    // get all the active render endpoints
    hr = pMMDeviceEnumerator->EnumAudioEndpoints(
        eRender, DEVICE_STATE_ACTIVE, &pMMDeviceCollection
    );
    pMMDeviceEnumerator->Release();
    if (FAILED(hr)) {
        printf("IMMDeviceEnumerator::EnumAudioEndpoints failed: hr = 0x%08x\n", hr);
        return hr;
    }

    UINT count;
    hr = pMMDeviceCollection->GetCount(&count);
    if (FAILED(hr)) {
        pMMDeviceCollection->Release();
        printf("IMMDeviceCollection::GetCount failed: hr = 0x%08x\n", hr);
        return hr;
    }

    for (UINT i = 0; i < count; i++) {
        IMMDevice *pMMDevice;

        // get the "n"th device
        hr = pMMDeviceCollection->Item(i, &pMMDevice);
        if (FAILED(hr)) {
            pMMDeviceCollection->Release();
            printf("IMMDeviceCollection::Item failed: hr = 0x%08x\n", hr);
            return hr;
        }

        // open the property store on that device
        IPropertyStore *pPropertyStore;
        hr = pMMDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
        if (FAILED(hr)) {
            pMMDevice->Release();
            pMMDeviceCollection->Release();
            printf("IMMDevice::OpenPropertyStore failed: hr = 0x%08x\n", hr);
            return hr;
        }

        // get the long name property
        PROPVARIANT pv; PropVariantInit(&pv);
        hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &pv);
        pPropertyStore->Release();
        if (FAILED(hr)) {
            pMMDevice->Release();
            pMMDeviceCollection->Release();
            printf("IPropertyStore::GetValue failed: hr = 0x%08x\n", hr);
            return hr;
        }

        if (VT_LPWSTR != pv.vt) {
            printf("PKEY_Device_FriendlyName variant type is %u - expected VT_LPWSTR", pv.vt);

            PropVariantClear(&pv);
            pMMDevice->Release();
            pMMDeviceCollection->Release();
            return E_UNEXPECTED;
        }

        // is it a match?
        if (0 == _wcsicmp(pv.pwszVal, szLongName)) {
            // did we already find it?
            if (NULL == *ppMMDevice) {
                *ppMMDevice = pMMDevice;
                pMMDevice->AddRef();
            } else {
                printf("Found (at least) two devices named %ls\n", szLongName);
                PropVariantClear(&pv);
                pMMDevice->Release();
                pMMDeviceCollection->Release();
                return E_UNEXPECTED;
            }
        }
        
        pMMDevice->Release();
        PropVariantClear(&pv);
    }
    pMMDeviceCollection->Release();
    
    if (NULL == *ppMMDevice) {
        printf("Could not find a device named %ls\n", szLongName);
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    return S_OK;
}