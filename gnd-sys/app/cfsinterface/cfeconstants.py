"""
    Copyright 2022 bitValence, Inc.
    All Rights Reserved.

    This program is free software; you can modify and/or redistribute it
    under the terms of the GNU Affero General Public License
    as published by the Free Software Foundation; version 3 with
    attribution addendums as found in the LICENSE.txt.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    Purpose:
        Define cFE constants
        todo: These definitions should come from EDS

"""

###############################################################################

class Cfe():

    CFE_TIME_FLY_ON_EID     = 20
    CFE_TIME_FLY_OFF_EID    = 21
    CFE_EVS_NO_FILTER       = 0x0000
    CFE_EVS_FIRST_ONE_STOP  = 0xFFFF
                
    EVS_DEBUG_MASK    = 0b0001
    EVS_INFO_MASK     = 0b0010
    EVS_ERROR_MASK    = 0b0100
    EVS_CRITICAL_MASK = 0b1000
                
    FILE_XFER_DATA_SEG_LEN = 512
