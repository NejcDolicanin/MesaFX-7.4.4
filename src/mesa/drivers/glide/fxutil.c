#ifdef _WIN32
/* FX */

#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>
#include "fxutil.h"

/* Forward declaration */
static char *QueryRegistry(const char *regPath, const char *name);

/* Detect if running on Win9x/ME */
int is_win9x()
{
    OSVERSIONINFO verInfo;
    ZeroMemory(&verInfo, sizeof(verInfo));
    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    if (GetVersionEx(&verInfo))
    {
        return (verInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ? 1 : 0;
    }
    return 0; /* Default: assume NT-based (Win2000/XP+) */
}

/* Reads an environment variable first, then checks registry */
char *fxGetRegistryOrEnvironmentString(const char *name)
{
    char *result;
    char regPath[256];
    int i;
    int win9x = is_win9x();

    /* Check environment variable first */
    static char envValue[256];
    if (GetEnvironmentVariable(name, envValue, sizeof(envValue)) > 0)
    {
        return envValue;
    }

    /* First check Device0 paths */
    if (win9x)
    {
        sprintf(regPath, "System\\CurrentControlSet\\Services\\Class\\Display\\0000\\GLIDE");
    }
    else
    {
        sprintf(regPath, "SYSTEM\\CurrentControlSet\\Services\\3dfxvs\\Device0\\GLIDE");
    }
    result = QueryRegistry(regPath, name);
    if (result)
        return result;

    if (win9x)
    {
        sprintf(regPath, "System\\CurrentControlSet\\Services\\Class\\Display\\0000\\DEFAULT");
    }
    else
    {
        sprintf(regPath, "SYSTEM\\CurrentControlSet\\Services\\3dfxvs\\Device0\\DEFAULT");
    }
    result = QueryRegistry(regPath, name);
    if (result)
        return result;

    /* Try alternate devices (both GLIDE and DEFAULT) */
    for (i = 1; i < 10; i++)
    {
        if (win9x)
        {
            sprintf(regPath, "System\\CurrentControlSet\\Services\\Class\\Display\\%04d\\GLIDE", i);
            result = QueryRegistry(regPath, name);
            if (result)
                return result;

            sprintf(regPath, "System\\CurrentControlSet\\Services\\Class\\Display\\%04d\\DEFAULT", i);
            result = QueryRegistry(regPath, name);
            if (result)
                return result;
        }
        else
        {
            sprintf(regPath, "SYSTEM\\CurrentControlSet\\Services\\3dfxvs\\Device%d\\GLIDE", i);
            result = QueryRegistry(regPath, name);
            if (result)
                return result;

            sprintf(regPath, "SYSTEM\\CurrentControlSet\\Services\\3dfxvs\\Device%d\\DEFAULT", i);
            result = QueryRegistry(regPath, name);
            if (result)
                return result;
        }
    }

    return NULL; /* Not found */
}

/* Helper: Queries a specific registry path for a value. Returns string if found, NULL if not. */
static char *QueryRegistry(const char *regPath, const char *name)
{
    static char value[256];
    HKEY hKey;
    DWORD dwType, dwSize;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(value);
        if (RegQueryValueEx(hKey, name, NULL, &dwType, (LPBYTE)value, &dwSize) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return value; /* Return the value string */
        }
        RegCloseKey(hKey);
    }

    return NULL; /* Not found */
}

/* Reads refresh rate from environment or registry. Returns 0 if not found or invalid. */
int ReadRefreshFromRegistry(void)
{
    char *valStr = fxGetRegistryOrEnvironmentString("RefreshRate");
    if (valStr && valStr[0] != '\0')
    {
        int val = atoi(valStr);
        if (val >= 50 && val <= 200)
        { /* Allow any reasonable refresh */
            return val;
        }
    }
    return 0; /* Not found or invalid */
}

/* Detects if the current process is a Sin game executable.
 * Returns 1 if the executable name starts with "sin" (case-insensitive), 0 otherwise.
 */
// int DetectSinGame(void)
// {
//     char exePath[MAX_PATH];
//     char *exeName;
    
//     if (GetModuleFileName(NULL, exePath, MAX_PATH) == 0)
//         return 0;
    
//     /* Get just the filename from the full path */
//     exeName = strrchr(exePath, '\\');
//     if (exeName)
//         exeName++; /* Skip the backslash */
//     else
//         exeName = exePath;
    
//     /* Check if filename starts with "sin" (case-insensitive) */
//     /* This catches sin.exe, sin1600.exe, sinctf.exe, etc. */
//     if (_strnicmp(exeName, "sin", 3) == 0)
//     {
//         /* Make sure it's an .exe file */
//         char *ext = strrchr(exeName, '.');
//         if (ext && _stricmp(ext, ".exe") == 0)
//         {
//             return 1;
//         }
//     }
    
//     return 0;
// }

static int ScanFileForPatterns(const char *filePath,
                               const char *patternA, size_t patternALen,
                               const char *patternB, size_t patternBLen)
{
    HANDLE hFile, hMap;
    BYTE *data;
    DWORD fileSize, i;
    size_t maxLen;
    int foundA = 0;
    int foundB = 0;

    if (!filePath || !filePath[0])
        return 0;

    hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    fileSize = GetFileSize(hFile, NULL);
    maxLen = (patternALen > patternBLen) ? patternALen : patternBLen;
    if (fileSize == INVALID_FILE_SIZE || fileSize < maxLen)
    {
        CloseHandle(hFile);
        return 0;
    }

    hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMap)
    {
        CloseHandle(hFile);
        return 0;
    }

    data = (BYTE *)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (!data)
    {
        CloseHandle(hMap);
        CloseHandle(hFile);
        return 0;
    }

    for (i = 0; i < fileSize; i++)
    {
        if (!foundA && i + patternALen <= fileSize && memcmp(data + i, patternA, patternALen) == 0)
        {
            foundA = 1;
        }
        if (!foundB && i + patternBLen <= fileSize && memcmp(data + i, patternB, patternBLen) == 0)
        {
            foundB = 1;
        }
        if (foundA && foundB)
        {
            break;
        }
    }

    UnmapViewOfFile(data);
    CloseHandle(hMap);
    CloseHandle(hFile);

    return (foundA && foundB) ? 1 : 0;
}

/* Detects if the current process is a Quake 3 (id Tech 3) engine game.
 * Searches for "GetRefAPI" string in the executable or loaded renderer DLLs.
 * Returns 1 if detected, 0 otherwise. Result is cached after first call.
 */
int DetectQuake3Engine(void)
{
    static int cached_result = -1;
    char exePath[MAX_PATH];
    const char *patternA = "GetRefAPI";
    const char *patternB = "RE_BeginRegistration";
    const size_t patternALen = strlen(patternA);
    const size_t patternBLen = strlen(patternB);
    HANDLE snapshot;
    MODULEENTRY32 moduleEntry;

    /* Return cached result if already computed */
    if (cached_result >= 0)
        return cached_result;

    cached_result = 0;

    /* Check the main executable first */
    if (GetModuleFileName(NULL, exePath, MAX_PATH) != 0)
    {
        if (ScanFileForPatterns(exePath, patternA, patternALen, patternB, patternBLen))
        {
            cached_result = 1;
            return cached_result;
        }
    }

    /* Scan loaded modules (renderer DLLs often contain GetRefAPI) */
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE)
        return cached_result;

    moduleEntry.dwSize = sizeof(MODULEENTRY32);
    if (Module32First(snapshot, &moduleEntry))
    {
        do
        {
            if (ScanFileForPatterns(moduleEntry.szExePath, patternA, patternALen, patternB, patternBLen))
            {
                cached_result = 1;
                break;
            }
        } while (Module32Next(snapshot, &moduleEntry));
    }

    CloseHandle(snapshot);
    return cached_result;
}

#endif /* _WIN32 */
