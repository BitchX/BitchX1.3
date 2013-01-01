CPP= g++ -Wall -c -g
LINK= g++ -g

all: p1

p1: p1.o gnode.o list.o grammar.o lexicon.o parse.o
	$(LINK) p1.o parse.o grammar.o lexicon.o list.o gnode.o -o p1

t1: t1.o gnode.o list.o
	$(LINK) t1.o gnode.o list.o -o t1
t1.o: t1.cpp
	$(CPP) t1.cpp
t2: t2.o gnode.o list.o
	$(LINK) t2.o gnode.o list.o -o t2
t2.o: t2.cpp
	$(CPP) t2.cpp
t3: t3.o lexicon.o gnode.o list.o
	$(LINK) t3.o lexicon.o gnode.o list.o -o t3
t3.o: t3.cpp
	$(CPP) t3.cpp
t4: t4.o grammar.o lexicon.o gnode.o list.o
	$(LINK) t4.o grammar.o lexicon.o gnode.o list.o -o t4
t4.o: t4.cpp
	$(CPP) t4.cpp
p1.o: p1.cpp
	$(CPP) p1.cpp
grammar.o: grammar.cpp
	$(CPP) grammar.cpp
lexicon.o: lexicon.cpp
	$(CPP) lexicon.cpp
gnode.o: gnode.cpp
	$(CPP) gnode.cpp
list.o: list.cpp
	$(CPP) list.cpp
parse.o: parse.cpp
	$(CPP) parse.cpp
clean:
	rm -f core p1 t1 t2 t3 t4 *.o *~ *#
