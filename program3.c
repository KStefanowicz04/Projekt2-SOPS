#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>


// Jeden mutex chroni cały stan monitora
pthread_mutex_t mutex_state = PTHREAD_MUTEX_INITIALIZER;
// Zmienna warunkowa - wątki czekają na niej na swoją kolej
pthread_cond_t turn = PTHREAD_COND_INITIALIZER;

// Globalne liczniki
int reader_count = 0; // Ilu czytelników jest aktualnie w środku
int reader_q = 0;     // Ilu czytelników czeka w kolejce
int writer_q = 0;     // Ilu pisarzy czeka w kolejce
int writer_in = 0;    // Czy pisarz jest w środku (0 - nie, 1 - tak)

// Tickety zapewniające sprawiedliwą kolejność 
unsigned long ticket_next = 0;  // Następny numerek do wydania
unsigned long ticket_serve = 0; // Numerek aktualnie obsługiwany (czoło kolejki)


// Funkcja pomocnicza do wypisywania stanu na ekranie
void print_status() {
    printf("ReaderQ: %2d WriterQ: %2d [in: R:%2d W:%2d]\n", reader_q, writer_q, reader_count, writer_in);
    fflush(stdout); // Wymuszenie natychmiastowego wypisania tekstu na ekran
}

// Wątek czytelnik
void* reader(void* arg) {
    while(1) {
        // Czytelnik podchodzi do czytelni, zapisuje się do kolejki i pobiera numerek
        pthread_mutex_lock(&mutex_state);
        reader_q++;
        unsigned long my_ticket = ticket_next++;
        print_status();

        // Czeka na swoją kolej  oraz na brak pisarza w środku
        while (my_ticket != ticket_serve || writer_in) {
            pthread_cond_wait(&turn, &mutex_state);
        }

        // MOMENT WEJŚCIA DO CZYTELNI
        reader_q--;       // Wychodzi z kolejki
        reader_count++;   // Wchodzi do środka
        ticket_serve++;   // Przepuszcza dalej - następny numerek staje się czołem
        print_status();
        // Budzi następnego: czytelnik wejdzie współbieżnie, pisarz poczeka aż wyjdziemy
        pthread_cond_broadcast(&turn);
        pthread_mutex_unlock(&mutex_state);

        // Symulacja czytania
        usleep(rand() % 500000);

        // MOMENT WYJŚCIA Z CZYTELNI
        pthread_mutex_lock(&mutex_state);
        reader_count--; // Czytelnik wychodzi
        // Jeśli to był ostatni czytelnik, budzimy czekających (np. pisarza na czele)
        if (reader_count == 0) {
            pthread_cond_broadcast(&turn);
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
        // Pisarz podchodzi, staje w kolejce i pobiera numerek
        pthread_mutex_lock(&mutex_state);
        writer_q++;
        unsigned long my_ticket = ticket_next++;
        print_status();

        // Czeka na swoją kolej  oraz na zupełnie pustą czytelnię
        while (my_ticket != ticket_serve || writer_in || reader_count > 0) {
            pthread_cond_wait(&turn, &mutex_state);
        }

        // MOMENT WEJŚCIA DO CZYTELNI
        writer_q--;     // Schodzi z kolejki
        writer_in = 1;  // Pisarz jest w środku
        ticket_serve++; // Przepuszcza dalej czoło kolejki (ale i tak nikt nie wejdzie, póki pisze)
        print_status();
        pthread_mutex_unlock(&mutex_state);

        // Symulacja pisania
        usleep(rand() % 800000);

        // MOMENT WYJŚCIA Z CZYTELNI
        pthread_mutex_lock(&mutex_state);
        writer_in = 0; // Sala wolna
        print_status();
        // Budzimy czekających - kolejny numerek (czytelnicy lub pisarz) może wejść
        pthread_cond_broadcast(&turn);
        pthread_mutex_unlock(&mutex_state);

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
