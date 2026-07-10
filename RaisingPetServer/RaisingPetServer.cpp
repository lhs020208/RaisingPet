#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mysql.h>

#include <array>
#include <iostream>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

namespace {
constexpr unsigned short kDefaultPort = 7777;
constexpr int kListenBacklog = SOMAXCONN;

class WinsockSession {
public:
	WinsockSession() : initialized_(WSAStartup(MAKEWORD(2, 2), &data_) == 0) {}
	~WinsockSession() {
		if (initialized_) {
			WSACleanup();
		}
	}

	bool IsInitialized() const { return initialized_; }

private:
	WSADATA data_{};
	bool initialized_ = false;
};

void HandleClient(SOCKET clientSocket, sockaddr_in clientAddress) {
	char addressBuffer[INET_ADDRSTRLEN]{};
	inet_ntop(AF_INET, &clientAddress.sin_addr, addressBuffer, sizeof(addressBuffer));
	const unsigned short clientPort = ntohs(clientAddress.sin_port);

	std::cout << "[Connected] " << addressBuffer << ':' << clientPort << '\n';

	std::array<char, 4096> buffer{};
	while (true) {
		const int received = recv(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0);
		if (received <= 0) {
			break;
		}

		int sentBytes = 0;
		while (sentBytes < received) {
			const int sent = send(clientSocket, buffer.data() + sentBytes, received - sentBytes, 0);
			if (sent == SOCKET_ERROR) {
				sentBytes = -1;
				break;
			}
			sentBytes += sent;
		}

		if (sentBytes < 0) {
			break;
		}
	}

	shutdown(clientSocket, SD_BOTH);
	closesocket(clientSocket);
	std::cout << "[Disconnected] " << addressBuffer << ':' << clientPort << '\n';
}

bool TryParsePort(const char* text, unsigned short& port) {
	try {
		const int value = std::stoi(text);
		if (value < 1 || value > 65535) {
			return false;
		}
		port = static_cast<unsigned short>(value);
		return true;
	}
	catch (...) {
		return false;
	}
}
} // namespace

bool TestDatabaseConnection()
{
	MYSQL* connection = mysql_init(nullptr);
	if (!connection)
	{
		std::cerr << "mysql_init failed\n";
		return false;
	}

	MYSQL* result = mysql_real_connect(
		connection,
		"127.0.0.1",
		"raisingpet_server",
		"12345678",
		"RaisingPet",
		3306,
		nullptr,
		0
	);

	if (!result)
	{
		std::cerr << "MySQL connection failed: "
			<< mysql_error(connection) << '\n';
		mysql_close(connection);
		return false;
	}

	std::cout << "MySQL connected successfully.\n";

	if (mysql_query(connection, "SELECT VERSION()") != 0)
	{
		std::cerr << "SELECT VERSION() failed: "
			<< mysql_error(connection) << '\n';
		mysql_close(connection);
		return false;
	}

	MYSQL_RES* queryResult = mysql_store_result(connection);
	if (queryResult)
	{
		MYSQL_ROW row = mysql_fetch_row(queryResult);
		if (row && row[0])
		{
			std::cout << "MySQL version: " << row[0] << '\n';
		}
		mysql_free_result(queryResult);
	}

	mysql_close(connection);
	return true;
}

int main(int argc, char* argv[]) {
	unsigned short port = kDefaultPort;
	if (argc > 2 || (argc == 2 && !TryParsePort(argv[1], port))) {
		std::cerr << "Usage: RaisingPetServer.exe [port]\n";
		return 1;
	}

	if (!TestDatabaseConnection())
	{
		std::cerr << "Database connection test failed.\n";
		return 1;
	}

	WinsockSession winsock;
	if (!winsock.IsInitialized()) {
		std::cerr << "WSAStartup failed: " << WSAGetLastError() << '\n';
		return 1;
	}

	const SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		std::cerr << "socket failed: " << WSAGetLastError() << '\n';
		return 1;
	}

	sockaddr_in serverAddress{};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);

	if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
		std::cerr << "bind failed: " << WSAGetLastError() << '\n';
		closesocket(listenSocket);
		return 1;
	}

	if (listen(listenSocket, kListenBacklog) == SOCKET_ERROR) {
		std::cerr << "listen failed: " << WSAGetLastError() << '\n';
		closesocket(listenSocket);
		return 1;
	}

	std::cout << "RaisingPet TCP server is listening on 0.0.0.0:" << port << '\n';
	std::cout << "Press Ctrl+C to stop.\n";

	while (true) {
		sockaddr_in clientAddress{};
		int clientAddressLength = sizeof(clientAddress);
		const SOCKET clientSocket = accept(
			listenSocket,
			reinterpret_cast<sockaddr*>(&clientAddress),
			&clientAddressLength);

		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "accept failed: " << WSAGetLastError() << '\n';
			continue;
		}

		std::thread(HandleClient, clientSocket, clientAddress).detach();
	}
}
