// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

bool floatIsEqual(float n1, float n2);
bool mtxIsEqual( float* mtx1, float* mtx2, unsigned w, unsigned h );
bool mtxIsEqualPart(float* mtx1, float* mtx2, unsigned w, unsigned h, unsigned miss);

// TODO: reference additional headers your program requires here
