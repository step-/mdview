#!/usr/bin/env -S awk -f

### extra styles beyond those constructed from the input files
BEGIN {
	# start RHS with a space
	extra["code"] = " white-space: pre-wrap;"
}
BEGIN {
	phase = "color" # next phase is "style" (last phase)
}
### all phases
NF < 3 || !/#define/ || $1 ~ /\/\*/ {
	next
}
{
	$1 = ""
}
### color phase: load colors from ARGV[1]
phase == "color" {
	if ($0 ~ /COLOR/) {
		name = $2
		color = $3
		gsub(/"/, "", color)
		if (color ~ /^MTX/) {
			color = colors[color]
		}
		colors[name] = color
		next
	}
	else {
		phase = "style"
	}
}
### style phase: make CSS rules from ARGV[2]
{
	name = $2;
	rhs = ""
	for (i = 3; i <= NF; i++) {
		rhs = rhs " " $i ";"
	}
	gsub(/\\"/,"", rhs)
	gsub(/"/, "", rhs)
	gsub(/=/, ":", rhs)
	gsub(/fgcolor/, "color", rhs)
	gsub(/bgcolor/, "background", rhs)
	gsub(/size/, "font-size", rhs)
	gsub(/variant/, "font-variant", rhs)
	gsub(/smallcaps/, "small-caps", rhs)
	gsub(/single/,"", rhs)
	gsub(/double/," &", rhs)
	gsub(/underline:/, "text-decoration: underline", rhs)
	gsub(/weight/, "font-weight", rhs)
	for (k in colors) {
		gsub(k, colors[k], rhs)
	}
	styles[name] = rhs

	lhs = name
	class_only = 0
	sub(/.*PANGO_/, "", lhs)
	if (lhs == "BLOCKQUOTE") {
		styles[name] = "\n\tmargin: 0;\n\tpadding: 0 1em;\n\tborder-left: 0.25em solid silver;\n"
	} else if (lhs == "CODE_SPAN") {
		lhs = "code"
	} else if (lhs == "CODEBLOCK") {
		lhs = "pre"
	} else if (lhs == "IMAGE") {
		lhs = "img"
	} else if (lhs == "URL") {
		lhs = "a"
	} else if (lhs == "URL_HEADING") {
		lhs = "a_heading"
		class_only = 1
	} else if (lhs == "TABLE") {
		sub (/background[^;]+;/, "", styles[name])
	} else if (lhs == "TH") {
		styles[name] = styles[name]" padding: 0.25em 0.5em;"
	} else if (lhs == "TD") {
		sub (/background[^;]+;/, "", styles[name])
		styles[name] = styles[name]" padding: 0.25em 0.5em;"
	}
	k = tolower(lhs)
	if (!class_only) {
		printf ".mtx-body %s {%s }\n", k, (styles[name] extra[k]) > OUTF
	}
	else {
		printf ".mtx-body %s {%s }\n", k, (styles[name] extra[k]) > OUTF
	}
	delete extra[k]
}
END {
	for (k in extra) {
		printf "%s, .mtx_%s {%s }\n", k, k, extra[k] > OUTF
	}
	print ".mtx-toc code {" > OUTF
	print "\tbackground-color: transparent;" > OUTF
	print "}" > OUTF

	exit (close(OUTF))
}
