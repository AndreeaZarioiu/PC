## client.c

* exit : inchide programul
* register : primeste username si password de la stdin, fiecare pe
	   o linie noua si trimite mesajul modificat prin 
	   functia compute_post_request catre server apoi afiseaza eroare 
	   daca exista, daca nu afiseaza "Registration successful".
* login : primeste username si password, fiecare pe
	   o linie noua si trimite mesajul modificat prin 
	   functia compute_post_request catre server apoi afiseaza eroare
	   daca exista, daca nu afiseaza "Welcome!"si pastreaza tokenul de
	   login in cookiel.
* enter_library : trimite tokenul primit la login la server printr-un GET si
		pastreaza tokenul primit inapoi in stringul token.
		Daca in raspuns nu primeste eroare afiseaza "Welcome to our
		library!". 

* get_books : trimite tokenul primit de la enter_library la server printr-un 
		GET si afiseaza eroare daca a primit una altfel afiseaza 
		cartile parsate prin json::parse. 


* add_book : Primeste pe linii separate informatiile despre carte de la stdin si
	   trimite mesajul format prin compute_post_request la server.
           Daca a primit eroare o afiseaza, altfel afiseaza "Book added!"
* delete_book : Primeste id.ul si trimite la server cerere de DELETE, daca 
		primeste eroare o afiseaza, altfel afiseaza "Book deleted!"
* logout : Trimite cerere GET si afiseaza eroare daca primeste una de la server, 
	altfel afiseaza "You successfully logged out!".
* get_book : trimite tokenul primit de la enter_library la server printr-un GET si
		afiseaza eroare daca a primit una altfel afiseaza informatiile 
		cerute parsate prin json::parse. 

*O comanda gresita nu este interpretata si se afiseaza "Unknown command"

## request.h/request.cpp

Implementarea functiilor compute_get_request, compute_post_request, 
compute_delete_request si send_to_server.

* Nlohmann - 
Am folosit biblioteca nlohmann pentru parsarea 
listei de carti primite la get_books. In functiile din main, printBookData 
si printLibrary trimit 
portiunea din raspunsul de la server ce trebuie parsata si prin 
json::parse obtin un array de obiecte json pe care il parcurg.
	
* Makefile
	make run - compileaza si ruleaza programul

