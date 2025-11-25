#ifdef _WIN32
/* FX */

#include <windows.h>
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
 * This is used to work around the 16-bit texture crash bug in Sin games. 
 * 
 * Because GL_SHARED_TEXTURE_PALETTE_EXT isnt handled well in Mesa.
 * Sin:
- Uses paletted textures (CI8 format)
- Enables GL_SHARED_TEXTURE_PALETTE_EXT
- Uses a single global shared palette_ for ALL textures
- combines with 16bit texture formats (like RGB565, ARGB4444)
The issue is texstore.c, how rgb565 and argb4444 is handled Nejc ToDo
 * */
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

#endif
