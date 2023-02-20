Crestus Adelin 321CD

Am implementat o aplicatie client-server tcp

Structuri de date folosite:

- DIE : Macro-ul din lab de verificare a erorilor

- T_udp_data : structura in care retin datele primite de la clientii UDP
    contine campurile din cerinta: topic, tip_date si continut

-  TQmsg - coada de mesaje(fiecare client TCP care este inactiv si are sf 1 pentru anumite topicuri
             va avea o astfel de coada alocata pe server in care i se vor pune mesaje)

-  Ttopic - structura in care retin coada de abonati pt fiecare topic, retin last pentru a face inserarea in O(1),
        fara a mai parcurge lista

-  Tclient - este structura in care serverul va retine toate datele despre clientii tcp.
        sf - vector de intregi (sf[i] = 0, inseamna ca pt topicul de pe pozitia i din vectorul de topicuri avem dezactivata optiune s&f)
                                pt sf[i] = 1 aceasta va fi activata
        q_data - coada de mesaje pe care le avem de trimis pentru atunci cand clientul va fi pornit
        Am retinut si ultimul element al cozii pt a insera in O(1).

Flow:

  ****  Server ****


    Deschidem conexiunile tcp si udp folosind socketi.
    Am dezactivat algoritmul lui Nagle.
    Ciclam intr-o bucla infinita si folosind select folosim multiplexarea conexiunilor si a inputului de la tastatura.

    Daca primim o cerere de conexiune folosim accept pentru a stabili o conexiune tcp.
    Daca deja am ajuns la limita cu vectorul de clienti il realocam cu dimensiune dubla.

    Imediat dupa ce a dat accept serverul trimite pe noul canal deschis mesajul "Give me your id".
    Indata ce il primeste clientul isi trimite id ul catre server.
    Serverul verifica daca deja exista id ul in vectorul lui de clienti.
    Daca da si este si on (are campul de enable egal cu 1), serverul ii trimite mesajul "Id exists"
    si inchide conexiune. Clientul se va opri si el la primirea acestui mesaj.
    Daca clientul exista in vectorul de clienti, dar era oprit, vom actualiza noile informatii si daca are 
    mesaje in coada de mesaje serverul le va trimite si va goli coada.

    Daca nu exista adaugam noul client in vectorul clientilor completand campurile acestuia cu informatiile din structura Tclient.

    Daca primim input de la tastatura verificam daca este comanda exit, iar in caz afirmativ 
    trimitem mesajul "server down" catre toti clientii care sunt activi. Dupa ce primim mesajul de confirmare "I'm out" inchidem conexiunile si serverul.

    Daca primim o datagrama verificam daca avem topicul primit in lista de topicuri, cu functia exist_topic,
    care intoarce pozitia topicului in vectorul de topicuri daca exista si -1 altfel.
    Daca functia returneaza -1 vom adauga noul topic in vector.

    Apoi serverul construieste stringul pe care sa l trimita clientilor tcp, in functie de datele din datagrama.
    Dupa ce se termina constructia stringului se parcurge lista de clienti ai topicului din datagrama.
    Daca clientul este activ i se trimite mesajul cu send, daca nu este activ, dar are optiunea s&f activa pentru topicul respectiv, se va pune mesajul in coada de mesaje.


    In cazul in care primim comanda de la unul din clientii tcp
    Poate fi doar de -subscribe, iar atunci il vom pune in coada de abonati ai topicului la care se aboneaza si setam sf-ul corespunzator.
                     -unsubscribe - serverul va sterge clientul din lista de abonati ai topicului respectiv.
                     - Daca primim mesajul "I'm out" setam campul enable al clientului de la care s-a primit pe 0 si stergem file descriptorul, iar serverul afiseaza mesajul de client deconectat.


    **** Client ****

    Parsam argumentele si trimitem cererea de conectare la server.
    Multiplexam cu select asemanator ca in server.
    Exista 2 posibilitati: 
    - sa se primeasca input de la tastatura
    - sa se primeasca mesaje de la server 

    Daca primim input de la tastatura il trimitem serverului
    Daca primim mesaje de la server, doar le afisam.

    Solutia propusa de mine implementeaza si protocolul de nivel aplicatie, iar trunchierile si concatenarile tcp-ului nu afecteaza ceea ce afiseaza clientul pe ecran, deoarece 
    fiecare string pe care il trimit are \n la final. Concatenand mai multe siruri care au \n printre ele si afisand unul singur, rezultatul va fi acelasi.
    De asemenea daca se trunchiaza din mesaj. La stdout se va afisa in continuare cand va veni restul mesajului.

    Am observat ca daca comentez testul de quick_flow se inchide si serverul corect, dar nu imi dau seama dc nu se mai intampla asta cu testul rulat. Probabil mai raman in bufferele cu care compar stringurile
    niste caractere ce nu ar trebui sa fie acolo, dar nu mi-a mai ramas timp sa studiez atent problema.....
