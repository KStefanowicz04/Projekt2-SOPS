#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Mutex do zabezpieczania zmiennych ze stanem 
pthread_mutex_t mutex_state = PTHREAD_MUTEX_INITIALIZER;
// Mutex, który blokuje całą czytelnię 
pthread_mutex_t room_empty = PTHREAD_MUTEX_INITIALIZER;

// Globalne liczniki
int reader_count = 0; // Ilu czytelników jest aktualnie w środku
int reader_q = 0;     // Ilu czytelników czeka w kolejce
int writer_q = 0;     // Ilu pisarzy czeka w kolejce
int writer_in = 0;    // Czy pisarz jest w środku (0 - nie, 1 - tak)


// Funkcja pomocnicza do wypisywania stanu na ekranie 
void print_status() {
    printf("ReaderQ: %2d WriterQ: %2d [in: R:%2d W:%2d]\n", reader_q, writer_q, reader_count, writer_in);
    fflush(stdout); // Wymuszenie natychmiastowego wypisania tekstu na ekran
}

// Wątek czytelnik
void* reader(void* arg) {
    while(1) {
        // Czytelnik podchodzi do czytelni i zapisuje się do kolejki
        pthread_mutex_lock(&mutex_state);
        reader_q++;
        print_status();
        pthread_mutex_unlock(&mutex_state);

        // MOMENT WEJŚCIA DO CZYTELNI 
        pthread_mutex_lock(&mutex_state);
        reader_q--;      // Wychodzi z kolejki
        reader_count++;  // wchodzi do środka czytelni
        
        // Jeśli to pierwszy czytelnik, to on blokuje salę dla pisarzy
        if (reader_count == 1) {
            pthread_mutex_lock(&room_empty); 
        }
        print_status();
        pthread_mutex_unlock(&mutex_state);

        // Symulacja czytania 
        usleep(rand() % 500000);

        //MOMENT WYJŚCIA Z CZYTELNI
        pthread_mutex_lock(&mutex_state);
        reader_count--; // Czytelnik wychodzi
        
        // Jeśli to był ostatni czytelnik w środku, otwieramy drzwi dla pisarzy
        if (reader_count == 0) {
            pthread_mutex_unlock(&room_empty); 
        }
        print_status();
        pthread_mutex_unlock(&mutex_state);

        // Życie poza czytelnią 
        usleep(rand() % 500000);
    }
    return NULL;
}

// Wątek-pisarz
void* writer(void* arg) {
    while(1) {
        // Pisarz podchodzi i staje w kolejce
        pthread_mutex_lock(&mutex_state);
        writer_q++;
        print_status();
        pthread_mutex_unlock(&mutex_state);

        // Pisarz próbuje zablokować salę dla siebie 
        // Jeśli są tam czytelnicy, to tutaj utknie i będzie czekał
        pthread_mutex_lock(&room_empty);
        
        // Jak już uda mu się wejść
        pthread_mutex_lock(&mutex_state);
        writer_q--;    // Schodzi z kolejki
        writer_in = 1; // Flaga, że pisarz jest w środku
        print_status();
        pthread_mutex_unlock(&mutex_state);

        // Symulacja pisania 
        usleep(rand() % 800000);

        // Pisarz kończy pracę i wychodzi
        pthread_mutex_lock(&mutex_state);
        writer_in = 0; // Sala wolna
        print_status();
        pthread_mutex_unlock(&mutex_state);

        // Zwalnia mutex sali, żeby inni (czytelnicy lub pisarze) mogli wejść
        pthread_mutex_unlock(&room_empty);

        // Życie poza czytelnią
        usleep(rand() % 800000);
    }
    return NULL;
}


// Main
int main(int argc, char* argv[]) {
    // Sprawdzenie czy użytkownik podał odpowiednią liczbę argumentów 
    if (argc < 3) {
        printf("Użycie: %s <liczba czytelników> <liczba pisarzy>\n", argv[0]);
        return 1;
    }
    
    // Pobranie liczby argumentów z linii poleceń i zamiana na inty
    int R = atoi(argv[1]);
    int W = atoi(argv[2]);

    // Dynamiczna alokacja tablic na identyfikatory wątków
    pthread_t *rt = malloc(R * sizeof(pthread_t));
    pthread_t *wt = malloc(W * sizeof(pthread_t));

    // Inicjalizacja generatora liczb losowych, żeby usleep działał różnie za każdym razem
    srand(time(NULL));

    // Tworzenie wątków dla czytelników
    for(int i=0; i<R; i++) {
        pthread_create(&rt[i], NULL, reader, NULL);
    }
    
    // Tworzenie wątków dla pisarzy
    for(int i=0; i<W; i++) {
        pthread_create(&wt[i], NULL, writer, NULL);
    }

    // Join, czyli czekanie w nieskończoność na zakończenie wątków (program działa w kółko)
    for(int i=0; i<R; i++) pthread_join(rt[i], NULL);
    for(int i=0; i<W; i++) pthread_join(wt[i], NULL);

    free(rt);
    free(wt);
    return 0;
}
