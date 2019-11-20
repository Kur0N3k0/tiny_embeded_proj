all: client

client:
	g++ -o client client.cpp -lpthread

arm-client:
	arm-linux-gnueabihf-g++ -o client client.cpp -lpthread

clean:
	rm client
