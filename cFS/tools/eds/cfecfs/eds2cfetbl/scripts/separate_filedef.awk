#!/usr/bin/awk -f

BEGIN {
    RS="\n";
    ORS="\n";
    FS=" ";
    IS_FILEDEF=0;   # set nonzero at the start of the filedef
}

IS_FILEDEF==0 {
    if (/CFE_TBL_FileDef_t/) {
        IS_FILEDEF=1
    }
}

IS_FILEDEF==WANT_FILEDEF {
    print
}

IS_FILEDEF==1 {
    if (/;/) {
        IS_FILEDEF=0
    }
}
