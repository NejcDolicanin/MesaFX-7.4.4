#ifndef FXUTIL_H
#define FXUTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Reads a value from either environment variables or the registry.
     * Returns a pointer to a static buffer containing the value (or NULL if not found).
     * The buffer is overwritten on each call, so copy it if you need to keep the value.
     */
    char *fxGetRegistryOrEnvironmentString(const char *name);

    /*
     * Reads optimal refresh rate from the registry.
     * Set through the environment variable HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\Display\0000\DEFAULT\RefreshRate
     */
    int ReadRefreshFromRegistry(void);

    /*
     * Detects if the current process is a Sin game executable.
     * Returns 1 if the executable name starts with "sin" (case-insensitive), 0 otherwise.
     * Used to work around the 16-bit texture crash bug in Sin games.
     */
    /* int DetectSinGame(void); */

#ifdef __cplusplus
}
#endif

#endif /* FXUTIL_H */
