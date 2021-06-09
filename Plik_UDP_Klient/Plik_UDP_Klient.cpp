#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment (lib, "ws2_32.lib")
#include <iphlpapi.h>
#include <conio.h>
#include <windows.h>
#include <string>
#include <fstream>
#include <iomanip>
#include <ctime>

using namespace std;

// BUFOR określający ile bajtów w jednej wiadomości ma zostać wysłanych
#define BUFOR 32768

int main()
{
	WSADATA wsaData; // inicjalizacja żądanej wersji biblioteki WinSock
	// zabezpieczenie w przypadku niepowodzenia inicjalizacji 
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
		_getch();
		exit(EXIT_FAILURE);
	}

	SOCKET soc_out = socket(AF_INET, SOCK_DGRAM, 0); // utworzenie nowego gniazda
	sockaddr_in serwer; 
	int dlugosc_serwera = sizeof(serwer);
	inet_pton(AF_INET, "192.168.1.8", &serwer.sin_addr); // konwersja adresu IP z postaci tekstowej
	serwer.sin_family = AF_INET;
	serwer.sin_port = htons(8080);

	ifstream p;  // Zmienna klasy ifstream obsługująca wejście plikowe
	char * pamiec;  // Bufor przechowujący fragment pliku w formie bitowej przeznaczony do przesłania
	int rozmiar;  // Zmienna z całkowitym rozmiarem bitowym pliku

	// flagi kolejno - odczyt pliku ze strumienia, strumień w formie binarnej, otwarcie tego pliku na końcu
	p.open("wiesiek-3.7z", ios::in | ios::binary | ios::ate);

	// Warunek sprawdzający czy plik jest prawidłowo otwarty
	if (p.is_open())
	{
		// tellg() zwraca pozycję bieżącego znaku w strumieniu wejściowym
		rozmiar = p.tellg();
		// Przypisanie do zmiennej łańczuchowej string liczbę całkowitą określającą rozmiar pliku
		string rozmiar_pliku = to_string(rozmiar);
		// Przypisanie do zmiennej łańczuchowej string liczbę całkowitą określającą rozmiar pakietu
		string rozmiar_pakietu = to_string(BUFOR);

		// Wysłanie wiadomości z wielkością pliku, aby odbiorca mógł odebrać poprawnie plik 
		if (int send = sendto(soc_out, rozmiar_pliku.c_str(), rozmiar_pliku.size() + 1, 0, (sockaddr*)&serwer, dlugosc_serwera) == SOCKET_ERROR)
		{
			printf("Błąd wysyłania widomości\n");
			_getch();
			exit(EXIT_FAILURE);
		}

		// Wysłanie wiadomości z wielkością pakietu
		if (int send = sendto(soc_out, rozmiar_pakietu.c_str(), rozmiar_pakietu.size() + 1, 0, (sockaddr*)&serwer, dlugosc_serwera) == SOCKET_ERROR)
		{
			printf("Błąd wysyłania widomości\n");
			_getch();
			exit(EXIT_FAILURE);
		}

		int ilosc_pakietow = 0;
		int licznik = 0;
		// Rozpoczęcie pomiaru czasu wysyłania plików
		clock_t start = clock();

		while (true)
		{
			// Plik należy podzielić na pakiety.
			// Trezba utworzyć warunek który sprawdza ile pakietów, o określonej wielkości zmieści się w jednym pliku.
			// Poniższy warunek sprawdza czy suma poprzednich pakietów oraz następnego pakietu, całkowicie mieści się w wielkości pliku.
			// Jeśli tak, następny pakiet jest wysyłany w standardwoym rozmiarze, jeśli nie, wysyłana jest reszta danych w pomniejszonym pakiecie.
			// Taki warunek określa na ile pakietów będzie podzielony plik, aby został on prawidłowo wysłany.
			if (rozmiar / (ilosc_pakietow * BUFOR + BUFOR) >= 1)
			{
				// Opóźnienie wysyłania pakietu
				Sleep(40);
				// Zliczanie kolejno wysłanych bajtów oraz ich sumowanie
				licznik += BUFOR;
				printf("Wyslano %d kB na %d kB\n", licznik / 1024, rozmiar / 1024);

				// Określenie wielkości pamięci dla danego pakietu pliku
				pamiec = new char[BUFOR];
				// Przesuwanie wskaźnika odczytu danych z strumienia względem określonego pakietu, flaga ios::beg określa początek strumienia
				p.seekg(BUFOR * ilosc_pakietow, ios::beg);
				// Odczytanie z pliku określonej ilości danych, oraz przypisanie jej do bufora
				p.read(pamiec, BUFOR);
				// Wysłanie wiadomości z buforem z pakietem danych pliku
				if (int send = sendto(soc_out, pamiec, BUFOR, 0, (sockaddr*)&serwer, dlugosc_serwera) == SOCKET_ERROR)
				{
					printf("Blad wysylania pliku\n");
					_getch();
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				// Reszta danych do wysłania w pakiecie mniejszym niż standardowy
				// Od całkowitego rozmiaru pliku, odejmowana jest suma wielkości wszystkich pakietów w romiarze standardowym
				int pozostale = (rozmiar - (BUFOR * ilosc_pakietow));

				// Zliczanie kolejno wysłanych bajtów oraz ich sumowanie
				licznik += pozostale;
				// Wyświetlanie aktualnie wysłanych danych
				printf("Wyslano %d kB na %d kB\n", licznik / 1024, rozmiar / 1024);

				// Określenie wielkości pamięci dla reszty danych pliku
				pamiec = new char[pozostale];
				// Przesuwanie wskaźnika odczyty danych z strumienia względem określonego pakietu, flaga ios::beg określa początek strumienia
				p.seekg(BUFOR * ilosc_pakietow, ios::beg);
				// Odczyt pozostałych danych z pliku oraz przypisanie ich do bufora
				p.read(pamiec, pozostale);
				// Wysłanie wiadomości z buforem z pozostałymi danymi pliku
				if (int send = sendto(soc_out, pamiec, pozostale, 0, (sockaddr*)&serwer, dlugosc_serwera) == SOCKET_ERROR) //Wysłanie pliku
				{
					printf("Blad wysylania pliku\n");
					_getch();
					exit(EXIT_FAILURE);
				}
				printf("+----------------------------------------+");
				printf("\nCalkowity rozmiar pliku: %d kB\n", rozmiar / 1024);
				printf("+----------------------------------------+\n");
				double pomiar_czasu = ((1000 * (clock() - start)) / CLOCKS_PER_SEC);
				cout << endl << "Czas przesylania pliku: " << setprecision(5) << pomiar_czasu / 1000 << " sekundy" << endl;
				_getch();
				break;		
			}
			ilosc_pakietow++;	
		}

		//Warunek sprawdzający czy plik przesłał się w całości
		if (licznik == rozmiar)
		{
			printf("\nPrzeslano caly plik\n");
			_getch();
		}
		else
		{
			printf("\nNieudany przesyl pliku\n");
			_getch();
		}

	}

	closesocket(soc_out);

	WSACleanup();
	_getch();
	return 0;
}