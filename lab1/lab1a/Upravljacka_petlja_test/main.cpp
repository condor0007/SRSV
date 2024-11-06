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

using namespace std;

struct zadatak{
    int period;
    int trenutakPrvePojave;
    int trajanjeObrade;
    int brojUlaza;
    int idUlaza;
    };

struct statistika {
    bool odradjenoStanje;
    std::chrono::time_point<std::chrono::high_resolution_clock> vrijemePojave;
    std::chrono::time_point<std::chrono::high_resolution_clock> vrijemePocetkaObrade;
    zadatak zad;
};

bool simulacijaTraje = true;

int prekasnoOdredjeni = 0;

vector<statistika> stats;

vector<statistika> redCekanja;

ofstream outputFile("output.txt", ios::app);

mutex mtx;
mutex red;


void ispisiZaglavlje() {
    lock_guard<mutex> lock(mtx);
    outputFile << "t\tdogađaj\n";
    outputFile << "----------------\n";
}

void upravljackaPetlja(){
    //std::cout << "upravljacka petlja" << std::endl;

    auto pocetnoVrijeme = std::chrono::high_resolution_clock::now();

    while(simulacijaTraje){
        if (redCekanja.size() > 0){

            statistika zad;

            {
                lock_guard<mutex> lock(red);
                zad = redCekanja.front();
                redCekanja.erase(redCekanja.begin());
            }

            zad.vrijemePocetkaObrade = std::chrono::high_resolution_clock::now();

            int vrijemePocetkaMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    zad.vrijemePocetkaObrade - pocetnoVrijeme).count();

            {
                lock_guard<mutex> lock(mtx);
                outputFile << vrijemePocetkaMs << "\tUpr: započinjem obradu ulaza-" << zad.zad.idUlaza << "\n";
            }

            std::this_thread::sleep_until(zad.vrijemePocetkaObrade
                                          + std::chrono::milliseconds(zad.zad.trajanjeObrade));

            zad.odradjenoStanje = true;

            stats.push_back(zad);


            int vrijemeKrajaMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::high_resolution_clock::now()
                                    - pocetnoVrijeme).count();

            {
                lock_guard<mutex> lock(mtx);
                outputFile << vrijemeKrajaMs << "\tUpr: gotova obrada ulaza-" << zad.zad.idUlaza << "\n";
            }

//            cout << std::chrono::duration_cast<std::chrono::milliseconds>(
//                    zad.vrijemePocetkaObrade.time_since_epoch()).count() << " - "
//                <<  std::chrono::duration_cast<std::chrono::milliseconds>(
//                    std::chrono::high_resolution_clock::now().time_since_epoch()).count()
//                << " - " << zad.zad.idUlaza << " rjesen" << std::endl;

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

//        cout << "Pocetno vrijeme " << std::chrono::duration_cast<std::chrono::milliseconds>(
//                pocetnoVrijeme.time_since_epoch()).count()
//            << " / Vrijeme aktivacije " << std::chrono::duration_cast<std::chrono::milliseconds>(
//                vrijemeAktivacije.time_since_epoch()).count() << std::endl;

        statistika novaStatistika = {false,
            std::chrono::high_resolution_clock::now(),
            std::chrono::time_point<std::chrono::high_resolution_clock>(),
            zad};

        if (simulacijaTraje){

            {
                lock_guard<mutex> lock(red);

                bool nijeUReduCekajna = true;

                for(const auto& zad : redCekanja) {

                    if (zad.zad.idUlaza == idUlaza) {
                        nijeUReduCekajna = false;
                        prekasnoOdredjeni++;
                    }

                }
                if (nijeUReduCekajna){
                    redCekanja.push_back(novaStatistika);
                }
            }



            int vrijemeAktivacijeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::high_resolution_clock::now()
                                    - pocetnoVrijeme).count();

            {
                lock_guard<mutex> lock(mtx);
                outputFile << vrijemeAktivacijeMs << "\tUlaz-" << zad.idUlaza << ": promjena ulaza\n";
            }

//            cout << std::chrono::duration_cast<std::chrono::milliseconds>(
//                    std::chrono::high_resolution_clock::now().time_since_epoch()).count()
//                << " - " << zad.idUlaza << " postavljen" << std::endl;
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

    //while (std::getline(inputFile, line)) {
    //    std::cout << line << std::endl;
    //}

    //inputFile.close();



    vector<zadatak> zadaci;

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
                int trajanjeObrade = stoi(line.substr(tpp + 1, to - tpp - 1)) * 3.5;
                size_t bu = line.find(",", to + 1);
                int brojUlaza = stoi(line.substr(to + 1, bu - to - 1));

                zadatak z = {period, trenutakPrvePojave, trajanjeObrade, brojUlaza, 0};
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

    vector<thread> threads;
    threads.emplace_back(upravljackaPetlja);

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


    int ukupnoZadataka = stats.size();
    int ukupnoNajkraceCekanje = INT_MAX;
    double ukupnoProsjecnoCekanje = 0;
    int ukupnoVrijemeSvi = 0;

    lock_guard<mutex> lock(mtx);

    for (int i = 1; i <= zadaci.size(); ++i) {

        int brojZadataka = 0;
        int najkraceCekanje = INT_MAX;
        double ukupnoCekanje = 0;
        int ukupnoVrijeme = 0;

        for (const auto& stat : stats) {

            if (stat.zad.idUlaza == i) {

                brojZadataka++;
                int cekanje = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  stat.vrijemePocetkaObrade - stat.vrijemePojave).count();
                ukupnoVrijeme += cekanje;

                if (cekanje < najkraceCekanje) {
                    najkraceCekanje = cekanje;
                }

                ukupnoVrijemeSvi += cekanje;

                if (cekanje < ukupnoNajkraceCekanje) {
                    ukupnoNajkraceCekanje = cekanje;
                }
            }
        }

        if (brojZadataka > 0) {
            ukupnoCekanje = ukupnoVrijeme / (double)brojZadataka;
            outputFile << "Ulaz-" << i << ": broj zadataka = " << brojZadataka
                       << ", najkraće čekanje = " << najkraceCekanje
                       << " ms, prosječno čekanje = " << ukupnoCekanje << " ms\n";
        } else {
            outputFile << "Ulaz-" << i << ": Nema zabilježenih obrada.\n";
        }
    }

    int nerijeseniZadaci = redCekanja.size();
    outputFile << "Ukupan broj zadataka u redu čekanja (neriješenih): " << nerijeseniZadaci << "\n";


    if (ukupnoZadataka > 0) {
        ukupnoProsjecnoCekanje = ukupnoVrijemeSvi / (double)ukupnoZadataka;
        outputFile << "Sveukupno: broj zadataka = " << ukupnoZadataka
                   << ", najkraće čekanje = " << ukupnoNajkraceCekanje
                   << " ms, prosječno čekanje = " << ukupnoProsjecnoCekanje << " ms\n";
    } else {
        outputFile << "Sveukupno: Nema zabilježenih obrada.\n";
    }

    double postotakPrekasnoOdradjenih = prekasnoOdredjeni / (double)ukupnoZadataka * 100;

    outputFile << "Postotak prekasno odrađenih = " << postotakPrekasnoOdradjenih <<  endl;


    outputFile.close();

    return 0;
}
