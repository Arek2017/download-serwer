#define _CRT_SECURE_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main()
{
	WSADATA wsaData; // inicjalizacja żądanej wersja biblioteki WinSock

	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("Error StartUP! : %d\n", iResult);
		return 1;
	}

	struct sockaddr_in myAddr;			//deklaracja struktur sockaddr_in
	myAddr.sin_family = AF_INET;		//ustawienie sin_family dla adresów w wersji 4

	int addr = inet_pton(AF_INET, "127.0.0.1", &(myAddr.sin_addr));		//przypisanie adresu do struktury sockaddr_in myAddr
	if (addr == -1) {
		cout << "Error inet_pton! ";
		WSACleanup();
		return 1;

	}
	myAddr.sin_port = htons(8080);							//ustawienie portu na 8080
	memset(myAddr.sin_zero, '\0', sizeof(myAddr.sin_zero)); //wyczyszczenie pamięci przed wypełnieniem struktury


	int sock1 = socket(AF_INET, SOCK_STREAM, 0);			// inicjalizacja socketu
	if (sock1 < 0) {
		cout << "Bad socket!: " << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	int bind1 = bind(sock1, (struct sockaddr*)&myAddr, sizeof(myAddr));		// inicjalizacja bind
	if (bind1 < 0) {
		cout << "Bad bind!: " << WSAGetLastError() << endl;
		closesocket(sock1);
		WSACleanup();
		return 1;
	}

	int listen1 = listen(sock1, 2);											// inicjalizacja listen
	if (listen1 < 0) {
		cout << "Bad listen: " << WSAGetLastError() << endl;
		closesocket(sock1);
		WSACleanup();
		return 1;
	}

	const int bufferSize = 256;
	char buf[bufferSize];

	vector<WSAPOLLFD> vecSockets;
	WSAPOLLFD socket;

	socket.fd = sock1;
	socket.events = POLLRDNORM;

	vecSockets.push_back(socket);

	map<int, FILE*> mapVecsock;

	int counterClients = 0;
	while (true) {
		int Poll = WSAPoll(&vecSockets.front(), vecSockets.size(), -1);
		if (Poll == SOCKET_ERROR) {
			cout << "Error WSAPoll()!: " << WSAGetLastError() << endl;
			for (const auto& v : mapVecsock) {
				fclose(v.second);
			}
			mapVecsock.clear();

			for (pollfd v : vecSockets) {
				closesocket(v.fd);
			}
			vecSockets.clear();
			closesocket(sock1);
			WSACleanup();
			return 1;
		}

		WSAPOLLFD newSocket;


		for (int i = 0; i < vecSockets.size(); i++) {

			int check = vecSockets[i].revents & (POLLIN | POLLHUP);

			if (check == FALSE) {}
			else if (vecSockets[i].fd == sock1) {

				struct sockaddr_in newAddr;
				socklen_t addr_size = sizeof(newAddr);

				int newClient = accept(sock1, (struct sockaddr*)&newAddr, &addr_size);
				if (newClient == -1) {
					cout << "Error accept!: " << WSAGetLastError();
					continue;
				}

				char ip4[INET_ADDRSTRLEN];

				if (!inet_ntop(AF_INET, &(newAddr.sin_addr), ip4, sizeof(ip4))) {
					cout << "Error inet_ntop! " << endl;
					closesocket(newClient);
					continue;
				}
				string ipAddres(ip4);														// stworzenie zmiennych tworzacych nazwe pliku
				string fileName = ipAddres + "-Nr" + to_string(counterClients) + ".txt";		// tekstowego z adresem ip i numerem klienta

				cout << "Correct Accept. Client address: " << ipAddres << " , Client number: " << newClient << endl << endl;

				FILE* file = fopen(fileName.c_str(), "wb"); //utworzenie nowego pliku
				if (!file) {
					cout << "Error file! " << endl;
					closesocket(newClient);
					continue;
				}

				counterClients++;					// inkrementacja licznika klientów

				newSocket.events = POLLRDNORM;		// przypisanie flagi
				newSocket.revents = 0;
				newSocket.fd = newClient;			// do nowego socketu	

				vecSockets.push_back(newSocket);	// zapisanie socketu do vectora	

				mapVecsock.insert({ newClient, file });		//przypisanie pliku do gniazda klienta
			}
			else if (vecSockets[i].revents & POLLRDNORM) { //Jeżeli klient ma flagę do odczytu
				int rcv = recv(vecSockets[i].fd, buf, bufferSize, 0);		//odebranie wiadomości
				if (rcv > 0) {

					if (fwrite(buf, sizeof(char), rcv, mapVecsock[vecSockets[i].fd])) {	//zapis danych do pliku
						cout << "Received: " << rcv << " B, from client " << vecSockets[i].fd << endl;
					}
					else {
						cout << "Error fwrite! " << endl;
						fclose(mapVecsock[vecSockets[i].fd]);
						mapVecsock.erase(vecSockets[i].fd);
						closesocket(vecSockets[i].fd);
						vecSockets.erase(vecSockets.begin() + i);
						i--;
						continue;
					}

				}
				else if (rcv == 0) {									// gdy klient wyśle 0B
					fclose(mapVecsock[vecSockets[i].fd]);				// to zamknij plik
					mapVecsock.erase(vecSockets[i].fd);				// wyczysc slad po kliencie

					cout << endl << "Client disconnected!: " << vecSockets[i].fd << endl << endl;

					closesocket(vecSockets[i].fd);						// zamknij socket 
					vecSockets.erase(vecSockets.begin() + i);			// wymaż klienta
					i--;
					continue;
				}
				else if (rcv == -1) {
					cout << "Error recv!: " << WSAGetLastError() << endl;
					fclose(mapVecsock[vecSockets[i].fd]);
					mapVecsock.erase(vecSockets[i].fd);
					closesocket(vecSockets[i].fd);
					vecSockets.erase(vecSockets.begin() + i);
					i--;
					continue;
				}
			}
			else if (vecSockets[i].revents & (POLLHUP | POLLERR)) { //Jeżeli klient się rozłączył
				fclose(mapVecsock[vecSockets[i].fd]);				// to zamknij plik
				mapVecsock.erase(vecSockets[i].fd);				// wyczysc slad po kliencie

				cout << endl << "Client disconnected!: " << vecSockets[i].fd << endl << endl;

				closesocket(vecSockets[i].fd);						// zamknij socket 
				vecSockets.erase(vecSockets.begin() + i);			// wymaż klienta
				i--;
				continue;
			}
		}
	}

	closesocket(sock1);
	WSACleanup();
	return 1;
}