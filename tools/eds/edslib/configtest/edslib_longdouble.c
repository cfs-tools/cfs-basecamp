/*
 * LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
 * 
 * Copyright (c) 2020 United States Government as represented by
 * the Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/**
 * \file     edslib_longdouble.c
 * \ingroup  edslib
 * \author   joseph.p.hickey@nasa.gov
 *
**  This is a configuration test file used by the build system to determine
**  the level of floating point type support of the compiler and C library.
**
**  For the Pack/Unpack routines in EdsLib it is beneficial to use the maximum
**  precision that is possible using the native FPU.  C99 defined the "long double"
**  type and other systems may even have some form of quad type.
**
**  On the other end of the spectrum, embedded microcontrollers may not even
**  support any sort of extended-precision type at all.
**
**  At the moment this checks if we can compile a simple function that uses
**  the long double type.  If this succeeds then it is assumed that the system
**  has a functional "long double" implementation.  Otherwise a standard "double"
**  can be used instead.
 */

#include <stdlib.h>
#include <math.h>
#include <float.h>

int main(void)
{
    long double x1,x2;
    long double y;
    int e;

    x1 = 1234.0L;

    /*
     * It is important to actually call some C99 long-double math functions,
     * as it is possible the compiler knows the type but the C library does not.
     * (RTEMS is an example)
     *
     * EdsLib uses some others beyond these two, but it is assumed if these
     * two will build and link than the others will too.
     */
    y = frexpl(x1, &e);
    x2 = ldexpl(y, e);

    if (x1 != x2)
    {
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
