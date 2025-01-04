# Auto-heading mitigations

To mitigate the effects of [issues 276] [and 277], auto-headings are not
created for headings containing an odd number of asterisk, underscore, or tilde
characters. As a result, these headings cannot be navigated using hotkeys.

[issues 276]: <http://github.com/mity/md4c/issues/276>
[and 277]: <http://github.com/mity/md4c/issues/277>
# \~
# *
# \_

-------------------------

# Headings in code blocks

As expected, heading syntax within code blocks is treated as regular text and
does not trigger auto-heading creation.

```
# all by itself
```

```
# 
# ▶◀
# A
# A B
# Setext 1
==========
```

```
all by itself
-------------
```

-------------------------

# Table of Contents

<!--[toc]-->
