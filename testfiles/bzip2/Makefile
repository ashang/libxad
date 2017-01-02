test: test1 test2 test3 test4 test5

test1:
	xadlist .

test2:
	rm -f sample1
	xadunfile sample1.bz2 .
	cmp sample1.ref sample1
	rm -f sample2
	xadunfile sample2.bz2 .
	cmp sample2.ref sample2
	rm -f sample3
	xadunfile sample3.bz2 .
	cmp sample3.ref sample3

test3:
	rm -f empty empty.ref
	touch empty.ref
	xadunfile empty.bz2 .
	cmp empty.ref empty

test4:
	rm -f panda
	xadunfile panda.bz2 .
	cmp poo.ref panda

test5:
	rm -f file[12]* both* unnamed.dat
	echo >file1.ref "this is file 1"
	echo >file2.ref "this is file 2"
	bzip2 -1 <file1.ref >file1.bz2
	bzip2 -9 <file2.ref >file2.bz2
	cat file1.ref file2.ref >both.ref
	cat file1.bz2 file2.bz2 >both.bz2
	xadunfile both.bz2 .
	cmp both.ref both

clean:
	rm -f file[12]* both* unnamed.dat panda empty empty.ref sample1 sample2

.PHONY: test test1 test2 test3 test4 test5
