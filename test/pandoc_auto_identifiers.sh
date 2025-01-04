#!/bin/sh

# Test pandoc auto_identifiers extensions.

for e in \
	auto_identifiers \
	auto_identifiers+ascii_identifiers \
	auto_identifiers+gfm_auto_identifiers \
	; do
	echo
	echo "::::: $e :::::"
	pandoc --wrap=none -f markdown-smart+$e -t commonmark --toc -s << \EOF | grep ^-
# 1 Maître d'hôtel. !


# 2 中文标题. !


# 3 é è ê ë ç á à â ä í ì î ï ó ò ô ö ú ù û ü. !


# 4 ALLCAPS. !


# 5 'I ♥ Dogs'.; !
'i-love-dogs'

# 6 '  Déjà Vu!  '.; !
'deja-vu'

# 7 'fooBar 123 $#%'.; !
'foo-bar-123'

# 8 'я люблю единорогов'.; !
'ya-lyublyu-edinorogov'

# 9 'a-b_c D--E__F'... !
EOF
done
