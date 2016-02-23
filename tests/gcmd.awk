#!/usr/bin/gawk -f

@load "gcmd";
BEGINFILE {
    GCMD = ""

    # using just cat should effectively produce the same results as not using GCMD at all
    if (FILENAME == "file1") {
        GCMD = "cat"
    }

    # return sorted output
    if (FILENAME == "file2") {
        GCMD = "cat {} | sort"
    }

    # gcmd_can_take_file should return false for file3
    # "file3" -> GCMD = ""

    # only read odd lines
    if (FILENAME == "file4") {
        GCMD = "awk 'NR % 2 {print;}'"
    }

    # this command should fail due to not existing
    if (FILENAME == "file5") {
        GCMD = "/usr/bin/qpwoeiruty"
    }

    print FILENAME
    print "---"
}


{ print; }

ENDFILE {
    print "---"
    print ""
}