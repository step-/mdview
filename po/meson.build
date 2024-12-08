i18n = import('i18n')
add_project_arguments('-DGETTEXT_PACKAGE="' + cfg.get('GETTEXT_PACKAGE') + '"',
	language:'c')
i18n.gettext(cfg.get('GETTEXT_PACKAGE'),
	args: [
		'--keyword=_',
		'--keyword=Q_:1g',
		'--package-version=' + cfg.get('PACKAGE_VERSION'),
		'--copyright-holder=step, ' + cfg.get('PACKAGE_URL'),
		'--msgid-bugs-address=' + cfg.get('PACKAGE_URL') / 'issues'
	]
)
