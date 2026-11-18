#!/usr/bin/awk -f

BEGIN {
    RS="\n";
    ORS="\n";
    FS=" ";
    IS_FILEDEF=0;   # set nonzero at the start of the filedef
}

# The C preprocessor outputs lines starting with # that indicate which file it came from
# We are only interested in content that was _directly_ in the table file here.
/^#/ {
    LINE=$2;
    FILE=$3;
    next;
}

# Pass through anything that is in the original source file
FILE=="\"" SRC "\"" {
    print
}
