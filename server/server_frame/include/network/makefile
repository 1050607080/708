#-----------------------------------------------------------

INC =  -I./ -I../utility -I../

#-----------------------------------------------------------
BINARY = libnet.a
BASEOBJS = $(patsubst ./%.cpp, ./%.o, $(wildcard ./*.cpp))

#-----------------------------------------------------------
#����all clean��αĿ��
.PHONY: all clean

all: $(BINARY)
	cp $(BINARY) ../../lib/

$(BINARY): $(BASEOBJS)
	ar -cvr $@ $^

clean:
	rm -f $(BASEOBJS) $(BINARY)

%.o: %.cpp
	g++ $(C_ARGS) -g -fPIC $(INC) -o $@ -c $< -Wno-deprecated

