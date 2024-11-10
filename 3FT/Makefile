#--------------------------------------------------------------------
# Makefile for Assignment 4, Part 3
# Authors: Will Grimes, David Wang
#--------------------------------------------------------------------

# GCC = gcc217m
GCC = gcc217

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
