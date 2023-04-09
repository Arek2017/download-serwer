#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Funkcja weryfikująca, czy wszystkie pakiety zostały wysłane
// Funkcja została zapożyczona ze strony: https://github.com/Kuszki/PWSS-TPK-Example/blob/main/sockbase.cpp
bool send_all(int sock, const char* data, size_t size)
{
	
	while (size > 0) // Gdy są jeszcze dane do wysłania
	{		
		const int sc = send(sock, data, size, 0); // Wyślij brakujące dane
		
		if (sc <= 0) return false; // W przypadku błędu przerwij działanie
		else
		{
			data += sc; // Przesuń wskaźnik na dane
			size -= sc; // Zmniejsz liczbę pozostałych danych
		}
	}
	return true;
}

int main(void) {

	WSADATA wsaData; // inicjalizacja żądanej wersja biblioteki WinSock
	WORD ver = MAKEWORD(2, 2);   //Dzięki temu program działa w systemie Windows
	int iResultWin = WSAStartup(ver, &wsaData);
	if (iResultWin != 0) { // walidacja
		printf("WSAStartup failed: %d\n", iResultWin);
		return 1;
	}

	SOCKET conSocket = socket(AF_INET, SOCK_STREAM, 0); // socket, potrzebny do                       //połączenia z klientem
	if (conSocket == INVALID_SOCKET) { // walidacja socketu
		cout << "Blad przy sokecie: " << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	struct sockaddr_in clientService; // deklaracja struktury
	memset(clientService.sin_zero, '\0', sizeof(clientService.sin_zero)); // ustawianie pamięci
	clientService.sin_family = AF_INET;

	inet_pton(AF_INET, "127.0.0.1", &(clientService.sin_addr));

	clientService.sin_port = htons(8080); // port 


	if (connect(conSocket, (SOCKADDR*)&clientService, sizeof(clientService)) < 0) {
		cout << "Blad przy connect: " << WSAGetLastError() << endl;
		WSACleanup();
		return 1;

	} // funkcja, która łączy serwer z klientem
	else {
		cout << "Polaczenie z serwerem pomyslne" << endl;
	}
	FILE* file;
	const char* Plik = "sciezka.exe"; // plik, z którego wczytywane są dane do bufora
	char bufer[sizeof(Plik)];

	errno_t err;
	if (err = fopen_s(&file, Plik, "rb") != 0) // walidacja otwarcia pliku do odczytu
	{
		cout << "Blad przy otwarciu pliku" << endl;
		strcpy_s(bufer, sizeof(Plik), "NO FILE");
	}

	size_t reading_size;
	int const bufer_size = 256;// wielkosc bufora do odczytu
	char buf[bufer_size];// tablica do odczytu

	const clock_t begin_time = clock(); // start czasu
	int part = 0;		// zmienne pomocnicze do wyliczenia
	int wielkosc = 0;	// wielkosci pakietu, ramki
	// oraz ilości przebiegów
	int iResult;
	while (!feof(file)) {
		do {
			if ((reading_size = fread(buf, 1, bufer_size, file)) < 0) {
				perror("Blad przy odczycie danych");
				WSACleanup();
				return 1;
			}
			if ((iResult = send_all(conSocket, buf, reading_size)) < 0) {
				perror("Blad przy wysylaniu danych");
				std::cout << WSAGetLastError() << std::endl;
				WSACleanup();
				return 1;
			}
			part++;
		} while (iResult != reading_size);
		wielkosc += iResult;
	}
	cout << "Wyslano " << wielkosc << " bajtow po " << bufer_size << " bytes. Zostalo wyslane w " << part << " ramkach " << endl;
	cout << "Czas wyslania: " << double(clock() - begin_time) / CLOCKS_PER_SEC << " s." << endl;

	closesocket(conSocket);
	WSACleanup();
}
