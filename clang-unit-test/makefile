CC = clang++
FLAG = -std=c++17 -stdlib=libc++ -Wall -Werror -Wfatal-errors -Wno-parentheses -pedantic

LIB = -lpthread -lc++

INCLUDE = -I ../include/ -I ../unit-test-common/

OBJS = guarantee-test.o life-time-cref-vtr-test.o life-time-iterator-storage-test.o\
	life-time-ref-vtr-test.o life-time-shared-ptr-vtr-test.o life-time-vtr-test.o\
	set-operation-test.o value-category-test.o iterator-default-construct-test.o\
	long-query-test.o

main : $(OBJS)
	$(CC) $(FLAG) $(OBJS) ../unit-test-common/main.cpp $(INCLUDE) $(LIB)

%.o : ../unit-test-common/%.cpp
	$(CC) $(FLAG) -c $< $(INCLUDE)
