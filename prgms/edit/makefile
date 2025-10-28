CXX:=i686-tritium-g++
CXXFLAGS:=-fno-exceptions -g
OUTPRG=EDIT.PRG
OUTFLDR=bin

.PHONY: all clean upload
.SUFFIXES: .o .cpp

OBJS=\
main.o \

all: $(OUTPRG)

$(OUTPRG): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(OUTPRG)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

dump: $(OUTPRG)
	i686-tritium-objdump -D $(OUTPRG)

clean:
	rm -f $(OBJS)
	rm -f *.d
	rm -f $(OUTPRG)

install: $(OUTPRG)
	mcopy -o -i ../../exampledisk.img $(OUTPRG) ::$(OUTFLDR)/$(OUTPRG)
