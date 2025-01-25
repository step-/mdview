#
# This meson build is designed to build a Release target.
# It won't build a Debug target properly.
#

project('mdview', 'c',
	version : '2025.01.04',
	license : 'GPL-2.0-or-later',
)

cfg = configuration_data()
cfg.set('PACKAGE_NAME', meson.project_name())
cfg.set('PACKAGE_VERSION', meson.project_version())
cfg.set('PACKAGE_URL', 'http://github.com/step-/mdview')
cfg.set('PACKAGE_DESC', 'GTK markdown viewer and CLI converter')
cfg.set('GETTEXT_PACKAGE', meson.project_name())
cfg.set('LOCALEDIR', get_option('prefix') / get_option('localedir'))
cfg.set('GIT_VERSION', '')

subdir('po')
subdir('resources')
subdir('src')
subdir('doc')
