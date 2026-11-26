#ifndef _FNX_LOADLIB_H
    #define _FNX_LOADLIB_H

    /* WIN32 INCLUDES */
    #ifndef LINUX
        #include <windows.h>
        #include <winbase.h>
    #else
        #include <dlfcn.h>
        #include <unistd.h>
        #define __stdcall
    #endif

    typedef void (__stdcall  * dlfunc) (void *(*)(char *), void (*)(char *, char *, int, void *));

    #ifdef WIN32
        #define dlopen(a,b)     LoadLibrary(a)
        #define dlsym(a,b)      (dlfunc)GetProcAddress(a,b)

        static char * dlerror (void)
        {
            static char * buffer = "Could not load library." ;
            return buffer;
        }
    #endif

#endif
