#include "stdafx.h"
#include "ClientNetworkManager.h"

#include <WS2tcpip.h>

#include <array>
#include <cstdint>

#pragma comment(lib, "Ws2_32.lib")

namespace
{
constexpr std::uint16_t kMaxPacketSize = 1024;

enum class PacketType : std::uint16_t
{
	RegisterRequest = 1,
	RegisterResponse = 2,
	LoginRequest = 3,
	LoginResponse = 4,
};

std::uint16_t ReadUInt16(const char* data)
{
	std::uint16_t value = 0;
	std::memcpy(&value, data, sizeof(value));
	return(value);
}

void WriteUInt16(std::vector<char>& buffer, std::uint16_t value)
{
	const char* bytes = reinterpret_cast<const char*>(&value);
	buffer.insert(buffer.end(), bytes, bytes + sizeof(value));
}

bool IsAccountTextValid(const std::string& text, std::size_t maxLength)
{
	if (text.empty() || text.size() > maxLength) return(false);
	for (char ch : text)
	{
		if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
			(ch >= '0' && ch <= '9'))) return(false);
	}
	return(true);
}

std::vector<char> MakeAuthRequestPacket(CLIENT_AUTH_REQUEST request,
	const std::string& id, const std::string& password)
{
	const PacketType packetType = (request == CLIENT_AUTH_REQUEST::REGISTER)
		? PacketType::RegisterRequest : PacketType::LoginRequest;

	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(
		4 + 4 + id.size() + password.size());
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(packetType));
	WriteUInt16(packet, static_cast<std::uint16_t>(id.size()));
	WriteUInt16(packet, static_cast<std::uint16_t>(password.size()));
	packet.insert(packet.end(), id.begin(), id.end());
	packet.insert(packet.end(), password.begin(), password.end());
	return(packet);
}

bool SendAll(SOCKET socket, const std::vector<char>& packet)
{
	int sentBytes = 0;
	while (sentBytes < static_cast<int>(packet.size()))
	{
		const int sent = send(socket, packet.data() + sentBytes,
			static_cast<int>(packet.size()) - sentBytes, 0);
		if (sent == SOCKET_ERROR || sent == 0) return(false);
		sentBytes += sent;
	}
	return(true);
}

bool ReceiveAll(SOCKET socket, char* buffer, int size)
{
	int receivedBytes = 0;
	while (receivedBytes < size)
	{
		const int received = recv(socket, buffer + receivedBytes, size - receivedBytes, 0);
		if (received == SOCKET_ERROR || received == 0) return(false);
		receivedBytes += received;
	}
	return(true);
}
}

CClientNetworkManager::CClientNetworkManager()
{
	WSADATA data = {};
	WSAStartup(MAKEWORD(2, 2), &data);
}

CClientNetworkManager::~CClientNetworkManager()
{
	Disconnect();
	WSACleanup();
}

bool CClientNetworkManager::StartRegister(const std::string& id, const std::string& password)
{
	return(StartAuthRequest(CLIENT_AUTH_REQUEST::REGISTER, id, password));
}

bool CClientNetworkManager::StartLogin(const std::string& id, const std::string& password)
{
	return(StartAuthRequest(CLIENT_AUTH_REQUEST::LOGIN, id, password));
}

bool CClientNetworkManager::StartAuthRequest(CLIENT_AUTH_REQUEST request,
	const std::string& id, const std::string& password)
{
	if (!IsAccountTextValid(id, 32) || !IsAccountTextValid(password, 64)) return(false);
	Disconnect();

	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_bHasCompletedResult = false;
		m_CompletedRequest = CLIENT_AUTH_REQUEST::NONE;
	}

	m_bStopRequested.store(false);
	m_bBusy.store(true);
	m_WorkerThread = std::thread(&CClientNetworkManager::AuthThread, this, request, id, password);
	return(true);
}

void CClientNetworkManager::Disconnect()
{
	m_bStopRequested.store(true);
	CloseSocket();
	if (m_WorkerThread.joinable()) m_WorkerThread.join();
	m_bBusy.store(false);
	m_bConnected.store(false);
}

bool CClientNetworkManager::ConsumeAuthResult(CLIENT_AUTH_REQUEST& request,
	CLIENT_AUTH_RESULT& result)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (!m_bHasCompletedResult) return(false);
	request = m_CompletedRequest;
	result = m_CompletedResult;
	m_bHasCompletedResult = false;
	return(true);
}

bool CClientNetworkManager::IsConnected() const
{
	return(m_bConnected.load());
}

CClientNetworkManager::SERVER_INFORMATION CClientNetworkManager::LoadServerInformation() const
{
	CreateDirectoryA("Network", NULL);

	SERVER_INFORMATION information;
	std::ifstream input("Network\\ServerInformation.txt");
	if (!input)
	{
		std::ofstream output("Network\\ServerInformation.txt", std::ios::trunc);
		if (output) output << information.ip << '\n' << information.port << '\n';
		return(information);
	}

	std::string ip;
	unsigned int port = 0;
	input >> ip >> port;
	if (!ip.empty()) information.ip = ip;
	if (port >= 1 && port <= 65535) information.port = static_cast<unsigned short>(port);
	return(information);
}

void CClientNetworkManager::StoreResult(CLIENT_AUTH_REQUEST request, CLIENT_AUTH_RESULT result)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_CompletedRequest = request;
	m_CompletedResult = result;
	m_bHasCompletedResult = true;
}

void CClientNetworkManager::CloseSocket()
{
	SOCKET socketToClose = INVALID_SOCKET;
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		socketToClose = m_Socket;
		m_Socket = INVALID_SOCKET;
	}
	if (socketToClose == INVALID_SOCKET) return;
	shutdown(socketToClose, SD_BOTH);
	closesocket(socketToClose);
}

void CClientNetworkManager::AuthThread(CLIENT_AUTH_REQUEST request,
	std::string id, std::string password)
{
	const SERVER_INFORMATION information = LoadServerInformation();
	SOCKET connectedSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connectedSocket == INVALID_SOCKET)
	{
		StoreResult(request, CLIENT_AUTH_RESULT::NETWORK_ERROR);
		m_bBusy.store(false);
		return;
	}

	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Socket = connectedSocket;
	}

	sockaddr_in address = {};
	address.sin_family = AF_INET;
	address.sin_port = htons(information.port);
	if (inet_pton(AF_INET, information.ip.c_str(), &address.sin_addr) != 1)
	{
		CloseSocket();
		StoreResult(request, CLIENT_AUTH_RESULT::NETWORK_ERROR);
		m_bBusy.store(false);
		return;
	}

	if (connect(connectedSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
	{
		CloseSocket();
		StoreResult(request, CLIENT_AUTH_RESULT::NETWORK_ERROR);
		m_bBusy.store(false);
		return;
	}

	m_bConnected.store(true);

	const std::vector<char> packet = MakeAuthRequestPacket(request, id, password);
	if (!SendAll(connectedSocket, packet))
	{
		CloseSocket();
		StoreResult(request, CLIENT_AUTH_RESULT::NETWORK_ERROR);
		m_bBusy.store(false);
		return;
	}

	char header[4] = {};
	if (!ReceiveAll(connectedSocket, header, sizeof(header)))
	{
		CloseSocket();
		StoreResult(request, CLIENT_AUTH_RESULT::NETWORK_ERROR);
		m_bBusy.store(false);
		return;
	}

	const std::uint16_t packetSize = ReadUInt16(header);
	const std::uint16_t packetType = ReadUInt16(header + 2);
	if (packetSize < 5 || packetSize > kMaxPacketSize)
	{
		CloseSocket();
		StoreResult(request, CLIENT_AUTH_RESULT::SERVER_ERROR);
		m_bBusy.store(false);
		return;
	}

	std::vector<char> payload(packetSize - 4);
	if (!ReceiveAll(connectedSocket, payload.data(), static_cast<int>(payload.size())))
	{
		CloseSocket();
		StoreResult(request, CLIENT_AUTH_RESULT::NETWORK_ERROR);
		m_bBusy.store(false);
		return;
	}

	const PacketType expectedResponse = (request == CLIENT_AUTH_REQUEST::REGISTER)
		? PacketType::RegisterResponse : PacketType::LoginResponse;
	if (packetType != static_cast<std::uint16_t>(expectedResponse) || payload.empty())
	{
		CloseSocket();
		StoreResult(request, CLIENT_AUTH_RESULT::SERVER_ERROR);
		m_bBusy.store(false);
		return;
	}

	const CLIENT_AUTH_RESULT result = static_cast<CLIENT_AUTH_RESULT>(
		static_cast<unsigned char>(payload[0]));
	StoreResult(request, result);

	if (request != CLIENT_AUTH_REQUEST::LOGIN || result != CLIENT_AUTH_RESULT::SUCCESS)
	{
		CloseSocket();
		m_bBusy.store(false);
		return;
	}

	m_bBusy.store(false);
	while (!m_bStopRequested.load())
	{
		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(connectedSocket, &readSet);
		timeval timeout = {};
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		const int selected = select(0, &readSet, nullptr, nullptr, &timeout);
		if (selected == SOCKET_ERROR) break;
		if (selected > 0 && FD_ISSET(connectedSocket, &readSet))
		{
			std::array<char, 256> discardBuffer = {};
			const int received = recv(connectedSocket, discardBuffer.data(),
				static_cast<int>(discardBuffer.size()), 0);
			if (received <= 0) break;
		}
	}

	CloseSocket();
	m_bConnected.store(false);
}
