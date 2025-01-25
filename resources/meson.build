awk = find_program('awk')
cpp = find_program('cpp')
sh  = find_program('sh')

# https://mesonbuild.com/Reference-manual_functions.html#custom_target
usage_md = custom_target('usage_md',
	input  : [ 'make_md.sh', 'usage.md.in'],
	output : 'usage.md',
	command : [ sh, '-xc', '"$1" "$2" "$3" "$4"', 'arg0',
		'@INPUT@', '@OUTPUT@', meson.build_root() ],
)

mtx1_css = custom_target('mtx1_css',
	input : ['make_css1.awk', '../src/mtxcolor.h', '../src/mtxstylepango.h'],
	output : 'mtx1.css',
	command : [ sh, '-xc', 'awk -f "$1" "OUTF=$4" "$2" "$3"', 'arg0',
		'@INPUT@', '@OUTPUT@'],
)

mtx2_css = custom_target('mtx2_css',
	input : ['make_css2.sh', 'github-markdown.css'],
	output : 'mtx2.css',
	command : [ sh, '-xc', '"$1" "$2" "$3"', 'arg0',
		'@INPUT@', '@OUTPUT@'],
)

# https://mesonbuild.com/Gnome-module.html#gnomecompile_resources
gnome = import('gnome')
all_resources = gnome.compile_resources('mtxresdata',
	'all.gresource.xml',
	dependencies : [usage_md, mtx1_css, mtx2_css],
	c_name : 'all'
)
