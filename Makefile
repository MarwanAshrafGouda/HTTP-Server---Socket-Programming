
all: client server

client: src/client.cpp
	g++ -o client src/client.cpp

server: src/server.cpp
	g++ -o server src/server.cpp

clean:
	rm server client