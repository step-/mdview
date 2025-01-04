#  --no-ext        DEPRECATED, like --no-auto-code, --no-permlink, --no-shebang
#                  and --no-table all combined
set -x
alias cmp_cmark_mdview='_f() { vimdiff <(cmark --nobreaks "$1") <(mdview --html --cm-block-end --no-smart --no-ext "$1"); }; _f'
alias cmp_cmark_mdview_soft_break='_f() { vimdiff <(cmark "$1") <(mdview --soft-break --html --no-gettext --cm-block-end --no-smart --no-ext "$1"); }; _f'
alias cmp_cmark_mdview_unsafe_html='_f() { vimdiff <(cmark --nobreaks --unsafe "$1") <(mdview --html --cm-block-end --no-smart --no-ext --unsafe-html "$1"); }; _f'
alias cmp_cmark_gfm='_f() { vimdiff <(cmark "$1" |grep -Fv "raw HTML omitted") <(cmark-gfm "$1" |grep -Fv "raw HTML omitted"); }; _f'
alias cmp_cmark_md2html='_f() { vimdiff <(cmark --unsafe "$1") <(md2html --xhtml "$1"); }; _f'
alias cmp_cmark_pandoc='_f() { vimdiff <(cmark --unsafe "$1") <(pandoc -f markdown-auto_identifiers -t html "$1"); }; _f'
alias cmp_gfm_mdview='_f() { vimdiff <(cmark-gfm --nobreaks --unsafe -e strikethrough -e table "$1") <(mdview --html --cm-block-end --no-auto-code --no-permlink --no-shebang --unsafe-html "$1"); }; _f'
alias cmp_gfm_mdview_soft_break='_f() { vimdiff <(cmark-gfm --unsafe -e strikethrough "$1") <(mdview --soft-break --html --no-gettext --cm-block-end --no-smart --no-ext --unsafe-html "$1"); }; _f'
alias cmp_gfm_md2html='_f() { vimdiff <(cmark-gfm --unsafe -e strikethrough -e table "$1") <(md2html --fstrikethrough --ftables --xhtml "$1"); }; _f'
alias cmp_pandoc_mdview='_f() { vimdiff <(pandoc -f markdown-auto_identifiers -t html "$1") <(mdview --html --cm-block-end --no-ext --unsafe-html "$1"); }; _f'
alias cmp_md2html_mdview='_f() { vimdiff <(md2html --ftables --xhtml "$1") <(mdview --html --cm-block-end --no-ext --unsafe-html "$1"); }; _f'

: "eg. map_failing_script echo # all on one line"
: "	map_failing_script 'vim -o'"
alias map_failing_script='_f() { make test-unattended 2> /dev/null | ag "FAIL 1 " | map -ds "$* test/script/%5.sh"; }; _f'

: "eg. map1_failing_script echo # each on its own line"
: "	map -1 'printf \"Enter=[ %B ] C-C=quit : \"; read x && ./% --vim' \$(map1_failing_script echo | uniq)"
alias map1_failing_script='_f() { make test-unattended 2> /dev/null | ag "FAIL 1 " | map -1 -ds "$* test/script/%5.sh"; }; _f'
set +x
