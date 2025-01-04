*em*  
**strong**  
***em strong***  
`code span`  
```
codeblock
```
~~strikethrough~~  
# Heading 1

## Heading 2

### Heading 3

#### Heading 4

##### Heading 5

###### Heading 6

* bullet

----

paragraph

BR  

[link text](https://x.com)

[link text](https://x.com "title")

![image of linux3](linux3.png)

![image of linux3](linux3.png "title")

----

```
-----------------------------------------------------------------------
-                               MDVIEW MTX                            -
-----------------------------------------------------------------------
```

Whether the path of an image, e.g. `![image path](tom&jerry.png)` (this path
does not exist on purpose), is _uri-escaped_ depends on the output mode:

--html escapes, --ansi, --text and --tty (the text modes) do not, and the GUI
viewer does when it loads the image from the path but does not on cursor hover:
![image path](tom&jerry).

The _uri-escaping_ rules for a link like `[link](https://tom&jerry.com)` (OK,
the URL isn't quite valid), also depend on the output mode: --html escapes, text
modes do not, the GUI viewer does on clicking the link but does not on cursor
hover: [link](https://tom&jerry.com).

---

MDVIEW4 supports nested and ordered lists (tight and loose):

1. e  

   1. f

1. g

divider

1. e  
   1. f
1. g
