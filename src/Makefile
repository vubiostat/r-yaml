RHOME=/home/stephej1/src/build/R-2.4.1
RLIB= $(RHOME)/lib
RINC= $(RHOME)/include

all: yaml.so

st.o: st.h st.c
	gcc -std=gnu99 -I$(RINC) -I. -g3 -c st.c -o st.o

yaml.o: yaml.c
	gcc -std=gnu99 -I$(RINC) -I. -g3 -c yaml.c -o yaml.o

yaml.so: st.o yaml.o
	gcc -std=gnu99 -shared -o yaml.so yaml.o st.o -L$(RLIB) -lR -lsyck

clean:
	rm -f *.o *.so test_Rhash
