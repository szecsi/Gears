// stdafx.cpp : source file that includes just the standard includes
// Gears.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef __linux__
void Sleep(uint msec)
	{
		struct timespec req, rem;
    	int err;
    	req.tv_sec = msec / 1000;
    	req.tv_nsec = (msec % 1000) * 1000000;
    	while ((req.tv_sec != 0) || (req.tv_nsec != 0)) {
    	    if (nanosleep(&req, &rem) == 0)
    	        break;
    	    err = errno;
    	    // Interrupted; continue
    	    if (err == EINTR) {
    	        req.tv_sec = rem.tv_sec;
    	        req.tv_nsec = rem.tv_nsec;
    	    }
    	    // Unhandleable error (EFAULT (bad pointer), EINVAL (bad timeval in tv_nsec), or ENOSYS (function not supported))
    	    break;
    	}
	}
#endif

// reference any additional headers you need in STDAFX.H
// and not in this file
