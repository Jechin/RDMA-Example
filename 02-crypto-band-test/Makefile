LD		:= g++
LDLIBS  := ${LDLIBS} -lrdmacm -libverbs -lpthread -lssl -lcrypto
DEPS	:= rdma.h
OBJ		:= client.o server.o
APPS	:= client server

all: ${APPS}

client:
	${LD} -o $@ client.cpp ${LDLIBS}

server:
	${LD} -o $@ server.cpp ${LDLIBS}

.PHONY: clean
clean:
	rm -f *.o client server