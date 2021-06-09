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

	SOCKET soc_in = socket(AF_INET, SOCK_DGRAM, 0); // utworzenie nowego gniazda
	sockaddr_in ser, klient; // Utworzone zostały dwie struktury przyjmujące wartości odpowiednio dla serwera i klienta
	inet_pton(AF_INET, "192.168.1.8", &ser.sin_addr); // konwersja adresu IP z postaci tekstowej
	ser.sin_family = AF_INET;
	ser.sin_port = htons(8080);

	// Przypisanie adresu i portu serwera do gniazda za pomocą funkcji bind
	// Przyjmuje ona deskryptor gniazda, wskaźnik na strukturę adresu oraz
	// rozmiar tej struktury
	if (bind(soc_in, (SOCKADDR*)&ser, sizeof(ser)) == SOCKET_ERROR)
	{
		printf("Nie można przypisać gniazda\n");
		_getch();
		exit(EXIT_FAILURE);
	}

	int klient_dlugosc = sizeof(klient);
	int rozmiar_pliku, rozmiar_pakietu;  // Zmienna erzyjmująca całkowity rozmiar pliku oraz rozmiar pakietu

	char plik_rozmiar[1024];  // Bufor z rozmiarem pliku jaki został nadesłany
	char pakiet_rozmiar[1024];
	char * pamiec;  // Bufor z fragmentem pliku w formie bitowej odebrany od nadawcy

	ofstream p;

	// Odebranie całkowitego rozmiaru pliku
	if (int recv = recvfrom(soc_in, plik_rozmiar, 1024, 0, (sockaddr*)&klient, &klient_dlugosc) == SOCKET_ERROR)
	{
		printf("Bledna wiadomosc\n");
		_getch();
	}

	// Odebranie całkowitego rozmiaru pakietu
	if (int recv = recvfrom(soc_in, pakiet_rozmiar, 1024, 0, (sockaddr*)&klient, &klient_dlugosc) == SOCKET_ERROR)
	{
		printf("Bledna wiadomosc\n");
		_getch();
	}

	// Zmiana zmiennej łańcuchowej char na zmienną liczby całkowitej, określająca rozmiar pliku
	rozmiar_pliku = stoi(plik_rozmiar);
	// Zmiana zmiennej łańcuchowej char na zmienną liczby całkowitej, określająca rozmiar pakietu
	rozmiar_pakietu = stoi(pakiet_rozmiar);

	// Zmienna zliczająca ilość odebranych pakietów 
	int ilosc_pakietow = 0;
	// Licznik, zliczający aktualną ilość odebranych danych
	int licznik = 0;

	// Rozpoczęcie pomiaru czasu wysyłania plików
	clock_t start = clock();

	while (true)
	{
		// Serwer otrzymuje informacje na jakiej wielkości pakiety został podzieliny plik i na podstawie tego, 
		// określa ilość pakietów jakie mają przyjść oraz kolejno odczytuje je i zapisuje do utworzonego pliku
		if (rozmiar_pliku / (ilosc_pakietow * rozmiar_pakietu + rozmiar_pakietu) >= 1)
		{
			// Zliczanie kolejno odebranych bajtów oraz ich sumowanie
			licznik += rozmiar_pakietu;
			// Wyświetlanie aktualnie odebranych danych
			printf("Odebrano %d kB na %d kB\n", licznik / 1024, rozmiar_pliku / 1024);
			//cout << "Odebrano " << licznik / 1024 << " kB / " << rozmiar_pliku / 1024 << " kB" << endl;

			// Określenie wielkości pamięci dla danego pakietu pliku
			pamiec = new char[rozmiar_pakietu];
			// Odebranie wiadomości z pakietem danych, oraz przypisanie ich do bufora
			if (int recv = recvfrom(soc_in, pamiec, rozmiar_pakietu, 0, (sockaddr*)&klient, &klient_dlugosc) == SOCKET_ERROR) //odebranie pliku
			{
				printf("Blad odbierania pliku\n");
				_getch();
			}
			// Otwarcie pliku do zapisu danych binarnie, oraz do zapisu do już istniejących nowych danych
			p.open("wiesiek-3_odbior.7z", ios::binary | ios_base::app);
			if (p.is_open())
			{
				// Zapis koreślonej ilości danych do pliku, z bufora
				p.write(pamiec, rozmiar_pakietu);
				p.close();
			}
		}
		else
		{
			int pozostale = (rozmiar_pliku - (rozmiar_pakietu * ilosc_pakietow));

			// Zliczanie kolejno odebranych bajtów oraz ich sumowanie
			licznik += pozostale;
			// Wyświetlanie aktualnie odebranych danych
			printf("Odebrano %d kB na %d kB\n", licznik / 1024, rozmiar_pliku / 1024);

			// Określenie wielkości pamięci dla reszty danych pliku
			pamiec = new char[pozostale];
			// Odebranie wiadomości z pakietem danych, oraz przypisanie ich do bufora
			if (int recv = recvfrom(soc_in, pamiec, pozostale, 0, (sockaddr*)&klient, &klient_dlugosc) == SOCKET_ERROR) //odebranie pliku
			{
				printf("Blad odbierania pliku\n");
				_getch();
			}
			p.open("wiesiek-3_odbior.7z", ios::binary | ios_base::app);
			if (p.is_open())
			{
				// Zapis koreślonej ilości danych do pliku, z bufora
				p.write(pamiec, pozostale);
				p.close();
			}
			printf("+----------------------------------------+");
			printf("\nCalkowity rozmiar pliku: %d kB\n", rozmiar_pliku / 1024);
			printf("+----------------------------------------+\n");
			// Wyświetlanie pomiaru czasu odebrania pliku
			double pomiar_czasu = ((1000 * (clock() - start)) / CLOCKS_PER_SEC);
			cout << endl << "Czas odbierania pliku: " << setprecision(5) << pomiar_czasu / 1000 << " sekundy" << endl;
			_getch();
			break;
		}
		ilosc_pakietow++;
	}

	closesocket(soc_in);

	WSACleanup();
	_getch();
	return 0;
}