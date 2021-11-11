
all: client server

client: src/client.cpp src/client.h
	g++ -o client src/client.cpp

server: src/server.cpp src/server.h
	g++ -pthread -o server src/server.cpp

clean:
	rm server client