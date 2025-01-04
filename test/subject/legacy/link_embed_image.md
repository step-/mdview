### Link and image can embed each other

For an image inside link text, the image becomes the link:
```markdown
[![image of the sun](sun.png)](https://sun.stars.org)
```
[![image of the sun](sun.png)](https://sun.stars.org),
and if the image file is missing:
[![image of the sun](sun.png-MISSING)](https://sun.stars.org).  
**html:** `<a href="https://sun.stars.org"><img src="sun.png" alt="image of the sun" /></a>`

---
For a link inside image alt text, the image is retained and the link is flattened:
```markdown
![[link to the sun](https://sun.stars.org)](sun.png)
```
![[link to the sun](https://sun.stars.org)](sun.png),
and if the image file is missing:
![[link to the sun](https://sun.stars.org)](sun.png-MISSING).  
**html:** `<img src="sun.png" alt="link to the sun" />`

---
For a link inside link text, only the interior link is retained.
```markdown
[a [link to the sun](https://sun.stars.org)](sun.md)
```
[a [link to the sun](https://sun.stars.org)](sun.md).  
**html:** `[a <a href="https://sun.stars.org">link to the sun</a>](sun.md)`

---
For an image inside image alt text, only the exterior image is retained, and all texts are flattened.
```markdown
![image of an ![image of the sun](sun.png)'s corona](corona.png)
```
![image of an ![image of the sun](sun.png)'s corona](corona.png),
and if the image file is missing:
![image of an ![image of the sun](sun.png)'s corona](corona.png-MISSING).  
**html:** `<img src="corona.png" alt="image of an image of the sun&apos;s corona" />`
