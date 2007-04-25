RHOME=/home/stephej1/src/build/R-2.4.1
RLIB= $(RHOME)/lib
RINC= $(RHOME)/include

all: test

test: test_R test_C

hash_test: hash_test.o st.o
	gcc -std=gnu99 -o hash_test st.o hash_test.o -L$(RLIB) -lR -Wl,-rpath,$(RLIB)

hash_test.o: hash_test.c
	gcc -std=gnu99 -I$(RINC) -I. -g3 -c hash_test.c -o hash_test.o

st.o: st.h st.c
	gcc -std=gnu99 -I$(RINC) -I. -g3 -c st.c -o st.o

test_C: hash_test
	@echo
	@echo === Running hash test ===
	@echo
	./hash_test

test_R:
	@for i in *test.R; do echo; echo === Running $$i ===; echo; myR < $$i; done

clean:
	rm -f *.o *.so hash_test
