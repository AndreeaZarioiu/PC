# Protocoale de comunicatii:
# Makefile


# Portul pe care asculta serverul (de completat)
PORT = 8080
ID = andreea200
# Adresa IP a serverului (de completat)
IP_SERVER = 127.0.0.1

build:
	g++ -std=c++11 server.cpp -o server
	g++ -std=c++11 subscriber.cpp -o subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_client:
	./subscriber ${ID} ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber

