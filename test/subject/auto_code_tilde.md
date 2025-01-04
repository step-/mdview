my ~/.vimrc [link](http://examples.com/~me/.vimrc)

```
/mnt/s/git/ste/mdview4 213♯ md2html --fstrikethrough << EOF
 a ~ b[](#a%20~%20b)
EOF
<p>a <del> b<a href="#a%20~%20b"></a></p>
/mnt/s/git/ste/mdview4 213♯ md2html --fstrikethrough << EOF
my ~/.vimrc [link](http://example.com/~me/.vimrc)
EOF
<p>my ~/.vimrc <a href="http://example.com/~me/.vimrc">link</a></p>
/mnt/s/git/ste/mdview4 213♯ md2html --fstrikethrough << EOF
my ~/.vimrc [link](http://example.com/me/~.vimrc)
EOF
<p>my <del>/.vimrc <a href="http://example.com/me/~.vimrc">link</a></p>
```


# a ~ b
# c _ d

<!--
with auto heading links enabled, the markdown above becomes

[a ~ b]: <#a%20~%20b>
[c _ d]: <#c%20_%20d>
# a ~ b&#x200B;[​](<#a%20~%20b>)&#x200B;[​](<#a-b>)&#x200B;[​](<#a_b>)
# c _ d&#x200B;[​](<#c%20_%20d>)&#x200B;[​](<#c-_-d>)&#x200B;[​](<#c___d>)

