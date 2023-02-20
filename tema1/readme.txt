Crestus Adelin 321CD

Am implementat in aceasta tema actiunile pe care le face un router in procesul de forwarding al pachetelor!

send_arp: Primeste headerele eth si arp, interfata pe care trimite si lungimea packetului.
Plaseaza headerele pe pozitiile lor, completeaza campurile structurii packet si trimite mesajul.

cautare_binara: Rezolva lpm. Primeste headerul ip pt a lua din el adresa destinatie, un pointer la struct_route_table_entry alocat
pe care il va popula, un pointer la tabela de rutare si lungimea acesteia.

Retinem masca maxima si indexul in variabile. Cat timp stanga <= dreapta verificam daca ip_hdr->daddr & rtable_ent[m].mask este egal cu rtable_ent[m].prefix.
Daca se respecta conditia practic avem un candidat, dar va trebui sa verificam si daca masca acestuia este maxima gasita pana acm.
Daca da actualizam destinatia, masca maxima si indexul. Apoi cautam in dreapta liniar pentru o eventuala masca mai mare cat timp avem match(tabela este sortata dupa prefix crescator si apoi dupa masca).
Daca adresa destinatiei este mai mica decat prefixul. Este clar ca atunci cand vom face & cu o masca vom obtine o valoare <= ip_destinatie. Deci va trebuie sa mergem in jumatatea din stanga a vectorului(prima jumatate).
Alfel mergem in a 2a jumatate.
Cand ajungem la s==d si anume la ultimul element de comparat. Daca face match verificam si daca masca este maxim si returnam indexul retinut indiferent daca mai face match sau nu (exista posibilitatea sa fii facut alti indici de dinainte).

cautare_binara_next_hop este foarte asemanatoare cu cea de mai sus insa este folosita pentru a afla interfata pe care trebuie trimis un arp_reply.

comparatorf: comparator pt qsort, ajuta la sortarea crescatoare inaintai dupa prefix si in caz de egalitate dupa masca.

Flow:
Primim un packet. Extrag headerul ethernet. Retin in variabile adresa mac a routerului si adresa ipv4.
Daca packetul este prea mic, ii dam drop.

Daca a venit pe adresa de broadcast sau pe adresa mac a routerului verificam daca este arp sau ip.
Daca este arp, avem 2 posibilitati: sa fie request sau reply.


Daca este request trimitem inapoi un arp reply, care contine pe adresa sursei adresa mac a routerului, iar pe a adresa destinatiei, sursa pachetului primit.(Asemanator procedam si pt headerul arp).
Cautam cu functia cautare_binara_next_hop ca sa stim pe ce interfata trimitem si daca gasim un index apelam functia  send_arp.

Daca este reply inseamna ca asteptam raspuns pentru a redirectiona un pachet din coada. Cand primim raspunsul populam adresa de destinatie din headerul ethernet si o adaugam in tabela mac-ul asociat pentru adresa ip pentru care am primit reply.
Trimitem pachetul extras din coada.

Pentru un pachet de tip IP verificam daca este pentru noi sau daca trebuie redirectat.
Daca este pentru noi verificam daca este un request de icmp si trimitem inapoi un reply, prin inversarea adreselor sursa si destinatie si actualizarea checksumului icmp.

Daca nu este pentru noi verificam daca checksumul este valid. Daca ttl ul este 0 sau 1 trimitem o eroare cu ajutorul imcp.
Altfel decrementam ttl ul din headerul ip si cautam binar in tabela de routare next hop.
Daca il gasim actualizam checksum-urile si verificam daca in tabela arp avem deja pentru adresa ip a urmatorului hop, o adresa mac asociata.
Daca gasim o scriem ca destinatie in headerul ethernet si trimitem pachetul.
Altfel punem pachetul in coada si generam un arp request pe care il dam pe broadcast cu adresa ip a urmatorului hop. Urmatorul nod care are adresa ip cautata va trimite reply-ul si vom pune adresa acestuia hardware in headerul ethernet, putand redirecta pachetul spre acesta.

Daca nu gasim un hop urmator, trimitem icmp error destination unreachable catre sursa care a trimis pachetul.

Am rezolvat toate cerintele, insa la partea de imcp error cred ca mai sunt unele mici probleme.
