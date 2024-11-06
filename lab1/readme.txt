1. laboratorijska vježba - Upravljačka petlja

Unutar programa postoje 2 vrste dretvi
1. Simulacijska dretva koja svaki period stavlja 1 zadatak u red cekanja. Red cekanja postoji posto su svi zadaci jednakog prioriteta pa se rjesava najstariji.
2. Upravljačka dretva koja uzima prvi element reda cekanja te ga obrađuje - dretva spava period potreban da se zadatak obradi.

Funkcija main, prvo cita file input.txt gdje se nalaze razliciti zadaci u obliku {period, prvaPobuda, vrijemeObrade, brojUlaza}. Od svakog ovakvog zadatka stvori strukturu koja se kasnije koristi u dretvama. Zatim program pokrene upravljačku dretvu koja obrađuje zadatke i onda za svaki zadatak stvori 1 simulacijsku dretvu. Podaci o rješenim zadacima se spremaju u vektor stats. Tip podataka u tom vektoru je struktura koja sprema vrijeme generiranja zadatka, vrijeme pocetka obrade i sam zadatak.
postavljanjem bilo kakvog inputa u terminal, zaustavlja se simulacija i ceka se da sve dretve zavrse s radom prije nego sto se pocne racunat statitsika.
Tokom rada svih dretvi, pocetak i kraj obrade zadataka i generiranje zadataka se sprema u file output.txt.

Varijable koje koriste sve dretve su zasticene semaforima.

Luka Buljeta 0036539861
