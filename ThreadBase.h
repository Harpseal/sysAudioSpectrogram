#ifndef THREAD_BASE_H_
#define THREAD_BASE_H_
#include <Windows.h>

class ThreadBase
{
public:
	ThreadBase(){m_hThread = NULL;}
	~ThreadBase(){Termiate();}

	virtual void Start() = 0;
	virtual void Termiate()
	{
		if (m_hThread)
		{
			::TerminateThread( m_hThread, 0 );
			//::CloseHandle( m_hThread );
			m_hThread = NULL;
		}
	}

	HANDLE m_hThread;
};

#endif