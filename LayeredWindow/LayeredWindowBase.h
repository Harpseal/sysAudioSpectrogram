#ifndef LAYERED_WINDOW_H_
#define LAYERED_WINDOW_H_

#include "LayeredWindowInfo.h"
class LayeredWindowAPIBase;
class LayeredWindowD2DBase;
class LayeredWindowGDI;
class LayeredWindowD2DtoDXGI;
class LayeredWindowD2DtoGDI;
class LayeredWindowD2DtoWIC;

class LayeredWindowBase
{
public:
	enum LayeredWindowTechType
	{
		LayeredWindow_TechType_GDI       = 0x01,
		LayeredWindow_TechType_D2D       = 0x10,
		LayeredWindow_TechType_D2DtoDXGI = 0x20 | LayeredWindow_TechType_D2D,
		LayeredWindow_TechType_D2DtoGDI  = 0x40 | LayeredWindow_TechType_D2D,
		LayeredWindow_TechType_D2DtoWIC  = 0x80 | LayeredWindow_TechType_D2D
	};
	LayeredWindowBase(int width,int height,LayeredWindowTechType techType);
	~LayeredWindowBase();


	static LRESULT CALLBACK WndProcStatic( HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam );
	LRESULT WndProc(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual bool Initialize(){return true;}
	virtual void Uninitialize(){}

	virtual void BeforeRender(){}
	virtual void Render();
	virtual void AfterRender(){}
	virtual bool SetSize(int w,int h);

	virtual void OnMouse(UINT msg,int x,int y,UINT vkey,short wheel){}
	virtual void OnKeyboard(int key,bool isKeyDown){}

	bool CheckWindowState();

//protected:
	LayeredWindowTechType m_techType;
	LayeredWindowInfo m_info;
	HWND m_hWnd;
	HANDLE m_hThreadMsg;

	union
	{
		//same api for D2D and GDI
		LayeredWindowAPIBase *pAPI;
		
		//specific only for D2D or GDI
		LayeredWindowD2DBase *pD2D;
		LayeredWindowGDI *pGDI; //Original type

		//Original types
		LayeredWindowD2DtoDXGI *pD2DtoDXGI;
		LayeredWindowD2DtoGDI *pD2DtoGDI;
		LayeredWindowD2DtoWIC *pD2DtoWIC;
	}m_pWin;

	static DWORD WINAPI StartMsgThread(void* pParam);
	void UpdateLayeredWindow();

};

#endif