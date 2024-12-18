#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <unistd.h>
#include <chrono>
#include <queue>
#include <mutex>
#include <climits>
#include <bits/stdc++.h>

using namespace std;

struct zadatak{
    int period;
    int trenutakPrvePojave;
    int trajanjeObrade;
    int brojUlaza;
    int idUlaza;
    bool produzetakPerioda;
    int brojProduzetaka;
    int preostaloVrijeme;
    std::chrono::time_point<std::chrono::high_resolution_clock> vrijemePojave;
    };



bool simulacijaTraje = true;

int dvaPerioda = 0;
vector<int> rjeseni;
vector<int> nerjeseni;

ofstream outputFile("output.txt", ios::app);

mutex mtx;
mutex red;
mutex prekid_mtx;

condition_variable prekid_cv;
int prekid = -2;

vector<int> zadA = {}; // <4
vector<int> zadB = {}; // >3, < 19
vector<int> zadC = {}; // > 18
vector<vector<int>> zad = {zadA, zadB, zadC};
vector<zadatak> zadaci;

vector<int> redosljed = {0, 1, -1, 0, 1, -1, 0, 1, 2, -1};
int t = 0; 
vector<int> ind = {0, 0, 0}; 

int daj_iduci() {
    int sljedeci_zadatak = 0;
    int tip = redosljed[t];
    t = (t + 1) % 10;
    if (tip != -1 && zad[tip].size() > 0) {
        cout << zad[tip][ind[tip]];
        sljedeci_zadatak = zad[tip][ind[tip]];
        ind[tip] = (ind[tip] + 1) % zad[tip].size();
        return sljedeci_zadatak - 1; 
    }
    return -1; 
}


void ispisiZaglavlje() {
    lock_guard<mutex> lock(mtx);
    outputFile << "t\tdogađaj\n";
    outputFile << "----------------\n";
}

void sat(){
    while(simulacijaTraje) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        {
            std::lock_guard<std::mutex> lock(prekid_mtx);
            if(prekid < 0){
                prekid = -1;
            }
        }
        prekid_cv.notify_one();
    }
    prekid_cv.notify_one();
}

void posebanPrekid(int idUlaza){
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    while(prekid >= 0){
        continue;
    }
    {
        std::lock_guard<std::mutex> lock(prekid_mtx);
        prekid = idUlaza;
        
    }
    prekid_cv.notify_one();
}

void upravljackaPetlja(){
    //std::cout << "upravljacka petlja" << std::endl;

    auto pocetnoVrijeme = std::chrono::high_resolution_clock::now();
    auto vrijeme_pocetka = std::chrono::high_resolution_clock::now();

    int id_zad = -1;
    int id_proslog = -1;
    while(simulacijaTraje){

         {
            std::unique_lock<std::mutex> lock(prekid_mtx);
            prekid_cv.wait(lock, [] { return prekid != -2 || !simulacijaTraje; });
         }
        id_proslog = id_zad;
        if(prekid >= 0){
            id_zad = prekid;
        } else{
            id_zad = daj_iduci();
        }

        auto vrijeme_kraja = std::chrono::high_resolution_clock::now();
        auto pocetak_prijasnjeg = vrijeme_pocetka;
        auto vrijeme_pocetka = std::chrono::high_resolution_clock::now();
        

        if (id_proslog >= 0){

            zadatak zad;

            {
                lock_guard<mutex> lock(red);
                zad = zadaci[id_proslog];
            }

            int preostalo = zad.preostaloVrijeme;

            int odradjeno = (int) std::chrono::duration_cast<std::chrono::milliseconds>(
                vrijeme_kraja - pocetak_prijasnjeg).count();

            bool gotov = false;

            if (preostalo < odradjeno){
                {
                lock_guard<mutex> lock(red);
                zadaci[id_proslog].preostaloVrijeme = 0;
                }
                gotov = true;
            } else {
                {
                    lock_guard<mutex> lock(red);
                    zadaci[id_proslog].preostaloVrijeme -= odradjeno;
                }
            }

            if(gotov){
                rjeseni.push_back(zad.idUlaza);
            
            
                int vrijemePocetkaMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   vrijeme_pocetka.time_since_epoch()).count();

            {
                lock_guard<mutex> lock(mtx);
                outputFile << vrijemePocetkaMs << "\tUpr: završena obrada ulaza-" << id_proslog << "\n";
            }
            }

        }

        if (id_zad >= 0){
               int vrijemePocetkaMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   vrijeme_pocetka.time_since_epoch()).count();

            {
                lock_guard<mutex> lock(mtx);
                outputFile << vrijemePocetkaMs << "\tUpr: započinjem obradu ulaza-" << id_zad << "\n";
            }
        }
        if(prekid >= 0){
            zadatak zad;
            {
                lock_guard<mutex> lock(red);
                zad = zadaci[id_zad];
            }

            auto pocetnoVrijeme = std::chrono::high_resolution_clock::now();
            auto vrijemeAktivacije = pocetnoVrijeme + std::chrono::milliseconds(zad.trajanjeObrade);
            std::this_thread::sleep_until(vrijemeAktivacije);

        }

     
    {
        std::lock_guard<std::mutex> lock(prekid_mtx);
        prekid = -2;
        
    }
    }
}

void simulatorUlaza(zadatak zad, int idUlaza){

    zad.idUlaza = idUlaza;

    auto pocetnoVrijeme = std::chrono::high_resolution_clock::now();
    auto vrijemeAktivacije = pocetnoVrijeme + std::chrono::milliseconds(zad.trenutakPrvePojave);

    //std::cout << "simulacijska petlja" << std::endl;

    while (simulacijaTraje) {

        std::this_thread::sleep_until(vrijemeAktivacije);


        if (simulacijaTraje){
            {
                lock_guard<mutex> lock(red); 
                if(zad.brojUlaza == 0){
                    zadaci[idUlaza - 1].preostaloVrijeme = zadaci[idUlaza - 1].trajanjeObrade;
                    posebanPrekid(idUlaza);
                }else if(idUlaza < 4){
                    if(zadA.end() != find(zadA.begin(), zadA.end(), idUlaza) && zad.brojProduzetaka >= 10){
                        zadaci[idUlaza - 1].brojProduzetaka = 0;
                        zadaci[idUlaza - 1].produzetakPerioda = true;
                    } else {
                        zadaci[idUlaza - 1].brojProduzetaka += 1;
                        zadaci[idUlaza - 1].vrijemePojave = std::chrono::high_resolution_clock::now();
                        zadaci[idUlaza - 1].preostaloVrijeme = zadaci[idUlaza - 1].trajanjeObrade;
                        zadaci[idUlaza - 1].produzetakPerioda = false;
                        zadA.push_back(idUlaza);

                    }
                } else if (idUlaza > 18){
                    if(zadC.end() != find(zadC.begin(), zadC.end(), idUlaza) && zad.brojProduzetaka >= 10){
                        zadaci[idUlaza - 1].brojProduzetaka = 0;
                        zadaci[idUlaza - 1].produzetakPerioda = true;
                    } else {
                        zadaci[idUlaza - 1].brojProduzetaka += 1;
                        zadaci[idUlaza - 1].vrijemePojave = std::chrono::high_resolution_clock::now();
                        zadaci[idUlaza - 1].preostaloVrijeme = zadaci[idUlaza - 1].trajanjeObrade;
                        zadaci[idUlaza - 1].produzetakPerioda = false;
                        zadC.push_back(idUlaza);

                    }
                } else {
                    if(zadB.end() != find(zadB.begin(), zadB.end(), idUlaza) && zad.brojProduzetaka >= 10){
                        zadaci[idUlaza - 1].brojProduzetaka = 0;
                        zadaci[idUlaza - 1].produzetakPerioda = true;
                    } else {
                        zadaci[idUlaza - 1].brojProduzetaka += 1;
                        zadaci[idUlaza - 1].vrijemePojave = std::chrono::high_resolution_clock::now();
                        zadaci[idUlaza - 1].preostaloVrijeme = zadaci[idUlaza - 1].trajanjeObrade;
                        zadaci[idUlaza - 1].produzetakPerioda = false;
                        zadB.push_back(idUlaza);

                    }
                }
            }

            int vrijemeAktivacijeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::high_resolution_clock::now()
                                    - pocetnoVrijeme).count();

            {
                lock_guard<mutex> lock(mtx);
                outputFile << vrijemeAktivacijeMs << "\tUlaz-" << zad.idUlaza << ": promjena ulaza\n";
            }

        }

        vrijemeAktivacije += std::chrono::milliseconds(zad.period);

        //cout << idUlaza << std::endl;

    }

}

int main()
{

    std::ifstream inputFile("input.txt");

    if (!inputFile) {
        cerr << "Error - input file ima probleme." << endl;
        return 1;
    }

    std::string line;

    while (std::getline(inputFile, line)) {

        //std::cout << line << std::endl;
        bool t = true;
        size_t index = 0;
        int id = 0;

        while (t){

            size_t substart = line.find("{", index);
            size_t subend = line.find("}", substart);
            index = subend + 1;
            id += 1;

            if (substart != string::npos && subend != string::npos) {

                size_t per = line.find(",", substart);
                int period = stoi(line.substr(substart + 1, per - substart - 1));
                size_t tpp = line.find(",", per + 1);
                int trenutakPrvePojave = stoi(line.substr(per + 1, tpp - per - 1));
                size_t to = line.find(",", tpp + 1);
                int trajanjeObrade = stoi(line.substr(tpp + 1, to - tpp - 1));
                size_t bu = line.find(",", to + 1);
                int brojUlaza = stoi(line.substr(to + 1, bu - to - 1));

                zadatak z = {period, trenutakPrvePojave, trajanjeObrade, brojUlaza, id, false, 10, 0, std::chrono::high_resolution_clock::now()};
                zadaci.push_back(z);

                cout << "Period: " << z.period
                    << ", Trenutak prve pojave: " << z.trenutakPrvePojave
                    << ", Trajanje obrade: " << z.trajanjeObrade
                    << ", Broj ulaza: " << z.brojUlaza << endl;

            } else {
                t = false;
                index = 0;
            }
        }
    }

    inputFile.close();

    ispisiZaglavlje();
    //return 0;
    vector<thread> threads;
    threads.emplace_back(upravljackaPetlja);
    threads.emplace_back(sat);

    for (int i = 0; i < zadaci.size(); ++i) {
        threads.emplace_back(simulatorUlaza, zadaci[i], i + 1);
    }

    std::cout << "Pritisnite Enter za zaustavljanje simulacije..." << std::endl;
    std::cin.get();
    simulacijaTraje = false;

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    lock_guard<mutex> lock(mtx);


    outputFile  << "Statistika: Broj rješenih" << rjeseni.size() << endl;
    outputFile  << "Statistika: Broj nerješenih" << nerjeseni.size() << endl;
    outputFile  << "Statistika: Broj produzetaka" << dvaPerioda << endl;

    outputFile.close();
    
    return 0;
}
