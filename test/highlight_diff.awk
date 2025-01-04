#!/usr/bin/gawk -f

###gawk

# assume unified diff format
/^\+/ { printf "\033[32m%s\033[0m\n", $0; next }
/^-/  { printf "\033[31m%s\033[0m\n", $0; next }

# assume word diff format (requires gawk)
{
	n = split($0, a, /{\+|\+}|\[-|-\]/, seps)
	for (i = 0; i < n; i++) {
		switch (seps[i]) {
			case "{+" : printf "\033[32m%s", seps[i]; break
			case "+}" : printf "%s\033[0m",  seps[i]; break
			case "[-" : printf "\033[31m%s", seps[i]; break
			case "-]" : printf "%s\033[0m",  seps[i]; break
		}
		printf "%s", a[i+1]
	}
	printf "\n"
}

### count results
# \033 comes from script/lib/common.sh main()
/\033.*PASS /    { ++pass    }
/\033.*FAIL /    { fail+=$3  }
/\033.*INVALID / { ++invalid }
/\033.*CREATED / { ++created }

END {
	if (pass || fail || invalid || created)
		printf "===== PASS %d, FAIL %d%s%s =====\n", pass, fail, invalid+0 ?", INVALID "invalid :"", created+0 ?", CREATED "created :""
	exit fail+invalid>0
}

###gawk
