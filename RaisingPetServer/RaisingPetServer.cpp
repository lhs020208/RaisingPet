#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <mysql.h>
#include <errmsg.h>
#include <mysqld_error.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

namespace {
#include "RaisingPetServerInternal.h"
#include "RaisingPetServerDatabase.cpp"
#include "RaisingPetServerPackets.cpp"
#include "RaisingPetServerAdmin.cpp"
#include "RaisingPetServerClient.cpp"

} // namespace

int main(int argc, char* argv[]) {
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	unsigned short port = kDefaultPort;
	if (argc > 2 || (argc == 2 && !TryParsePort(argv[1], port))) {
		std::cerr << "Usage: RaisingPetServer.exe [port]\n";
		return 1;
	}
	if (port == 65535) {
		std::cerr << "Port 65535 cannot be used because admin port uses [port + 1].\n";
		return 1;
	}
	const unsigned short adminPort = static_cast<unsigned short>(port + 1);

	WinsockSession winsock;
	if (!winsock.IsInitialized()) {
		std::cerr << "WSAStartup failed: " << WSAGetLastError() << '\n';
		return 1;
	}

	Database database;
	if (!database.Connect()) {
		std::cerr << "Database connection failed.\n";
		return 1;
	}

	const SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		std::cerr << "socket failed: " << WSAGetLastError() << '\n';
		return 1;
	}
	if (!SetNonBlocking(listenSocket)) {
		std::cerr << "ioctlsocket failed: " << WSAGetLastError() << '\n';
		closesocket(listenSocket);
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

	std::vector<ClientSession> sessions;
	std::mutex sessionsMutex;
	std::array<char, 4096> buffer{};
	auto lastFinancialCheckTime = std::chrono::steady_clock::now();

	std::cout << "RaisingPet TCP server is listening on 0.0.0.0:" << port << '\n';
	std::thread(RunAdminServer, adminPort, std::ref(sessions), std::ref(sessionsMutex)).detach();
	std::thread(RunStockPriceUpdateServer,
		std::ref(sessions), std::ref(sessionsMutex)).detach();
	std::cout << "Press Ctrl+C to stop.\n";

	while (true) {
		fd_set readSet;
		fd_set writeSet;
		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		FD_SET(listenSocket, &readSet);

		{
			std::lock_guard<std::mutex> sessionsLock(sessionsMutex);
			for (const ClientSession& session : sessions) {
				if (session.socket == INVALID_SOCKET) continue;
				FD_SET(session.socket, &readSet);
				if (!session.sendBuffer.empty()) FD_SET(session.socket, &writeSet);
			}
		}

		timeval timeout{};
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		const int selected = select(0, &readSet, &writeSet, nullptr, &timeout);
		if (selected == SOCKET_ERROR) {
			std::cerr << "select failed: " << WSAGetLastError() << '\n';
			continue;
		}

		if (FD_ISSET(listenSocket, &readSet)) {
			while (true) {
				sockaddr_in clientAddress{};
				int clientAddressLength = sizeof(clientAddress);
				const SOCKET clientSocket = accept(listenSocket,
					reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressLength);
				if (clientSocket == INVALID_SOCKET) {
					const int error = WSAGetLastError();
					if (error != WSAEWOULDBLOCK) std::cerr << "accept failed: " << error << '\n';
					break;
				}

				SetNonBlocking(clientSocket);
				char addressBuffer[INET_ADDRSTRLEN]{};
				inet_ntop(AF_INET, &clientAddress.sin_addr, addressBuffer, sizeof(addressBuffer));
				const unsigned short clientPort = ntohs(clientAddress.sin_port);

				ClientSession session;
				session.socket = clientSocket;
				session.addressText = std::string(addressBuffer) + ':' + std::to_string(clientPort);
				std::cout << "[Connected] " << session.addressText << '\n';
				std::lock_guard<std::mutex> sessionsLock(sessionsMutex);
				if (sessions.size() >= 30) {
					closesocket(clientSocket);
					continue;
				}
				sessions.push_back(std::move(session));
			}
		}

		{
			std::lock_guard<std::mutex> sessionsLock(sessionsMutex);
			const auto now = std::chrono::steady_clock::now();
			const bool shouldCheckFinancialProducts =
				(now - lastFinancialCheckTime) >= std::chrono::seconds(1);
			if (shouldCheckFinancialProducts) lastFinancialCheckTime = now;

			for (ClientSession& session : sessions) {
				if (session.socket == INVALID_SOCKET) continue;

				if (shouldCheckFinancialProducts && session.authenticated) {
					std::vector<FinancialMoneyChange> changes;
					if (database.ProcessDueFinancialProducts(session.playerId, changes))
						QueueFinancialChanges(session, changes);
				}

				if (FD_ISSET(session.socket, &readSet)) {
					while (true) {
						const int received = recv(session.socket, buffer.data(),
							static_cast<int>(buffer.size()), 0);
						if (received > 0) {
							session.receiveBuffer.insert(session.receiveBuffer.end(),
								buffer.data(), buffer.data() + received);
							if (!ProcessReceiveBuffer(database, session)) {
								CloseSession(database, session);
								break;
							}
						}
						else if (received == 0) {
							CloseSession(database, session);
							break;
						}
						else {
							const int error = WSAGetLastError();
							if (error != WSAEWOULDBLOCK) CloseSession(database, session);
							break;
						}
					}
				}

				if (session.socket != INVALID_SOCKET && FD_ISSET(session.socket, &writeSet) &&
					!session.sendBuffer.empty()) {
					const int sent = send(session.socket, session.sendBuffer.data(),
						static_cast<int>(session.sendBuffer.size()), 0);
					if (sent > 0) {
						session.sendBuffer.erase(session.sendBuffer.begin(),
							session.sendBuffer.begin() + sent);
					}
					else if (sent == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
						CloseSession(database, session);
					}
				}
			}

			sessions.erase(std::remove_if(sessions.begin(), sessions.end(),
				[](const ClientSession& session) { return session.socket == INVALID_SOCKET; }),
				sessions.end());
		}
	}
}
