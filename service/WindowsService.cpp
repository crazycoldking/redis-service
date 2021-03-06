// WindowsService.cpp : Entry point for the service
//

#include "stdafx.h"
#include "ServiceInstaller.h"
#include "ServiceBase.h"
#include "WindowsServiceImpl.h"
#include <process.h>

bool isrunningasservice(unsigned int pid);

// Internal name of the service
#define SERVICE_NAME             L"Redis"

// Displayed name of the service
#define SERVICE_DISPLAY_NAME     L"Redis Service"

// Displayed service description
#define SERVICE_DISPLAY_DESCRIPTION	L"Hosts Redis server in a windows service"

// Service start options.
#define SERVICE_START_TYPE       SERVICE_AUTO_START

// List of service dependencies - "dep1\0dep2\0\0"
#define SERVICE_DEPENDENCIES     L""

// The name of the account under which the service should run
#define SERVICE_ACCOUNT          L"NT AUTHORITY\\LocalService"

// The password to the service account name
#define SERVICE_PASSWORD         NULL

CWindowsServiceImpl* service = NULL;

BOOL ctrlHandler(DWORD fdwCtrlType) 
{ 
	BOOL result = S_OK;

	switch (fdwCtrlType) 
	{ 
		// Handle the CTRL-C signal. 
		case CTRL_C_EVENT: 
			printf("Ctrl-C pressed, exiting...\n\n");
			break;
		// CTRL-CLOSE: confirm that the user wants to exit. 
		case CTRL_CLOSE_EVENT: 
			printf("Window is closing...\n\n");
			break;
		// Pass other signals to the next handler. 
		case CTRL_BREAK_EVENT: 
			printf("Ctrl-Break pressed, exiting...\n\n");
			break;
		case CTRL_LOGOFF_EVENT: 
			printf("Logoff detected, exiting...\n\n");
			break;
		case CTRL_SHUTDOWN_EVENT: 
			printf("System shutdown, exiting...\n\n");
			break;
		default:
			result = S_FALSE;
			break;
	} 

	if (result == S_OK)
		shutdown();

	printf("Exit done");

	return result;
} 

int wmain(int argc, wchar_t *argv[])
{
	// Executed as console application
    if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
    {
        if (_wcsicmp(L"install", argv[1] + 1) == 0)
        {
            // Install the service when the command is 
            // "-install" or "/install".
			
            InstallService(
                SERVICE_NAME,                // Name of service
                SERVICE_DISPLAY_NAME,        // Name to display
				SERVICE_DISPLAY_DESCRIPTION, // Description to display
                SERVICE_START_TYPE,          // Service start type
                SERVICE_DEPENDENCIES,        // Dependencies
                SERVICE_ACCOUNT,             // Service running account
                SERVICE_PASSWORD             // Password of the account
                );
        }
        else if (_wcsicmp(L"remove", argv[1] + 1) == 0)
        {
            // Uninstall the service when the command is 
            // "-remove" or "/remove".
            UninstallService(SERVICE_NAME);
        }
		else if (_wcsicmp(L"commandline", argv[1] + 1) == 0)
		{
			// Set keyboard event handler
			SetConsoleCtrlHandler((PHANDLER_ROUTINE)ctrlHandler, TRUE);

			// Execute functionality as command line console tool
			service = new CWindowsServiceImpl(SERVICE_NAME);

			if (service)
				service->ServiceWorkerThread();
		}
    }
    else if (isrunningasservice(_getpid()))
	{
		// Start service instance when executed as a service from SCM
		service = new CWindowsServiceImpl(SERVICE_NAME);
		CServiceBase::Run(*service);
	}
	else
    {
        wprintf(L"Parameters:\n");
        wprintf(L" /install      to install the service.\n");
        wprintf(L" /remove       to remove the service.\n");
		wprintf(L" /commandline  to execute as CLI tool.\n");
    }

    return 0;
}

void shutdown()
{
	if (service)
	{
		service->OnStop();

		delete service;
		service = NULL;
	}
}

bool isrunningasservice(unsigned int pid)
{
    bool result = false;

	try
	{
		SC_HANDLE hScm = OpenSCManager(
			0,
			SERVICES_ACTIVE_DATABASE,
			SC_MANAGER_ENUMERATE_SERVICE
		);

		if (hScm == 0)
			return result;

		DWORD ServicesBufferRequired = 0;
		DWORD ResumeHandle = 0;

		DWORD ServicesBufferSize = 0;
		DWORD ServicesCount = 0;
		ENUM_SERVICE_STATUS_PROCESS* ServicesBuffer = 0;

		EnumServicesStatusEx(hScm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, 
		SERVICE_ACTIVE, 0, 0, &ServicesBufferRequired, &ServicesCount, &ResumeHandle, 0);

		ServicesBuffer = (ENUM_SERVICE_STATUS_PROCESS*)new char[ServicesBufferRequired];
		ServicesBufferSize = ServicesBufferRequired;
		EnumServicesStatusEx(hScm,
							SC_ENUM_PROCESS_INFO,
							SERVICE_WIN32, 
							SERVICE_ACTIVE,
							(LPBYTE)ServicesBuffer,
							ServicesBufferSize, 
							&ServicesBufferRequired,
							&ServicesCount,
							&ResumeHandle,
							0);

		ENUM_SERVICE_STATUS_PROCESS* ServicesBufferPtr = ServicesBuffer;
		while (ServicesCount--)
		{
			if (ServicesBufferPtr->ServiceStatusProcess.dwProcessId == pid)
			{
				result = true;
				break;
			}

			ServicesBufferPtr++;
		}

		delete [] ServicesBuffer;

		CloseServiceHandle(hScm);
	}
	catch(...)
	{
	}

    return result;
}