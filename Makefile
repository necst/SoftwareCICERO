NAME = cicero
CC = g++
INCDIR = ./include
SRCDIR = ./src
LIBDIR = ./lib
TESTDIR = ./test
CFLAGS = -Wall -O3 -I $(INCDIR)

all: $(NAME) test

test: testbase testmulti

$(NAME):
	$(CC) $(SRCDIR)/*.cpp $(LIBDIR)/*.cpp -o $@ $(CFLAGS) 

testbase:
	$(CC) $(TESTDIR)/testBase.cpp $(LIBDIR)/CiceroBase.cpp -o $(TESTDIR)/testBase $(CFLAGS) 
	cd $(TESTDIR) && ./testBase > testBase.results
	diff -s	$(TESTDIR)/testBase.results	$(TESTDIR)/protomata.results

testmulti:
	$(CC) $(TESTDIR)/testMulti.cpp $(LIBDIR)/CiceroMulti.cpp -o $(TESTDIR)/testMulti $(CFLAGS) 
	cd $(TESTDIR) && ./testMulti > testMulti.results
	diff -s	$(TESTDIR)/testMulti.results $(TESTDIR)/protomata.results
