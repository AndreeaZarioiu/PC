helper.h

In fisierul helper.h am definit urmatoarele structuri:
	udp_message - pentru mesajele primite de la clientii udp
	from_subscriber - pentru mesajele primite de la clientii tcp
	tcp_message - pentru mesajele pe care le trimite serverul
				  la clientii abonati
	client - datele despre un client tcp abonat



server.cpp

Serverul:
 	-asteapta conexiuni tcp, daca a primit un request de la un client tcp
 	completeaza o structura client cu ip.ul, portul si file descriptorul
 	acestuia dupa care asteapata sa primeasca id.ul clientului. Clientul
 	este inserat intr-un unordered map (subscribers), iar la primirea id-ului
 	acesta este completat in structura respectiva si afisat mesajul cerut
 	daca id.ul nu este deja folosit de
 	alt client, caz in care se trimite un mesaj de eroare clientului si este sters
 	din subscribers. Daca id-ul este unic se verifica daca clientul nu a mai fost
 	conectat la server, adica daca id-ul sau se gaseste in unsubscribed, daca da
 	i se trimit mesajele de la topicurile la care se abonase cu sf 1 din backUp si
 	este reabonat la topicurile la care se abonase inainte sa se deconecteze
 	
 	-asteapta mesaje udp pentru socketul udp_socket si le converteste prin functia
 	convert_msg in mesaje pentru clientii tcp (tcp_message) in functie de tipul 
 	acestora dupa care le trimite la clientii abonati la topicul respectiv
 	sau le pastreaza in backUp pentru clientii care s-au deconectat si erau abonati la
 	topicurile respective cu sf 1

 	-asteapta doar comanada exit de la stdin pentru care trimite clientilor comanda exit
 	si ii deconecteaza, inchizand toti socketii.

 	-asteapta comenzi de la clientii tcp sub forma from_subscriber, daca sunt 
 	de subscribe ii insereaza intr-un unorder_map, domains, parcurs de fiecare data 
 	cand serverul primeste mesaje udp, iar daca sf.ul este 1 ii adauga si in toStore, 
 	pentru situatia in care se vor deconecta de la server. Daca mesajul este de unsubscribe,
 	topicul respectiv este sters din domains si din toStore. ifEverReturn pastreaza domeniile
 	la care se aboneaza un client indiferent de sf.



subscriber.cpp

Clientul udp asteapta ca argument id.ul si verifica daca acesta are mai mult de 10 caractere
caz in care trimite meaj de eroare si opreste programul. Dupa ce se conecteaza la server isi
trimite si idul iar daca primeste eroarea de la server ca id-ul este deja folosit de alt client
afiseaza "Id already in use" si se inchide. Clientul asteapta de la server mesaje sub forma
tcp_message pe care le afiseaza sub forma ceruta. Daca primeste mesaje de la stdin de exit, 
subscribe sau unsubscribe acesta le proceseaza, altfel trimite mesaj de eroare si le ignora.
Pentru exit clientul se deconecteaza de la server si se inchide iar pentru subscribe sau 
unsubscribe completeaza un mesaj de tipul from_subscriber, il trimite serverului si daca 
l-a trimis cu succes, consemneaza actiunea realizata afisand subscribed/unsubscribed si 
numele topicului.






