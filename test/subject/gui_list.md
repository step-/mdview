List
* a1

---

List
* A
a b c d e f g h i j k l m n o p q r s t u v w x y z a b c d e f g h i j k l m n o p q r s t u v w x y z a b c d e f g h i j k l m n o p q r s t u v w x y z a b c d e f g h i j k l m n o p q r s t u v w x y z
* B
a b c d e f g h i j k l m n o p q r s t u v w x y z a b c d e f g h i j k l m n o p q r s t u v w x y z a b c d e f g h i j k l m n o p q r s t u v w x y z a b c d e f g h i j k l m n o p q r s t u v w x y z BR  
a b c d e f g h i j k l m n o p q r s t u v w x y z a b c d e f g h i j k l m n o p q r s t u v w x y z a b c d e f g h i j k l m n o p q r s t u v w x y z a b c d e f g h i j k l m n o p q r s t u v w x y z

---

Loose list - "f" is indented more than "e" and "g"
1. e BR  

   1. f

1. g
<!-- this comment ends the list above -->
Tight list - "f" is indented more than "e" and "g"
1. e BR  
   1. f
1. g

---

Mixed UL/OL list
1. Fruit
   * Apple
1. Dairy
   1. Milk

---

List - two numbered levels
1. a1
    1. a1,1

    1. a1,2
    1. a1,3
1. a2
    1. a2,1
    1. a2,2

---

List - six numbered levels
1. a1
    1. a1,1

1. a2
    1. a2,1

    1. a2,2
        1. a2,2,1
            1. a2,2,1,1
                1. a2,2,1,1,1
                    1. a2,2,1,1,1,1
a b c d e f g h i j k l m n o p q r s t u v w x y z a b c d e f g h i j k l m n o p q r s t u v w x y z a b c d e f g h i j k l m n o p q r s t u v w x y z a b c d e f g h i j k l m n o p q r s t u v w x y z
                1. a2,2,2,1,1
            1. a2,2,2,1
        1. a2,2,2
    1. a2,3

<!-- x -->

Mixed list
1. Fruit
   * Apple
   * Orange
   * Banana
1. Dairy
   1. Milk
   1. Cheese

----

List - item "c" is indented more than "a" and "b"
* "a" [A](U) /filea
* "b" [B](U) /fileb

    * "c" [C](U) /filec

<!-- x -->

Email address in fenced code block in list

* foo
*
    ```
    foo@bar.org
        new line (+4 spaces)
    ```

List with strong emphasis

- **strong**
- _emphasis_
- **strong** and _emphasis_
- ***strong emphasis*** and ___emphasis strong___
    - **strong** and _emphasis_ (level 2)

----

List in blockquote (WIP)

> 1. BR  
foo BR  
bar

<!-- x -->

Blockquote in list (WIP)

* > 1. BR  
foo BR  
bar
