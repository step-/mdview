# Subfile.md

## Pre-conditions

* `arg[1]` is `examples/relative.md` therefore `image_directory` is `examples`
* `relative.md` targets this file as `relative/subfile.md`

## Test image

![this should be picture square.svg](square.svg)

this relative image link is `square.svg`;
the image link works because the image file lives in `image_directory`

## Tests

Click the links below sequentially and don't **click the Back button** unless instructed:

*before clicking, the referrer was* `examples/relative.md`

---

### 1 [index.md](index.md)
* this should work because `image_directory` is `examples` and `index.md` lives there
* *after clicking, the referrer becomes* `examples/relative/subfile.md`
* after visiting `index.md` **click the Back button** to return here and continue with the next test...

---

*on returning from* `index.md`*, the referrer became* `examples/relative/subfile.md` *canonicalized* `$PWD/examples/relative/subfile.md`

### 2 [../index.md](../index.md)
* this should work because `../index.md` exists relative to the referrer's directory
* *after clicking, the referrer becomes* `examples/relative/subfile.md`
* after visiting `index.md` **click the Back button** to return here and continue with the next test...

---

*on returning from* `index.md`*, the referrer became* `examples/relative/subfile.md` *canonicalized* `$PWD/examples/relative/subfile.md`

### 3 [subfile1.md](subfile1.md)
* this should work because `subfile1.md` exists relative to the referrer's directory
* after visiting `subfile1.md` **click the Back button** to return here and continue with the next test...

---

*after clicking, the referrer became* `$PWD/examples/relative/subfile.md`

### 4 [relative/subfile1.md](relative/subfile1.md)
* this should work because `relative/subfile1.md` exists relative to `image_directory`
* when you get to `subfile1.md` **continue testing THERE**

### END OF subfile.md
