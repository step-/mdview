### Output by fd-faq AST/BulletList.sh

<!-- can convert to HTML with command
 mdview --html --html5 --html-full test/subject/gui_list_pandoc_4space_rule.md > /tmp/pandoc_bullets.html
-->

Test: show output of pandoc's 4-space indentation style.

Comment: mdview cannot render consecutive bullets straight
inline as the online dingus can. See also the fd-faq project,
which implements a work-around to hide intermediate bullets.

### 4-space rule

Pandoc `markdown_strict` output for @MDVIEW_MD@.
This output coincides with pandoc's native `markdown` format.

View it online: [dingus]

 -   -   -   -   -   5

<!-- -->

-   1
-   -   2
    -   -   3
        -   -   4
            -   -   5

[dingus]: <https://spec.commonmark.org/dingus/?text=%23%23%23%204-space%20rule%0A%0A%20-%20%20%20-%20%20%20-%20%20%20-%20%20%20-%20%20%205%0A%0A%3C!--%20--%3E%0A%0A-%20%20%201%0A-%20%20%20-%20%20%202%0A%20%20%20%20-%20%20%20-%20%20%203%0A%20%20%20%20%20%20%20%20-%20%20%20-%20%20%204%0A%20%20%20%20%20%20%20%20%20%20%20%20-%20%20%20-%20%20%205%0A%23%23%23%202-space%20rule%0A%0A%20-%20-%20-%20-%20-%205%0A%0A%3C!--%20--%3E%0A%0A-%201%0A-%20-%202%0A%20%20-%20-%203%0A%20%20%20%20-%20-%204%0A%20%20%20%20%20%20-%20-%205%0A>

### 2-space rule

Pandoc `commonmark` output for @MDVIEW_MD@.

View it online: [dingus]

 - - - - - 5

<!-- -->

- 1
- - 2
  - - 3
    - - 4
      - - 5

