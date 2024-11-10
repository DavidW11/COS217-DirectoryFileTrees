#--------------------------------------------------------------------
# Makefile for Assignment 4, Part 2
# ft* targets are built using checkerFT
# rules to build ftBad*.o and nodeBad*.o from source will fail
# Author: Christopher Moretti
#--------------------------------------------------------------------

# GCC = gcc217
GCC = gcc217m

TARGETS = ft

.PRECIOUS: %.o

all: $(TARGETS)

clean:
	rm -f $(TARGETS) meminfo*.out

clobber: clean
	rm -f dynarray.o path.o ft_client.o checkerFT.o node.o ftGood.o *~

ft: dynarray.o path.o checkerFT.o node.o ft.o ft_client.o
	$(GCC) -g $^ -o $@

dynarray.o: dynarray.c dynarray.h
	$(GCC) -g -c $<

path.o: path.c dynarray.h path.h a4def.h
	$(GCC) -g -c $<

ft_client.o: ft_client.c ft.h a4def.h
	$(GCC) -g -c $<

checkerFT.o: checkerFT.c dynarray.h checkerFT.h node.h path.h a4def.h
	$(GCC) -g -c $<

node.o: node.c dynarray.h checkerFT.h node.h path.h a4def.h
	$(GCC) -g -c $<

ft.o: ft.c dynarray.h checkerFT.h node.h ft.h path.h a4def.h
	$(GCC) -g -c $<
