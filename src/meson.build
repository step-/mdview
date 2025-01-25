src = [
	'main.c',
	'mtxviewer.c',
	'mtxtextview.c',
	'entity.c',
	'md4c.c',
	'mtxrender.c',
	'mtx.c',
	'mtxcmm.c',
	'mtxdbg.c',
	'mtxmarkup.c',
]

dep = []
if get_option('gtk') == '2'
  dep += dependency('gtk+-2.0', version : '>= 2.24.33')
else
  dep += dependency('gtk+-3.0', version : '>= 3.24.33')
endif
dep += dependency('glib-2.0', version : '>= 2.72.4')
dep += dependency('pango', version : '>= 1.50.6')

configure_file(input: 'mtxversion.h.in',
	output : 'mtxversion.h',
	configuration : cfg)

mdview = executable('mdview',
	src + all_resources,
	override_options : ['optimization=3', 'debug=false', 'strip=true'],
	c_args : '-DGTK_DISABLE_DEPRECATED' +
		' -UCOMMONMARK_FENCED_CODEBLOCK_LINE_ENDING' +
		' -DG_DISABLE_ASSERT -DG_SEAL_ENABLE' +
		' -Wextra -Wshadow',
	dependencies : dep,
	install : true,
)
