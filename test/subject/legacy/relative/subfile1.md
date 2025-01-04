# Subfile1.md

## Pre-conditions

* `arg[1]` is `examples/relative.md` therefore `image_directory` is `examples`
* `subfile.md` targets this file as `relative/subfile1.md`

## Test image

![this should be picture 1.svg](1.svg)

this relative image link is `1.svg`;
its referrer is this file `subfile1.md`;
the image link works because it is relative to its referrer's directory.

## Tests

Continue if you're coming here from `subfile.md`'s test #4

Click the links below sequentially and don't **click the Back button** unless instructed:

*before clicking, the referrer is* `examples/relative/subfile.md` *canonicalized* `$PWD/examples/relative/subfile.md`

---

### 1 [index.md](index.md)
* this should work because `image_directory` is `examples` and `index.md` lives there
* *after clicking, the referrer becomes* `$PWD/examples/relative/subfile1.md`
* after visiting `index.md` **click the Back button** to return here and continue with the next test...

---

*on returning from* `index.md`*, the referrer became* `$PWD/examples/relative/subfile1.md`

### 2 [../index.md](../index.md)
* this should work because `../index.md` exists relative to the referrer's directory
* *after clicking, the referrer becomes* `$PWD/examples/relative/subfile1.md`
* after visiting `index.md` **click the Back button** to return here and continue with the next test...

---

*on returning from* `index.md`*, the referrer became* `$PWD/examples/relative/subfile1.md`

### 3 [subfile.md](subfile.md)
* this should work because `subfile.md` exists relative to the referrer's directory
* after visiting `subfile.md` **click the Back button** to return here and continue with the next test...

---

*after clicking, the referrer became* `$PWD/examples/relative/subfile1.md`

### 4 [relative/subfile.md](relative/subfile.md)
* this should work because `relative/subfile.md` exists relative to `image_directory`
* after visiting `subfile.md` **click the Back button** to return here and continue with the next test...

---

*after clicking, the referrer became* `$PWD/examples/relative/subfile1.md`

### 5 [../relative/subfile.md](../relative/subfile.md)
* this should work because `../relative/subfile.md` exists relative to the referrer's directory
* after visiting `subfile.md` **click the Back button** to return here and end testing

### END OF subfile1.md
