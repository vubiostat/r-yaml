all: yaml.so

yaml.so: yaml.o st.o
	R CMD SHLIB yaml.o st.o

yaml.o: yaml.c
	R CMD COMPILE yaml.c

st.o: st.h st.c
	R CMD COMPILE st.c

clean:
	rm -fr *.so *.o
