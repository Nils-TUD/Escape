BEGIN {
}

END {
    print ""
    print "CODE\tstart\t0x0"
}

/^[[:xdigit:]]+ ([[:xdigit:]]+ )?(t|T) .*$/ {
	if($2 ~ /(t|T)/)
		printf "%s\tCODE 0x%s\n", $3, $1
	else
		printf "%s\tCODE 0x%s\n", $4, $1
}
