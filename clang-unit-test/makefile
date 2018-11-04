CC = clang-6.0
FLAG = -std=c++17 -g -Wall -Werror -Wfatal-errors -Wno-parentheses -stdlib=libc++

LIB = -lpthread -lc++ -lc++abi -lm -lc -lgcc_s -lgcc

INCLUDE = -I ../include/ -I ../unit-test-common/

OBJS = basic-combined-query-test.o guarantee-test.o life-time-cref-vtr-test.o\
	life-time-ref-vtr-test.o life-time-shared-ptr-vtr-test.o life-time-vtr-test.o\
	set-operation-test.o value-category-test.o

main : $(OBJS)
	$(CC) $(FLAG) $(OBJS) ../unit-test-common/main.cpp $(INCLUDE) $(LIB)

%.o : ../unit-test-common/%.cpp
	$(CC) $(FLAG) -c $< $(INCLUDE)
