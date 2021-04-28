main.o: lex.yy.c main.tab.c main.tab.h headers.h
	gcc lex.yy.c main.tab.c -o main.o

main.tab.c main.tab.h: main.y headers.h
	bison -d main.y 

lex.yy.c: main.l headers.h main.tab.h
	flex main.l

.PHONY : clean
clean :
	rm -f *.o lex.yy.c y.tab.c y.tab.h *.out main.tab.c main.tab.h


