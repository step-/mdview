all: mdview.1

mdview.1: mdview.1.h2m make_mdview_1.sh Makefile ../src/mdview
	./make_mdview_1.sh $< $@ ../src/mdview

clean:
	$(RM) -v mdview.1 gmon.out

.PHONY: all clean
