#!/usr/bin/awk -f

BEGIN {
    RS="=";
    ORS="\n";
    FS=" ";
    TYPENAME="";
}

END {
    if (TYPENAME == "") {
        print "\nERROR: Unable to extract type name for table\n" > "/dev/stderr"
        exit 1
    }
}


$NF ~ OBJNAME {
    for (i=1;i<NF;i++) if ($i != "const" && $i != "static") TYPENAME=$i
    print TYPENAME
    exit 0
}
