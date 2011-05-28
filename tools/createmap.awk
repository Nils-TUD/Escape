# script to transform a ld script 

BEGIN {
    secRepl[".text"] = "CODE"
    #secRepl[".data"] = "DATA"
    #secRepl[".bss"] = "BSS"
}

END {
    print ""
    for(sec in sections) {
        if(sec in secUsed) {
            print sections[sec]
        }
    }
}

/^\.[^\t ]+[\t ]+[0x[:xdigit:]]+[\t ]+[0x[:xdigit:]]+([\t ]+load address [0x[:xdigit:]]+)?[\t ]*$/ {
    # section found
    secName = $1
    if(secName ~ /\.text|\.bss|\.data/) {
        secStart = strtonum($2)
        secLen = strtonum($3)
        sections[secName] = sprintf("%s\tstart 0x%.16X\tsize 0x%.16X",    \
                                    secRepl[secName], secStart, secLen)
    }
}

/^[\t ]+[0x[:xdigit:]]+[\t ]+[^\t ]+[\t ]*$/ {
    # symbol found
    if(secName in sections) {
        secUsed[secName] = 1
        printf "%s\t%s\t0x%.8X\n", $2, secRepl[secName], strtonum($1) - secStart
    }
}
