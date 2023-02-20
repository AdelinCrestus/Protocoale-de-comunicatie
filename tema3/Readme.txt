Crestus Adelin 321CD

Am luat laboratorul 10 drept schelet.

    Am retinut buffere pt stringul pe care il citim de la tastatura pentru comenzi, payloadul pe care il vom pune cerereii HTTP,
dar si pentru cookie-ul sesiunii si pentru token ul JWT.

    Ciclam la infinit pana cand primim comanda exit.
    Daca primim de la tastatura register afisam prompt pentru username si password.
    Formam jsonul (am ales sa-l formez singur fara parson, pentru ca nu prea aveam semnal bun pentru a descarca parson de pe git ... cand m-am apucat de tema, iar avand in vedere ca erau doar cateva linii de scris n-am mai schimbat nici dupa).
    apelam functia din lab pentru a creea post_requestul si il trimitem spre server.
    Primim raspunsul, iar daca avem un cod care incepe cu 2(Ex 200, 201) inseamna ca a actiunea a avut succes.
    Altfel afisam un program de eroare(username ul deja exista).

    Pentru login afisam aceleasi prompt-uri si construim payloadul asemanator.
    Va fi diferita calea catre care va fi trimisa cererea, iar din raspunsul serverului vom retine cookie-ul.

    enter_library: Verificam daca lungimea cookie ului este > 0.
    Daca nu inseamna ca nu suntem logati.
    Altfel construim get_requestul cu functia din laborator.
    Din raspunsul serverului vom retine tokenul JWT.
    Am procedat similar si la toate celelalte actiuni.
    La delete am considerat ca singura diferenta dintre o cerere delete si una get este doar primul cuvant.
    Asa ca am creat tot o cerere get pentru ca aveam deja functie care facea asta, iar dupa doar am inlocuit GET cu DELETE.

    Pt logout dupa ce a reusit am golit ses_cookie si token.
