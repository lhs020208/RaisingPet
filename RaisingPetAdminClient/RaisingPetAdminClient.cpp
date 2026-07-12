#include <WinSock2.h>
#include <WS2tcpip.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

namespace {
constexpr const char* kDefaultServerIp = "127.0.0.1";
constexpr unsigned short kDefaultAdminPort = 7778;
constexpr const char* kAdminId = "admin";
constexpr const char* kAdminPassword = "12345678";

enum class PacketType : std::uint16_t {
	AdminLoginRequest = 100,
	AdminLoginResponse = 101,
};

enum class AuthResult : std::uint8_t {
	Success = 0,
	InvalidFormat = 1,
	AlreadyExists = 2,
	NotFound = 3,
	WrongPassword = 4,
	DatabaseError = 5,
	ServerError = 6,
};

class WinsockSession {
public:
	WinsockSession() : initialized_(WSAStartup(MAKEWORD(2, 2), &data_) == 0) {}
	~WinsockSession() {
		if (initialized_) WSACleanup();
	}

	bool IsInitialized() const { return initialized_; }

private:
	WSADATA data_{};
	bool initialized_ = false;
};

bool TryParsePort(const char* text, unsigned short& port) {
	try {
		const int value = std::stoi(text);
		if (value < 1 || value > 65535) return false;
		port = static_cast<unsigned short>(value);
		return true;
	}
	catch (...) {
		return false;
	}
}

void WriteUInt16(std::vector<char>& buffer, std::uint16_t value) {
	const char* bytes = reinterpret_cast<const char*>(&value);
	buffer.insert(buffer.end(), bytes, bytes + sizeof(value));
}

std::uint16_t ReadUInt16(const char* data) {
	std::uint16_t value = 0;
	std::memcpy(&value, data, sizeof(value));
	return value;
}

std::vector<char> MakeAdminLoginPacket() {
	const std::string id = kAdminId;
	const std::string password = kAdminPassword;
	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(
		4 + 4 + id.size() + password.size());
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(PacketType::AdminLoginRequest));
	WriteUInt16(packet, static_cast<std::uint16_t>(id.size()));
	WriteUInt16(packet, static_cast<std::uint16_t>(password.size()));
	packet.insert(packet.end(), id.begin(), id.end());
	packet.insert(packet.end(), password.begin(), password.end());
	return packet;
}

bool SendAll(SOCKET socket, const char* data, int size) {
	int sentBytes = 0;
	while (sentBytes < size) {
		const int sent = send(socket, data + sentBytes, size - sentBytes, 0);
		if (sent == SOCKET_ERROR || sent == 0) return false;
		sentBytes += sent;
	}
	return true;
}

bool SendAll(SOCKET socket, const std::vector<char>& packet) {
	return SendAll(socket, packet.data(), static_cast<int>(packet.size()));
}

bool ReceiveAll(SOCKET socket, char* buffer, int size) {
	int receivedBytes = 0;
	while (receivedBytes < size) {
		const int received = recv(socket, buffer + receivedBytes, size - receivedBytes, 0);
		if (received == SOCKET_ERROR || received == 0) return false;
		receivedBytes += received;
	}
	return true;
}

bool ReceiveLine(SOCKET socket, std::string& line) {
	line.clear();
	while (true) {
		char ch = 0;
		const int received = recv(socket, &ch, 1, 0);
		if (received == SOCKET_ERROR || received == 0) return false;
		if (ch == '\n') return true;
		if (ch != '\r') line.push_back(ch);
	}
}
} // namespace

int main(int argc, char* argv[]) {
	std::string serverIp = kDefaultServerIp;
	unsigned short adminPort = kDefaultAdminPort;
	if (argc >= 2) serverIp = argv[1];
	if (argc >= 3 && !TryParsePort(argv[2], adminPort)) {
		std::cerr << "Usage: RaisingPetAdminClient.exe [server-ip] [admin-port]\n";
		return 1;
	}
	if (argc > 3) {
		std::cerr << "Usage: RaisingPetAdminClient.exe [server-ip] [admin-port]\n";
		return 1;
	}

	WinsockSession winsock;
	if (!winsock.IsInitialized()) {
		std::cerr << "WSAStartup failed: " << WSAGetLastError() << '\n';
		return 1;
	}

	const SOCKET socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketHandle == INVALID_SOCKET) {
		std::cerr << "socket failed: " << WSAGetLastError() << '\n';
		return 1;
	}

	sockaddr_in serverAddress{};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(adminPort);
	if (inet_pton(AF_INET, serverIp.c_str(), &serverAddress.sin_addr) != 1) {
		std::cerr << "Invalid server IP: " << serverIp << '\n';
		closesocket(socketHandle);
		return 1;
	}

	std::cout << "Connecting to RaisingPet admin server "
		<< serverIp << ':' << adminPort << "...\n";
	if (connect(socketHandle, reinterpret_cast<sockaddr*>(&serverAddress),
		sizeof(serverAddress)) == SOCKET_ERROR) {
		std::cerr << "connect failed: " << WSAGetLastError() << '\n';
		closesocket(socketHandle);
		return 1;
	}

	if (!SendAll(socketHandle, MakeAdminLoginPacket())) {
		std::cerr << "Failed to send admin login packet.\n";
		closesocket(socketHandle);
		return 1;
	}

	char header[4]{};
	if (!ReceiveAll(socketHandle, header, sizeof(header))) {
		std::cerr << "Failed to receive admin login response.\n";
		closesocket(socketHandle);
		return 1;
	}

	const std::uint16_t packetSize = ReadUInt16(header);
	const std::uint16_t packetType = ReadUInt16(header + 2);
	if (packetSize != 5 || packetType != static_cast<std::uint16_t>(PacketType::AdminLoginResponse)) {
		std::cerr << "Invalid admin login response.\n";
		closesocket(socketHandle);
		return 1;
	}

	char resultByte = 0;
	if (!ReceiveAll(socketHandle, &resultByte, 1)) {
		std::cerr << "Failed to receive admin login result.\n";
		closesocket(socketHandle);
		return 1;
	}

	const AuthResult result = static_cast<AuthResult>(static_cast<unsigned char>(resultByte));
	if (result != AuthResult::Success) {
		std::cerr << "Admin login failed. result=" << static_cast<int>(result) << '\n';
		closesocket(socketHandle);
		return 1;
	}

	std::cout << "Admin connected.\n";
	std::cout << "Available commands:\n";
	std::cout << "  addmoney {playerId|_allplayer} {amount|-all}\n";
	std::cout << "  clearIL {playerId|_allplayer} {I|L|IL}\n";
	std::cout << "  DB CLEAR\n";
	std::cout << "Type quit/exit to close.\n";

	std::string line;
	while (true) {
		std::cout << "admin> ";
		if (!std::getline(std::cin, line)) break;
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line == "quit" || line == "exit") break;
		if (line.empty()) continue;

		line.push_back('\n');
		if (!SendAll(socketHandle, line.data(), static_cast<int>(line.size()))) {
			std::cerr << "Failed to send command text. Connection closed.\n";
			break;
		}
		std::string response;
		if (!ReceiveLine(socketHandle, response)) {
			std::cerr << "Failed to receive server response. Connection closed.\n";
			break;
		}
		std::cout << response << '\n';
	}

	shutdown(socketHandle, SD_BOTH);
	closesocket(socketHandle);
	return 0;
}
