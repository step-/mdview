help2man = find_program('help2man', required: false)

if help2man.found()
	meson.add_install_script('meson_install_script.sh')
else
	warning('help2man command not found: man page mdview.1 will not be installed')
endif
