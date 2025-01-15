#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <pthread.h> 
#include <thread>
#include <unistd.h>
#include <chrono>
#include <queue>
#include <mutex>
#include <climits>
#include <sched.h>
#include <time.h>

using namespace std;

struct zadatak{
    int period;
    int trenutakPrvePojave;
    int trajanjeObrade;
    int brojUlaza;
    int idUlaza;
    };

struct statistika {
    int brojZavrsenih = 0;
    int brojNerijesenih = 0;
};
statistika stat;

bool simulacijaTraje = true;

vector<zadatak> zadaci;
vector<int>statusZadataka;

ofstream outputFile("output.txt", ios::app);

mutex mem;
mutex out;

volatile int brojIteracija = 1000000;


void ispisiZaglavlje() {
    lock_guard<mutex> lock(out);
    outputFile << "t\tdogađaj\n";
    std::cout << "t\tdogađaj\n";
    outputFile << "----------------\n";
    std::cout << "----------------\n";
}

void trosi_10_ms() {
    for (int i = 0; i < brojIteracija; i++) {
        asm volatile("" ::: "memory"); 
    }
}



void postaviBrojIteracija(){
    brojIteracija = 1000000;
    bool vrti = true;

    while (vrti) {
        auto t0 = std::chrono::high_resolution_clock::now();
        trosi_10_ms();
        auto t1 = std::chrono::high_resolution_clock::now();

        auto trajanje = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        if (trajanje >= 10) {
            vrti = false; // Vrijeme simulacije je dovoljno dugo
        } else {
            brojIteracija *= 10; // Vrijeme simulacije je prekratko, povećaj broj iteracija
        }
    }

    auto t0 = std::chrono::high_resolution_clock::now();
    trosi_10_ms();
    auto t1 = std::chrono::high_resolution_clock::now();

    auto trajanje = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    brojIteracija = brojIteracija * 10 / trajanje;
}

void simuliraj_x_ms(int x) {
    for (int i = 0; i < x / 10; i++) {
        trosi_10_ms();
    }
}

void upravljackaPetlja(int *id){
    //std::cout << "upravljacka petlja" << std::endl;


    auto pocetnoVrijeme = std::chrono::high_resolution_clock::now();


    int idUlaza = *id;

    if (idUlaza < 1 || idUlaza > zadaci.size()) {
        std::cerr << "Invalid idUlaza: " << idUlaza << std::endl;
        return;
    }

    zadatak zad = zadaci[idUlaza - 1];

    while(simulacijaTraje){
        std::this_thread::sleep_for(std::chrono::milliseconds(zad.period / 10));
        if (statusZadataka[idUlaza - 1] > 0){

            auto vrijemePocetkaObrade = std::chrono::high_resolution_clock::now();

            int vrijemePocetkaMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    vrijemePocetkaObrade - pocetnoVrijeme).count();

            {
                lock_guard<mutex> lock(out);
                outputFile << vrijemePocetkaMs << "\tUpr" << idUlaza << ": započinjem obradu ulaza-" << idUlaza << "\n";
                std::cout << vrijemePocetkaMs << "\tUpr" << idUlaza << ": započinjem obradu ulaza-" << idUlaza << "\n";
            }

            simuliraj_x_ms(zad.trajanjeObrade); 

            {
                lock_guard<mutex> lock(mem);
                stat.brojZavrsenih += 1;
                statusZadataka[idUlaza - 1] = 0;
            }
            


            int vrijemeKrajaMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::high_resolution_clock::now()
                                    - pocetnoVrijeme).count();

            {
                lock_guard<mutex> lock(out);
                outputFile << vrijemeKrajaMs << "\tUpr" << idUlaza << ": gotova obrada ulaza-" << idUlaza << "\n";
                std::cout << vrijemeKrajaMs << "\tUpr" << idUlaza << ": gotova obrada ulaza-" << idUlaza << "\n";
            }


        }

    }
}

void simulatorUlaza(int *id){

    int idUlaza = *id;
    zadatak zad = zadaci[idUlaza - 1];
    zad.idUlaza = idUlaza;

    std::cout << "Period: " << zad.period
                    << ", Trenutak prve pojave: " << zad.trenutakPrvePojave
                    << ", Trajanje obrade: " << zad.trajanjeObrade
                    << ", Broj ulaza: " << zad.brojUlaza << endl;

    auto pocetnoVrijeme = std::chrono::high_resolution_clock::now();
    auto vrijemeAktivacije = pocetnoVrijeme + std::chrono::milliseconds(zad.trenutakPrvePojave);
        
    std::this_thread::sleep_for(std::chrono::milliseconds(zad.trenutakPrvePojave)); //ovo se ne aktivira

    while(false){

        auto vrijeme = std::chrono::high_resolution_clock::now();
        if (vrijeme >= vrijemeAktivacije) {
            
            int vrijemeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    vrijeme - pocetnoVrijeme).count();
            int vrijemeAktivacijeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    vrijemeAktivacije - pocetnoVrijeme).count();
            {
                lock_guard<mutex> lock(out);
                std::cout << "Vrijeme: " << vrijemeMs << " ms since epoch, "
                     << "Vrijeme aktivacije: " << vrijemeAktivacijeMs << " ms since epoch" << endl;
            
            }
            break;
        }
    }

    while (simulacijaTraje) {

        if (statusZadataka[idUlaza - 1] == 0) {
            {
                lock_guard<mutex> lock(mem);
                if(statusZadataka[idUlaza - 1] == 0){
                    statusZadataka[idUlaza - 1] = 1;
                } else {
                    stat.brojNerijesenih += 1;
                }
            }

            int vrijemeAktivacijeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::high_resolution_clock::now()
                                    - pocetnoVrijeme).count();

            {
                lock_guard<mutex> lock(out);
                outputFile << vrijemeAktivacijeMs << "\tUlaz-" << zad.idUlaza << ": promjena ulaza\n";
                std::cout << vrijemeAktivacijeMs << "\tUlaz-" << zad.idUlaza << ": promjena ulaza\n";        
            }

        }

        vrijemeAktivacije += std::chrono::milliseconds(zad.period);
        std::this_thread::sleep_for(std::chrono::milliseconds(zad.period)); // ni ovo se ne aktivira
        while(false){

        auto vrijeme = std::chrono::high_resolution_clock::now();
        if (vrijeme >= vrijemeAktivacije) {
            
            int vrijemeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    vrijeme - pocetnoVrijeme).count();
            int vrijemeAktivacijeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    vrijemeAktivacije - pocetnoVrijeme).count();
{
                lock_guard<mutex> lock(out);
                std::cout << "Vrijeme: " << vrijemeMs << " ms since epoch, "
                     << "Vrijeme aktivacije: " << vrijemeAktivacijeMs << " ms since epoch" << endl;
            
            }
            break;
        }
    }
    }


    }



int main()
{
    postaviBrojIteracija();

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

        while (t){

            size_t substart = line.find("{", index);
            size_t subend = line.find("}", substart);
            index = subend + 1;

            if (substart != string::npos && subend != string::npos) {

                size_t per = line.find(",", substart);
                int period = stoi(line.substr(substart + 1, per - substart - 1));
                size_t tpp = line.find(",", per + 1);
                int trenutakPrvePojave = stoi(line.substr(per + 1, tpp - per - 1));
                size_t to = line.find(",", tpp + 1);
                int trajanjeObrade = stoi(line.substr(tpp + 1, to - tpp - 1));
                size_t bu = line.find(",", to + 1);
                int brojUlaza = stoi(line.substr(to + 1, bu - to - 1));

                zadatak z = {period, trenutakPrvePojave, trajanjeObrade, brojUlaza, 0};
                zadaci.push_back(z);

                std::cout << "Period: " << z.period
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

    vector<pthread_t> threads;

    statusZadataka.resize(zadaci.size(), 0);
    int prioritet = 0;
    int period = 0;


    for (int i = 0; i < zadaci.size(); ++i) {
        if (zadaci[i].period > period) {
            period = zadaci[i].period;
            prioritet += 1;
        }

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

        struct sched_param param;
        param.sched_priority = prioritet;
        pthread_attr_setschedparam(&attr, &param);

        pthread_t thread_ul;
        int* id_ul = new int(i + 1);
        pthread_create(&thread_ul, &attr, (void*(*)(void*))simulatorUlaza, id_ul);
        threads.emplace_back(thread_ul);
        delete id_ul;

        pthread_t thread_up;
        int* id_up = new int(i + 1);
        pthread_create(&thread_up, &attr, (void*(*)(void*))upravljackaPetlja, id_up);
        threads.emplace_back(thread_up);
        delete id_up;


        pthread_attr_destroy(&attr);
    }

    sleep(60 * 2);
    simulacijaTraje = false;

    for (auto& t : threads) {
        pthread_join(t, nullptr);
    }

    outputFile << "Statistika:\n";
    std::cout << "Statistika:\n";
    outputFile << "Broj zadataka: " << stat.brojZavrsenih + stat.brojNerijesenih << "\n";
    std::cout << "Broj zadataka: " << stat.brojZavrsenih + stat.brojNerijesenih << "\n";
    outputFile << "Broj zavrsenih: " << stat.brojZavrsenih << "\n";
    std::cout << "Broj zavrsenih: " << stat.brojZavrsenih << "\n";
    outputFile << "Broj nerijesenih: " << stat.brojNerijesenih << "\n";
    std::cout << "Broj nerijesenih: " << stat.brojNerijesenih << "\n";

    outputFile.close();

    return 0;
}
