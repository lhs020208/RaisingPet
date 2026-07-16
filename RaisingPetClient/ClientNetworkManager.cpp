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
	MoneyUpdate = 5,
	MoneyChanged = 6,
	SavingsJoinRequest = 7,
	SavingsJoinResponse = 8,
	LoanApplyRequest = 9,
	LoanApplyResponse = 10,
	FinancialProductCompleted = 11,
	FinancialProductActive = 12,
	StockIssueRequest = 13,
	StockIssueResponse = 14,
	StockIssueStatus = 15,
	StockManagementInfoRequest = 16,
	StockManagementInfoResponse = 17,
};

std::uint16_t ReadUInt16(const char* data)
{
	std::uint16_t value = 0;
	std::memcpy(&value, data, sizeof(value));
	return(value);
}

std::int64_t ReadInt64(const char* data)
{
	std::int64_t value = 0;
	std::memcpy(&value, data, sizeof(value));
	return(value);
}

std::uint64_t ReadUInt64(const char* data)
{
	std::uint64_t value = 0;
	std::memcpy(&value, data, sizeof(value));
	return(value);
}

std::uint32_t ReadUInt32(const char* data)
{
	std::uint32_t value = 0;
	std::memcpy(&value, data, sizeof(value));
	return(value);
}

void WriteUInt16(std::vector<char>& buffer, std::uint16_t value)
{
	const char* bytes = reinterpret_cast<const char*>(&value);
	buffer.insert(buffer.end(), bytes, bytes + sizeof(value));
}

void WriteUInt32(std::vector<char>& buffer, std::uint32_t value)
{
	const char* bytes = reinterpret_cast<const char*>(&value);
	buffer.insert(buffer.end(), bytes, bytes + sizeof(value));
}

void WriteUInt64(std::vector<char>& buffer, std::uint64_t value)
{
	const char* bytes = reinterpret_cast<const char*>(&value);
	buffer.insert(buffer.end(), bytes, bytes + sizeof(value));
}

std::vector<char> MakeFinancialProductRequestPacket(PacketType packetType,
	unsigned int productId)
{
	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(4 + sizeof(std::uint32_t));
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(packetType));
	WriteUInt32(packet, static_cast<std::uint32_t>(productId));
	return(packet);
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

std::vector<char> MakeMoneyUpdatePacket(unsigned int money)
{
	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(4 + sizeof(std::uint64_t));
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(PacketType::MoneyUpdate));
	WriteUInt64(packet, static_cast<std::uint64_t>(money));
	return(packet);
}

std::vector<char> MakeStockIssueRequestPacket(const std::string& stockNameUtf8)
{
	std::vector<char> packet;
	const std::uint16_t stockNameLength = static_cast<std::uint16_t>(stockNameUtf8.size());
	const std::uint16_t packetSize = static_cast<std::uint16_t>(
		4 + sizeof(std::uint16_t) + stockNameLength);
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(PacketType::StockIssueRequest));
	WriteUInt16(packet, stockNameLength);
	packet.insert(packet.end(), stockNameUtf8.begin(), stockNameUtf8.end());
	return(packet);
}

std::vector<char> MakeStockManagementInfoRequestPacket()
{
	std::vector<char> packet;
	const std::uint16_t packetSize = 4;
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(PacketType::StockManagementInfoRequest));
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

bool ProcessPersistentPacketBuffer(std::vector<char>& receiveBuffer,
	std::vector<std::pair<std::int64_t, unsigned int>>& moneyChanges,
	std::vector<CLIENT_FINANCIAL_APPLICATION_RESULT>& financialResults,
	std::vector<CLIENT_FINANCIAL_COMPLETION>& financialCompletions,
	std::vector<CLIENT_FINANCIAL_ACTIVE_STATUS>& financialActiveStatuses,
	std::vector<CLIENT_STOCK_ISSUE_APPLICATION_RESULT>& stockIssueResults,
	std::vector<CLIENT_STOCK_ISSUE_STATUS>& stockIssueStatuses,
	std::vector<CLIENT_STOCK_MANAGEMENT_INFO>& stockManagementInfos)
{
	while (receiveBuffer.size() >= 4)
	{
		const std::uint16_t packetSize = ReadUInt16(receiveBuffer.data());
		const std::uint16_t packetType = ReadUInt16(receiveBuffer.data() + 2);
		if (packetSize < 4 || packetSize > kMaxPacketSize) return(false);
		if (receiveBuffer.size() < packetSize) return(true);

		const char* payload = receiveBuffer.data() + 4;
		const std::uint16_t payloadSize = static_cast<std::uint16_t>(packetSize - 4);
		if (packetType == static_cast<std::uint16_t>(PacketType::MoneyChanged))
		{
			if (payloadSize != sizeof(std::int64_t) + sizeof(std::uint64_t)) return(false);
			const std::int64_t delta = ReadInt64(payload);
			const std::uint64_t finalValue = ReadUInt64(payload + sizeof(std::int64_t));
			if (finalValue > UINT_MAX) return(false);
			moneyChanges.push_back({ delta, static_cast<unsigned int>(finalValue) });
		}
		else if (packetType == static_cast<std::uint16_t>(PacketType::SavingsJoinResponse) ||
			packetType == static_cast<std::uint16_t>(PacketType::LoanApplyResponse))
		{
			constexpr std::uint16_t expectedSize = 1 + sizeof(std::uint32_t) +
				sizeof(std::uint32_t) + sizeof(std::int64_t) + sizeof(std::uint64_t);
			if (payloadSize != expectedSize) return(false);
			const std::uint64_t finalValue = ReadUInt64(payload + 1 + sizeof(std::uint32_t) +
				sizeof(std::uint32_t) + sizeof(std::int64_t));
			if (finalValue > UINT_MAX) return(false);
			CLIENT_FINANCIAL_APPLICATION_RESULT financialResult;
			financialResult.eCategory =
				(packetType == static_cast<std::uint16_t>(PacketType::SavingsJoinResponse))
				? CLIENT_FINANCIAL_CATEGORY::SAVINGS : CLIENT_FINANCIAL_CATEGORY::LOAN;
			financialResult.eResult = static_cast<CLIENT_FINANCIAL_RESULT>(
				static_cast<unsigned char>(payload[0]));
			financialResult.nProductId = static_cast<unsigned int>(ReadUInt32(payload + 1));
			financialResult.nDurationSeconds = static_cast<unsigned int>(
				ReadUInt32(payload + 1 + sizeof(std::uint32_t)));
			financialResult.nMoneyDelta = ReadInt64(payload + 1 + sizeof(std::uint32_t) +
				sizeof(std::uint32_t));
			financialResult.nFinalMoney = static_cast<unsigned int>(finalValue);
			financialResults.push_back(financialResult);
		}
		else if (packetType == static_cast<std::uint16_t>(PacketType::FinancialProductCompleted))
		{
			if (payloadSize != 1 + sizeof(std::uint32_t)) return(false);
			const unsigned char category = static_cast<unsigned char>(payload[0]);
			if (category > 1) return(false);
			CLIENT_FINANCIAL_COMPLETION financialCompletion;
			financialCompletion.eCategory = (category == 0)
				? CLIENT_FINANCIAL_CATEGORY::SAVINGS : CLIENT_FINANCIAL_CATEGORY::LOAN;
			financialCompletion.nProductId = static_cast<unsigned int>(ReadUInt32(payload + 1));
			financialCompletions.push_back(financialCompletion);
		}
		else if (packetType == static_cast<std::uint16_t>(PacketType::FinancialProductActive))
		{
			if (payloadSize != 1 + sizeof(std::uint32_t) + sizeof(std::uint32_t)) return(false);
			const unsigned char category = static_cast<unsigned char>(payload[0]);
			if (category > 1) return(false);
			CLIENT_FINANCIAL_ACTIVE_STATUS status;
			status.eCategory = (category == 0)
				? CLIENT_FINANCIAL_CATEGORY::SAVINGS : CLIENT_FINANCIAL_CATEGORY::LOAN;
			status.nProductId = static_cast<unsigned int>(ReadUInt32(payload + 1));
			status.nRemainingSeconds = static_cast<unsigned int>(
				ReadUInt32(payload + 1 + sizeof(std::uint32_t)));
			financialActiveStatuses.push_back(status);
		}
		else if (packetType == static_cast<std::uint16_t>(PacketType::StockIssueResponse))
		{
			constexpr std::uint16_t expectedSize = 1 + sizeof(std::uint64_t) +
				sizeof(std::uint32_t);
			if (payloadSize != expectedSize) return(false);
			const std::uint64_t finalValue = ReadUInt64(payload + 1);
			if (finalValue > UINT_MAX) return(false);
			CLIENT_STOCK_ISSUE_APPLICATION_RESULT result;
			result.eResult = static_cast<CLIENT_STOCK_ISSUE_RESULT>(
				static_cast<unsigned char>(payload[0]));
			result.nFinalMoney = static_cast<unsigned int>(finalValue);
			result.nStockId = static_cast<unsigned int>(
				ReadUInt32(payload + 1 + sizeof(std::uint64_t)));
			stockIssueResults.push_back(result);
		}
		else if (packetType == static_cast<std::uint16_t>(PacketType::StockIssueStatus))
		{
			if (payloadSize < 1 + sizeof(std::uint16_t)) return(false);
			CLIENT_STOCK_ISSUE_STATUS status;
			status.bIssued = (payload[0] != 0);
			const std::uint16_t stockNameLength = ReadUInt16(payload + 1);
			if (stockNameLength > 150 ||
				payloadSize != 1 + sizeof(std::uint16_t) + stockNameLength)
				return(false);
			status.strStockNameUtf8.assign(payload + 1 + sizeof(std::uint16_t),
				payload + 1 + sizeof(std::uint16_t) + stockNameLength);
			stockIssueStatuses.push_back(status);
		}
		else if (packetType == static_cast<std::uint16_t>(PacketType::StockManagementInfoResponse))
		{
			const char* cursor = payload;
			const char* end = payload + payloadSize;
			if (cursor + 1 + sizeof(std::uint16_t) > end) return(false);
			CLIENT_STOCK_MANAGEMENT_INFO info;
			info.bIssued = (*cursor++ != 0);
			const std::uint16_t stockNameLength = ReadUInt16(cursor);
			cursor += sizeof(std::uint16_t);
			if (stockNameLength > 150 || cursor + stockNameLength > end) return(false);
			info.strStockNameUtf8.assign(cursor, cursor + stockNameLength);
			cursor += stockNameLength;
			if (cursor + sizeof(std::uint32_t) * 3 + sizeof(std::uint64_t) +
				sizeof(std::uint32_t) + sizeof(std::uint64_t) * 2 > end) return(false);
			info.nSoldQuantity = static_cast<unsigned int>(ReadUInt32(cursor));
			cursor += sizeof(std::uint32_t);
			info.nUnsoldQuantity = static_cast<unsigned int>(ReadUInt32(cursor));
			cursor += sizeof(std::uint32_t);
			info.nSaleableQuantity = static_cast<unsigned int>(ReadUInt32(cursor));
			cursor += sizeof(std::uint32_t);
			const std::uint64_t revenue = ReadUInt64(cursor);
			if (revenue > UINT_MAX) return(false);
			info.nIssuanceRevenue = static_cast<unsigned int>(revenue);
			cursor += sizeof(std::uint64_t);
			info.nRecentTradeQuantity = static_cast<unsigned int>(ReadUInt32(cursor));
			cursor += sizeof(std::uint32_t);
			const std::uint64_t currentPrice = ReadUInt64(cursor);
			if (currentPrice > UINT_MAX) return(false);
			info.nCurrentPrice = static_cast<unsigned int>(currentPrice);
			cursor += sizeof(std::uint64_t);
			const std::uint64_t previousPrice = ReadUInt64(cursor);
			if (previousPrice > UINT_MAX) return(false);
			info.nPreviousPrice = static_cast<unsigned int>(previousPrice);
			cursor += sizeof(std::uint64_t);
			for (int i = 0; i < 3; ++i)
			{
				if (cursor + sizeof(std::uint16_t) > end) return(false);
				const std::uint16_t holderNameLength = ReadUInt16(cursor);
				cursor += sizeof(std::uint16_t);
				if (holderNameLength > 32 || cursor + holderNameLength + sizeof(std::uint32_t) > end)
					return(false);
				info.TopHolders[i].strPlayerId.assign(cursor, cursor + holderNameLength);
				cursor += holderNameLength;
				info.TopHolders[i].nQuantity = static_cast<unsigned int>(ReadUInt32(cursor));
				cursor += sizeof(std::uint32_t);
			}
			if (cursor != end) return(false);
			stockManagementInfos.push_back(info);
		}

		receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.begin() + packetSize);
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

bool CClientNetworkManager::SendMoneyUpdate(unsigned int money)
{
	if (!m_bConnected.load()) return(false);
	const std::vector<char> packet = MakeMoneyUpdatePacket(money);

	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Socket == INVALID_SOCKET) return(false);
	return(SendAll(m_Socket, packet));
}

bool CClientNetworkManager::SendSavingsJoinRequest(unsigned int nProductId)
{
	if (!m_bConnected.load() || nProductId < 1 || nProductId > 10) return(false);
	const std::vector<char> packet = MakeFinancialProductRequestPacket(
		PacketType::SavingsJoinRequest, nProductId);

	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Socket == INVALID_SOCKET) return(false);
	return(SendAll(m_Socket, packet));
}

bool CClientNetworkManager::SendLoanApplyRequest(unsigned int nProductId)
{
	if (!m_bConnected.load() || nProductId < 1 || nProductId > 10) return(false);
	const std::vector<char> packet = MakeFinancialProductRequestPacket(
		PacketType::LoanApplyRequest, nProductId);

	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Socket == INVALID_SOCKET) return(false);
	return(SendAll(m_Socket, packet));
}

bool CClientNetworkManager::SendStockIssueRequest(const std::string& stockNameUtf8)
{
	if (!m_bConnected.load() || stockNameUtf8.empty() || stockNameUtf8.size() > 150)
		return(false);
	const std::vector<char> packet = MakeStockIssueRequestPacket(stockNameUtf8);

	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Socket == INVALID_SOCKET) return(false);
	return(SendAll(m_Socket, packet));
}

bool CClientNetworkManager::SendStockManagementInfoRequest()
{
	if (!m_bConnected.load()) return(false);
	const std::vector<char> packet = MakeStockManagementInfoRequestPacket();

	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Socket == INVALID_SOCKET) return(false);
	return(SendAll(m_Socket, packet));
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
		m_ServerMoneyChanges.clear();
		m_FinancialApplicationResults.clear();
		m_FinancialCompletions.clear();
		m_FinancialActiveStatuses.clear();
		m_StockIssueResults.clear();
		m_StockIssueStatuses.clear();
		m_StockManagementInfos.clear();
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

bool CClientNetworkManager::ConsumeServerMoneyChange(std::int64_t& deltaMoney,
	unsigned int& finalMoney)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_ServerMoneyChanges.empty()) return(false);
	deltaMoney = m_ServerMoneyChanges.front().first;
	finalMoney = m_ServerMoneyChanges.front().second;
	m_ServerMoneyChanges.erase(m_ServerMoneyChanges.begin());
	return(true);
}

bool CClientNetworkManager::ConsumeFinancialApplicationResult(
	CLIENT_FINANCIAL_APPLICATION_RESULT& result)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_FinancialApplicationResults.empty()) return(false);
	result = m_FinancialApplicationResults.front();
	m_FinancialApplicationResults.erase(m_FinancialApplicationResults.begin());
	return(true);
}

bool CClientNetworkManager::ConsumeFinancialCompletion(
	CLIENT_FINANCIAL_COMPLETION& completion)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_FinancialCompletions.empty()) return(false);
	completion = m_FinancialCompletions.front();
	m_FinancialCompletions.erase(m_FinancialCompletions.begin());
	return(true);
}

bool CClientNetworkManager::ConsumeFinancialActiveStatus(
	CLIENT_FINANCIAL_ACTIVE_STATUS& status)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_FinancialActiveStatuses.empty()) return(false);
	status = m_FinancialActiveStatuses.front();
	m_FinancialActiveStatuses.erase(m_FinancialActiveStatuses.begin());
	return(true);
}

bool CClientNetworkManager::ConsumeStockIssueResult(
	CLIENT_STOCK_ISSUE_APPLICATION_RESULT& result)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_StockIssueResults.empty()) return(false);
	result = m_StockIssueResults.front();
	m_StockIssueResults.erase(m_StockIssueResults.begin());
	return(true);
}

bool CClientNetworkManager::ConsumeStockIssueStatus(CLIENT_STOCK_ISSUE_STATUS& status)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_StockIssueStatuses.empty()) return(false);
	status = m_StockIssueStatuses.front();
	m_StockIssueStatuses.erase(m_StockIssueStatuses.begin());
	return(true);
}

bool CClientNetworkManager::ConsumeStockManagementInfo(CLIENT_STOCK_MANAGEMENT_INFO& info)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_StockManagementInfos.empty()) return(false);
	info = m_StockManagementInfos.front();
	m_StockManagementInfos.erase(m_StockManagementInfos.begin());
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
	std::vector<char> persistentReceiveBuffer;
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
			persistentReceiveBuffer.insert(persistentReceiveBuffer.end(),
				discardBuffer.data(), discardBuffer.data() + received);

			std::vector<std::pair<std::int64_t, unsigned int>> moneyChanges;
			std::vector<CLIENT_FINANCIAL_APPLICATION_RESULT> financialResults;
			std::vector<CLIENT_FINANCIAL_COMPLETION> financialCompletions;
			std::vector<CLIENT_FINANCIAL_ACTIVE_STATUS> financialActiveStatuses;
			std::vector<CLIENT_STOCK_ISSUE_APPLICATION_RESULT> stockIssueResults;
			std::vector<CLIENT_STOCK_ISSUE_STATUS> stockIssueStatuses;
			std::vector<CLIENT_STOCK_MANAGEMENT_INFO> stockManagementInfos;
			if (!ProcessPersistentPacketBuffer(persistentReceiveBuffer,
				moneyChanges, financialResults, financialCompletions, financialActiveStatuses,
				stockIssueResults, stockIssueStatuses, stockManagementInfos)) break;
			if (!moneyChanges.empty() || !financialResults.empty() ||
				!financialCompletions.empty() || !financialActiveStatuses.empty() ||
				!stockIssueResults.empty() || !stockIssueStatuses.empty() ||
				!stockManagementInfos.empty())
			{
				std::lock_guard<std::mutex> lock(m_Mutex);
				m_ServerMoneyChanges.insert(m_ServerMoneyChanges.end(),
					moneyChanges.begin(), moneyChanges.end());
				m_FinancialApplicationResults.insert(m_FinancialApplicationResults.end(),
					financialResults.begin(), financialResults.end());
				m_FinancialCompletions.insert(m_FinancialCompletions.end(),
					financialCompletions.begin(), financialCompletions.end());
				m_FinancialActiveStatuses.insert(m_FinancialActiveStatuses.end(),
					financialActiveStatuses.begin(), financialActiveStatuses.end());
				m_StockIssueResults.insert(m_StockIssueResults.end(),
					stockIssueResults.begin(), stockIssueResults.end());
				m_StockIssueStatuses.insert(m_StockIssueStatuses.end(),
					stockIssueStatuses.begin(), stockIssueStatuses.end());
				m_StockManagementInfos.insert(m_StockManagementInfos.end(),
					stockManagementInfos.begin(), stockManagementInfos.end());
			}
		}
	}

	CloseSocket();
	m_bConnected.store(false);
}
