## Parsare

In fisierul data.h am structura rtable_entry ce modeleaza forma unui
element din tabela de rutare pe care o pastrez sub forma unui vector
de astfel de structuri. Prin functia parse_rtable (din data.h) citesc
din fisierul rtable.txt linie cu linie si dublez dimensiunea vectorului
cand este necesar.

## Protocolul Arp

Cand primesc un pachet verific daca ether_type este de tip Arp
dupa care daca este Arp request caz in care modific pachetul primit
completand macul routerului si inversand ether_shost cu ether_dhost
si arp_tpa cu arp_spa.

Daca am primit un Arp reply completez tabela mea arp cu macul.ul primit
si parcurg coada pentru a trimite pachetele din acestea.

Daca am primit un pachet pentru care nu cunosc adresa mac fac un pachet nou
de tip Arp request, completez adresa broadcast si celelalte campuri necesare
(arp_spa pentru ip-ul a carui mac il caut, arp_op pentru tipul operatiei...).
Mesajul il bag in coada si trimit requestul.

## Procesul de dirijare

Tabela de rutare o sortez prin qsort crescator dupa prefix si masca iar parcurgerea
o realizez prin cautare binara. Dirijarea este precum in laboratorul 4, primesc un pachet,
verific daca este de tip arp, caz in care se intampla cele relatate la punctul 2,
verific daca este icmp echo request si intorc pachetul in cazul in care este, verific ca 
pachetul sa nu aiba ttl < 1 (daca are trimit icmp time exceeded), recalculez suma de control
(daca difera arunc pachetul), caut in tabela de rutare cea mai buna ruta pentru pachet
(daca nu gasesc trimit icmp host unreachable), decrementez ttl.ul, recalculez suma de control, 
caut in tabela arp macul corespunzator si trimit pachetul.

## Protocolul ICMP

-daca routerul primeste un icmp echo request destinat lui raspunde inversand sursa cu 
destinatia pachetului si schimbandu-i tipul in ICMP_ECHOREPLY.

-daca ttl-ul este <= 1 modifica tipul pachetului, inverseaza sursa cu destinatia si 
completeaza restul campurilor necesare dupa care trimite pachetul.

-daca nu gasesc intrare in tabela de rutare trimit un pachet icmp host unreachable la fel
ca la subpunctul anterior dar cu tipul ICMP_DEST_UNREACH.
