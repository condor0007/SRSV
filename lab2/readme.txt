Druga laboratorijska vjezba

Unutar funkcije main se ulazni podaci spreme u strukturu zadatak, inicijalizira se dijeljena memorija i stvore se subprocesi.
Za svaki ulaz se stvori jedan proces SimulacijskaPetlja koja periodički stvara zadatke.
Stvara se proces sat koji periodički šalje SIGUSR1 na proces UpravljačkaPetlja
Proces UpravljačkaPetlja nakon svakog signala sprema podatke zadatka kojeg je do nedavno rješavao pa odna odlučuje o sljedećem zadatku bazirano na broju duplih perioda u zadnjih 10 prekida, izlazu funkcije daj_iduci(koji odabire sljedeći zadatak).
