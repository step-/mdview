<!-- Test subject extracted from function mtx_cmm_regex_astx -->

SETEXT 0
=======

# ATX 1

atx 1

   ##      ATX 2

atx 2

SETEXT 1 !
==========

setext 1

SETEXT 2
+SETEXT 2
--------

setext 2

------------------------------------------------------------------------------

> quote
====================

* UL list item
====================

- UL list item
====================

1. OL list item
====================

<!-- This comment breaks out of the OL list above.
As with cmark, mdview --html and --pango do NOT need this
comment to break out. But mdview --text needs it otherwise
it will render the SETEXT heading below as regular text.
This is likely a bug in --text rendering.
-->
   SETEXT Z
+SETEXT Z   
============

#  
# Fatdog64-903 FAQ title format
#  
