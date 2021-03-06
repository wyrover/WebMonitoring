#include "stdafx.h"
#include "ServiceInstaller.h"
#include "utils.h"
#include <stdio.h> 


bool InstallService(PWSTR pszServiceName,
    PWSTR pszDisplayName,
    DWORD dwStartType,
    PWSTR pszDependencies,
    PWSTR pszAccount,
    PWSTR pszPassword)
{
    wchar_t szPath[MAX_PATH];
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    bool success = false;
    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0)
    {
        wprintf(L"GetModuleFileName failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    // Open the local default service control manager database 
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT |
        SC_MANAGER_CREATE_SERVICE);
    if (schSCManager == NULL)
    {
        wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    // Install the service into SCM by calling CreateService 
    schService = CreateService(
        schSCManager,                   // SCManager database 
        pszServiceName,                 // Name of service 
        pszDisplayName,                 // Name to display 
        SERVICE_QUERY_STATUS,           // Desired access 
        SERVICE_WIN32_OWN_PROCESS,      // Service type 
        dwStartType,                    // Service start type 
        SERVICE_ERROR_NORMAL,           // Error control type 
        szPath,                         // Service's binary 
        NULL,                           // No load ordering group 
        NULL,                           // No tag identifier 
        pszDependencies,                // Dependencies 
        pszAccount,                     // Service running account 
        pszPassword                     // Password of the account 
    );
    if (schService == NULL)
    {
        //wprintf(L"CreateService failed w/err 0x%08lx\n", GetLastError());
        printError(L"CreateService failed");
        goto Cleanup;
    }

    wprintf(L"%s is installed.\n", pszServiceName);
    success = true;
Cleanup:
    // Centralized cleanup for all allocated resources. 
    if (schSCManager)
    {
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    if (schService)
    {
        CloseServiceHandle(schService);
        schService = NULL;
    }
    return success;
}


// 
//   FUNCTION: UninstallService 
// 
//   PURPOSE: Stop and remove the service from the local service control  
//   manager database. 
// 
//   PARAMETERS:  
//   * pszServiceName - the name of the service to be removed. 
// 
//   NOTE: If the function fails to uninstall the service, it prints the  
//   error in the standard output stream for users to diagnose the problem. 
// 
void UninstallService(PWSTR pszServiceName)
{
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS ssSvcStatus = {};

    // Open the local default service control manager database 
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {
        wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    // Open the service with delete, stop, and query status permissions 
    schService = OpenService(schSCManager, pszServiceName, SERVICE_STOP |
        SERVICE_QUERY_STATUS | DELETE);
    if (schService == NULL)
    {
        wprintf(L"OpenService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    // Try to stop the service 
    if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
    {
        wprintf(L"Stopping %s.", pszServiceName);
        Sleep(1000);

        while (QueryServiceStatus(schService, &ssSvcStatus))
        {
            if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
            {
                wprintf(L".");
                Sleep(1000);
            }
            else break;
        }

        if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
        {
            wprintf(L"\n%s is stopped.\n", pszServiceName);
        }
        else
        {
            wprintf(L"\n%s failed to stop.\n", pszServiceName);
        }
    }

    // Now remove the service by calling DeleteService. 
    if (!DeleteService(schService))
    {
        wprintf(L"DeleteService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    wprintf(L"%s is removed.\n", pszServiceName);

Cleanup:
    // Centralized cleanup for all allocated resources. 
    if (schSCManager)
    {
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    if (schService)
    {
        CloseServiceHandle(schService);
        schService = NULL;
    }
}