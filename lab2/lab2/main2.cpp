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
#include <stack>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstring>
#include <setjmp.h>

using namespace std;

#define SHM_KEY 12345

struct zadatak{
    int period;
    int trenutakPrvePojave;
    int trajanjeObrade;
    int brojUlaza;
    int idUlaza;
    int preostaloVrijeme;
    };


struct SharedMemory{

    bool simulacijaTraje;
    int dvaPerioda;
    int brojPrekida;    
    int brojRijesenih;
    int brojNerijesenih;
    mutex mem;
    mutex out;
    zadatak zadaci[38];
    int zad[3][10] ={0};
    int idUlaz;
} *sharedMemory;

// bool simulacijaTraje = true;


int brojProduzetaka = 10;

// int dvaPerioda = 0;
// int brojPrekida = 0;
// vector<int> rijeseni;
// vector<int> nerijeseni;

ofstream outputFile("output.txt", ios::app);

// mutex mtx;
// mutex red;
// mutex prekid_mtx;

// condition_variable prekid_cv;

stack<int> stog;

// vector<int> zadA = {}; // <4
// vector<int> zadB = {}; // >3, < 19
// vector<int> zadC = {}; // > 18
// vector<vector<int>> zad = {zadA, zadB, zadC};
vector<zadatak> zadaci;

vector<int> redosljed = {0, 1, -1, 0, 1, -1, 0, 1, 2, -1};
int t = 0; 
vector<int> ind = {0, 0, 0}; 

int id_zad = -1;
int signalRez = -1;

std::chrono::_V2::system_clock::time_point vrijeme1;
std::chrono::_V2::system_clock::time_point vrijeme2;

sigjmp_buf jumpBuffer;


void initializeSharedMemory() {
    int shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid < 0) {
        cerr << "Error creating shared memory!" << endl;
        exit(1);
    }
    sharedMemory = (SharedMemory *)shmat(shmid, nullptr, 0);
    if (sharedMemory == (void *)-1) {
        cerr << "Error attaching shared memory!" << endl;
        exit(1);
    }
}

void cleanupSharedMemory() {
    shmdt((void *)sharedMemory);
    shmctl(shmget(SHM_KEY, sizeof(SharedMemory), 0666), IPC_RMID, nullptr);
}

int daj_iduci() {
    // cout << "Funkcija daj_iduci" << endl;
    int sljedeci_zadatak = 0;
    int tip = redosljed[t];
    t = (t + 1) % 10;

    // cout << "Funkcija daj_iduci tip " << tip << endl;
    if (tip != -1) {
        int broj_zadataka = (tip == 0 ? 3 : (tip == 1 ? 15 : 20));
        // cout << "Funkcija daj_iduci broj " << broj_zadataka << endl;
        int iteracija = 0;
        if (broj_zadataka > 0 && ind[tip] < broj_zadataka) {
            // cout << "Funkcija daj_iduci if " << endl;
            while (sljedeci_zadatak == 0 && iteracija < broj_zadataka){
                // cout << "Funkcija daj_iduci while 1" << endl;
                sljedeci_zadatak = sharedMemory->zad[tip][ind[tip]];
                // cout << "Funkcija daj_iduci while 2" << endl;
                ind[tip] = (ind[tip] + 1) % broj_zadataka;
                // cout << "Funkcija daj_iduci while 3" << endl;
                iteracija++;
                // cout << "Funkcija daj_iduci while 4" << endl;
                // cout << "Iteracija " << iteracija << endl;
            }
        }
    }
    //cout << "Funkcija daj_iduci, return"<< sljedeci_zadatak - 1 << endl;
    return sljedeci_zadatak - 1;
}


void ispisiZaglavlje() {
    pid_t pid = getpid();
    outputFile << "t\tdogađaj\t"<< pid <<"\n";
    outputFile << "----------------\n";
}

void sat(pid_t pid){
    while(sharedMemory->simulacijaTraje) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //cout << kill(pid, SIGUSR1) << endl;
        if (kill(pid, SIGUSR1) == -1) {
            std::cerr << "Greška pri slanju signala procesu " << pid << std::endl;
            break;
        } 
        // else {  
        //     std::cout << "Periodicni interupt " << pid << std::endl;
        // }
    }
    
}

//treba prije spremanja na stog pogledat za potencijalno produljenje
// upravljacka petlja treba imat loop koji izvlaci id sa stoga pa spava x vremena
// ako se dogodi interupt u snu, preostalo vrijeme treba updateat ova metoda
void upravljackaPetljaSignal(int signum){
    // struct sigaction sa;
    // sa.sa_handler = upravljackaPetljaSignal;
    // sa.sa_flags = 0;
    // sigemptyset(&sa.sa_mask);

    // sigset_t set;
    // sigemptyset(&set);
    // sigaddset(&set, SIGINT);
    // sigprocmask(SIG_BLOCK, &set, NULL);


    // // Deblokiramo signal SIGINT
    // sigprocmask(SIG_UNBLOCK, &set, NULL);

    // signal(SIGUSR1, upravljackaPetljaSignal);   
    // cout << "Stigao signal" << endl;
                
    // signal(SIGINT, SIG_IGN);
    // signal(SIGUSR1, upravljackaPetljaSignal);

    if(signum == SIGUSR1){
        cout << "signal aktivacije" << endl;
        // signalRez = daj_iduci();
        siglongjmp(jumpBuffer, 1);
    }

}

void upravljackaPetlja(){
    //std::cout << "upravljacka petlja" << std::endl;

    signal(SIGUSR1, upravljackaPetljaSignal);

    // struct sigaction sa;
    // sa.sa_handler = upravljackaPetljaSignal;
    // sa.sa_flags = 0;
    // sigemptyset(&sa.sa_mask);
    // if (sigaction(SIGUSR1, &sa, NULL) == -1) {
    //     cerr << "Greška pri postavljanju signalnog handlera" << std::endl;
    //     exit(EXIT_FAILURE);
    // }
    auto pocetnoVrijeme = std::chrono::high_resolution_clock::now();
    vrijeme2 = std::chrono::high_resolution_clock::now();
    vrijeme1 = std::chrono::high_resolution_clock::now();
    
    while(sharedMemory->simulacijaTraje){

        if(sigsetjmp(jumpBuffer, 1) > 0){
            // struct sigaction sa;
            // sa.sa_handler = upravljackaPetljaSignal;
            // sa.sa_flags = 0; // Bez SA_RESETHAND
            // sigaction(SIGINT, &sa, NULL);
            // signal(SIGINT, SIG_IGN);
            // signal(SIGUSR1, upravljackaPetljaSignal);


            int vrijemeAktivacijeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::high_resolution_clock::now()
                                    - pocetnoVrijeme).count();

            if(id_zad != -1){
                // cout << "zad != -1" << endl;
            
                vrijeme2 = std::chrono::high_resolution_clock::now();
            {
                lock_guard<mutex> lock(sharedMemory->mem);
                sharedMemory->zadaci[id_zad].preostaloVrijeme -=  std::chrono::duration_cast<std::chrono::milliseconds>(vrijeme2 - vrijeme1).count(); 
            }
    
                if(brojProduzetaka >= 10){
                    brojProduzetaka = 0;
                    sharedMemory->dvaPerioda++;
                {
                    lock_guard<mutex> lock(sharedMemory->out);
                    cout << vrijemeAktivacijeMs << "\tNastavlja se zadatak "<< id_zad + 1 << endl;
                }
                } else {
                    brojProduzetaka++;
                    int id_novi = daj_iduci();
                    if(id_novi != -1){
                    
                        stog.push(id_zad);
                    {
                        lock_guard<mutex> lock(sharedMemory->out);
                        cout << vrijemeAktivacijeMs << "\tPrekida se zadatak "<< id_zad + 1 << endl;
                    }
                        id_zad = id_novi;
                        sharedMemory->brojPrekida++;
                {
                    lock_guard<mutex> lock(sharedMemory->out);
                    cout << vrijemeAktivacijeMs << "\tPocinje se zadatak "<< id_zad + 1 << endl;
                }
                }
            }

        } else {
            brojProduzetaka++;
            id_zad = daj_iduci();
            if (id_zad!= -1){
                {
                    lock_guard<mutex> lock(sharedMemory->out);
                    cout << vrijemeAktivacijeMs << "\tPocinje se zadatak "<< id_zad + 1 << endl;
                }
            }
        }
        vrijeme1 = std::chrono::high_resolution_clock::now();
        }
        // cout << "linija nakon setjmp" << endl;
        // signal(SIGUSR1, upravljackaPetljaSignal); 

        while(stog.empty() && sharedMemory->simulacijaTraje && id_zad == -1){
            sleep(1);
        }
        if (id_zad != -1){

            int obrada;
            // cout << "Upravljacka petlja, postoji zad" << endl;
            
            while(sharedMemory->simulacijaTraje){
            
                {
                    lock_guard<mutex> lock(sharedMemory->mem);
                    obrada = sharedMemory->zadaci[id_zad].preostaloVrijeme;
                }  
            
                auto vrijemeAktivacije = vrijeme1 + std::chrono::milliseconds(obrada);
                std::this_thread::sleep_until(vrijemeAktivacije);


                int vrijemeAktivacijeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::high_resolution_clock::now()
                                    - pocetnoVrijeme).count();
                {
                    lock_guard<mutex> lock(sharedMemory->out);
                    cout << vrijemeAktivacijeMs << "\tOdradjen zadatak "<< id_zad + 1 << endl;
                }
            
                {
                    lock_guard<mutex> lock(sharedMemory->mem);
                    if(sharedMemory->zadaci[id_zad].preostaloVrijeme == obrada){
                        sharedMemory->zadaci[id_zad].preostaloVrijeme = 0;
                        sharedMemory->brojRijesenih++;
                        int x = (id_zad <= 3 ? 1 : (id_zad <= 18 ? 2 : 3));
                        if(x==1){
                            sharedMemory->zad[0][id_zad - 1] = 0;
                        } else if(x==2){
                            sharedMemory->zad[1][id_zad - 3 - 1] = 0;
                        } else {
                            sharedMemory->zad[2][id_zad - 18 - 1] = 0;
                        }
                        id_zad = -1;
                        
                    }
                }
                if (!stog.empty()){
                    id_zad = stog.top();
                    stog.pop();
                {
                    lock_guard<mutex> lock(sharedMemory->out);
                    cout << vrijemeAktivacijeMs << "\tZadatak uzet sa stoga "<< id_zad + 1 << endl;
                }
                } else {
                    break;
                }
            }
        }

        // cout << "Upravljacka petlja, gotova iteracija" << endl;

    }
}

void simulatorUlaza(zadatak zad, int idUlaza){

    zad.idUlaza = idUlaza;
    int x = (idUlaza <= 3 ? 1 : (idUlaza <= 18 ? 2 : 3));

    auto pocetnoVrijeme = std::chrono::high_resolution_clock::now();
    auto vrijemeAktivacije = pocetnoVrijeme + std::chrono::milliseconds(zad.trenutakPrvePojave);

    //std::cout << "simulacijska petlja" << std::endl;

    while (sharedMemory->simulacijaTraje) {

        std::this_thread::sleep_until(vrijemeAktivacije);


        if (sharedMemory->simulacijaTraje){
            {

                lock_guard<mutex> lock(sharedMemory->mem); 
                if(idUlaza < 4){
                    if(sharedMemory->zad[0][idUlaza - 1]){
                        sharedMemory->brojNerijesenih++;
                    }
                        sharedMemory->zad[0][idUlaza - 1] = idUlaza;
                        sharedMemory->zadaci[idUlaza - 1].preostaloVrijeme = zad.trajanjeObrade;

                    
                } else if (idUlaza < 19){
                    if(sharedMemory->zad[1][idUlaza - 3 - 1]){
                        sharedMemory->brojNerijesenih++;
                    }
                        sharedMemory->zad[1][idUlaza - 3 - 1] = idUlaza;
                        sharedMemory->zadaci[idUlaza - 1].preostaloVrijeme = zad.trajanjeObrade;

                    
                } else {
                    if(sharedMemory->zad[2][idUlaza - 18 - 1]){
                        sharedMemory->brojNerijesenih++;
                    }
                        sharedMemory->zad[2][idUlaza - 18 - 1] = idUlaza;
                        sharedMemory->zadaci[idUlaza - 1].preostaloVrijeme = zad.trajanjeObrade;

                    }
                }
            

            int vrijemeAktivacijeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::high_resolution_clock::now()
                                    - pocetnoVrijeme).count();

            cout << vrijemeAktivacijeMs << "\tUlaz-" << idUlaza << ": promjena ulaza\n";
            

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

                zadatak z = {period, trenutakPrvePojave, trajanjeObrade, brojUlaza, id, 0};
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

    // ispisiZaglavlje();
    //return 0;
    // vector<thread> threads;
    // threads.emplace_back(upravljackaPetlja);
    // threads.emplace_back(sat);
    // cout << "ispis zaglavlja proso" << endl;
    initializeSharedMemory();
    sharedMemory->simulacijaTraje=true;
    sharedMemory->brojNerijesenih=0;
    sharedMemory->brojPrekida=0;
    sharedMemory->brojRijesenih=0;
    sharedMemory->dvaPerioda=0;

    for (int i = 0; i < zadaci.size(); ++i) {
        sharedMemory->zadaci[i] = zadaci[i];
    }    
    // cout << "shared memory definiran" << endl;

    vector<pid_t> childPIDs;

    pid_t pid = fork();
    if (pid == 0) {
        upravljackaPetlja();
        exit(0); 
    } else if (pid > 0) {
        childPIDs.push_back(pid);
        for(int i = 0; i < zadaci.size(); ++i){
            pid_t pid = fork();
            if(pid == 0){
                childPIDs.push_back(pid);
                simulatorUlaza(zadaci[i], i + 1);
                exit(0);
            }
        }
    } else {
        cerr << "Greška prilikom kreiranja procesa (upravljackaPetlja)!" << endl;
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        sat(childPIDs[0]);
        exit(0); 
    } else {
        childPIDs.push_back(pid2);
    }

    std::cout << "Pritisnite Enter za zaustavljanje simulacije..." << std::endl;
    std::cin.get();
    sharedMemory->simulacijaTraje = false;

    for (auto &ulazPid : childPIDs) {
        waitpid(ulazPid, nullptr, 0);
    }


    for (auto &ulazPid : childPIDs) {
        kill(ulazPid, SIGKILL);
    }




    outputFile  << "Statistika: Broj rješenih" << sharedMemory->brojRijesenih << endl;
    outputFile  << "Statistika: Broj nerješenih" << sharedMemory->brojNerijesenih << endl;
    outputFile  << "Statistika: Broj produzetaka" << sharedMemory->dvaPerioda << endl;
    outputFile  << "Statistika: Broj prekida" << sharedMemory->brojPrekida << endl;

    outputFile.close();
    
    cleanupSharedMemory();
    return 0;
}
