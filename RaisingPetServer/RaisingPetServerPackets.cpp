// packet and session notification helpers for RaisingPetServer.cpp
// This file is intentionally included by RaisingPetServer.cpp to preserve the original internal linkage.

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

bool SetNonBlocking(SOCKET socket) {
	u_long mode = 1;
	return ioctlsocket(socket, FIONBIO, &mode) != SOCKET_ERROR;
}

std::uint16_t ReadUInt16(const char* data) {
	std::uint16_t value = 0;
	std::memcpy(&value, data, sizeof(value));
	return value;
}

std::uint64_t ReadUInt64(const char* data) {
	std::uint64_t value = 0;
	std::memcpy(&value, data, sizeof(value));
	return value;
}

std::uint32_t ReadUInt32(const char* data) {
	std::uint32_t value = 0;
	std::memcpy(&value, data, sizeof(value));
	return value;
}

void WriteUInt16(std::vector<char>& buffer, std::uint16_t value) {
	const char* bytes = reinterpret_cast<const char*>(&value);
	buffer.insert(buffer.end(), bytes, bytes + sizeof(value));
}

void WriteUInt32(std::vector<char>& buffer, std::uint32_t value) {
	const char* bytes = reinterpret_cast<const char*>(&value);
	buffer.insert(buffer.end(), bytes, bytes + sizeof(value));
}

void WriteInt64(std::vector<char>& buffer, std::int64_t value) {
	const char* bytes = reinterpret_cast<const char*>(&value);
	buffer.insert(buffer.end(), bytes, bytes + sizeof(value));
}

void WriteUInt64(std::vector<char>& buffer, std::uint64_t value) {
	const char* bytes = reinterpret_cast<const char*>(&value);
	buffer.insert(buffer.end(), bytes, bytes + sizeof(value));
}

bool ParseAuthRequest(const std::vector<char>& payload, AuthRequest& request) {
	if (payload.size() < 4) return false;
	const std::uint16_t idLength = ReadUInt16(payload.data());
	const std::uint16_t passwordLength = ReadUInt16(payload.data() + 2);
	if (idLength == 0 || passwordLength == 0) return false;
	if (idLength > 32 || passwordLength > 64) return false;
	if (payload.size() != 4u + idLength + passwordLength) return false;

	request.id.assign(payload.data() + 4, payload.data() + 4 + idLength);
	request.password.assign(payload.data() + 4 + idLength,
		payload.data() + 4 + idLength + passwordLength);
	return true;
}

bool ParseStockIssueRequest(const std::vector<char>& payload, std::string& stockName) {
	if (payload.size() < sizeof(std::uint16_t)) return false;
	const std::uint16_t stockNameLength = ReadUInt16(payload.data());
	if (stockNameLength == 0 || stockNameLength > 150) return false;
	if (payload.size() != sizeof(std::uint16_t) + stockNameLength) return false;
	stockName.assign(payload.data() + sizeof(std::uint16_t),
		payload.data() + sizeof(std::uint16_t) + stockNameLength);
	for (unsigned char ch : stockName) {
		if (ch < 0x20 || ch == 0x7F) return false;
	}
	return true;
}

bool IsUtf8DisplayTextValid(const std::string& text, std::size_t maxBytes) {
	if (text.empty() || text.size() > maxBytes) return false;
	bool hasNonWhitespace = false;
	for (unsigned char ch : text) {
		if (ch < 0x20 || ch == 0x7F) return false;
		if (!(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'))
			hasNonWhitespace = true;
	}
	return hasNonWhitespace;
}

bool ParseNicknameSetupRequest(const std::vector<char>& payload, std::string& nickname) {
	if (payload.size() < sizeof(std::uint16_t)) return false;
	const std::uint16_t nicknameLength = ReadUInt16(payload.data());
	if (payload.size() != sizeof(std::uint16_t) + nicknameLength) return false;
	nickname.assign(payload.data() + sizeof(std::uint16_t),
		payload.data() + sizeof(std::uint16_t) + nicknameLength);
	return IsUtf8DisplayTextValid(nickname, 96);
}

bool ParseStockTradeRequest(const std::vector<char>& payload,
	StockTradeAction& action, std::uint32_t& stockId, std::uint32_t& quantity) {
	constexpr std::size_t expectedSize = 1 + sizeof(std::uint32_t) + sizeof(std::uint32_t);
	if (payload.size() != expectedSize) return false;
	const std::uint8_t actionValue = static_cast<std::uint8_t>(payload[0]);
	if (actionValue > 1) return false;
	action = (actionValue == 0) ? StockTradeAction::Buy : StockTradeAction::Sell;
	stockId = ReadUInt32(payload.data() + 1);
	quantity = ReadUInt32(payload.data() + 1 + sizeof(std::uint32_t));
	return true;
}

std::vector<char> MakePacketWithResult(PacketType type, AuthResult result) {
	std::vector<char> packet;
	packet.reserve(sizeof(PacketHeader) + 1);
	WriteUInt16(packet, static_cast<std::uint16_t>(sizeof(PacketHeader) + 1));
	WriteUInt16(packet, static_cast<std::uint16_t>(type));
	packet.push_back(static_cast<char>(result));
	return packet;
}

std::vector<char> MakeAuthResponse(PacketType type, AuthResult result) {
	std::vector<char> packet;
	const bool includeFirstLoginState = (type == PacketType::LoginResponse);
	const std::uint16_t packetSize = static_cast<std::uint16_t>(
		sizeof(PacketHeader) + 1 + (includeFirstLoginState ? 1 : 0));
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(type));
	packet.push_back(static_cast<char>(result));
	if (includeFirstLoginState) packet.push_back(0);
	return packet;
}

std::vector<char> MakeLoginResponse(AuthResult result, bool hasLoggedIn) {
	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(sizeof(PacketHeader) + 2);
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(PacketType::LoginResponse));
	packet.push_back(static_cast<char>(result));
	packet.push_back(hasLoggedIn ? 1 : 0);
	return packet;
}

std::vector<char> MakeMoneyChangedPacket(std::int64_t delta, std::uint64_t finalMoney) {
	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(
		sizeof(PacketHeader) + sizeof(std::int64_t) + sizeof(std::uint64_t));
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(PacketType::MoneyChanged));
	WriteInt64(packet, delta);
	WriteUInt64(packet, finalMoney);
	return packet;
}

std::vector<char> MakeFinancialApplicationResponse(PacketType type,
	const FinancialApplicationResult& result) {
	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(
		sizeof(PacketHeader) + 1 + sizeof(std::uint32_t) + sizeof(std::uint32_t) +
		sizeof(std::int64_t) + sizeof(std::uint64_t));
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(type));
	packet.push_back(static_cast<char>(result.result));
	WriteUInt32(packet, result.productId);
	WriteUInt32(packet, result.durationSeconds);
	WriteInt64(packet, result.moneyDelta);
	WriteUInt64(packet, result.finalMoney);
	return packet;
}

std::vector<char> MakeFinancialProductCompletedPacket(FinancialCategory category,
	std::uint32_t productId) {
	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(
		sizeof(PacketHeader) + 1 + sizeof(std::uint32_t));
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(PacketType::FinancialProductCompleted));
	packet.push_back(static_cast<char>(category));
	WriteUInt32(packet, productId);
	return packet;
}

std::vector<char> MakeFinancialProductActivePacket(const FinancialActiveStatus& status) {
	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(
		sizeof(PacketHeader) + 1 + sizeof(std::uint32_t) + sizeof(std::uint32_t));
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(PacketType::FinancialProductActive));
	packet.push_back(static_cast<char>(status.category));
	WriteUInt32(packet, status.productId);
	WriteUInt32(packet, status.remainingSeconds);
	return packet;
}

std::vector<char> MakeStockIssueResponse(const StockIssueApplicationResult& result) {
	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(
		sizeof(PacketHeader) + 1 + sizeof(std::uint64_t) + sizeof(std::uint32_t));
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(PacketType::StockIssueResponse));
	packet.push_back(static_cast<char>(result.result));
	WriteUInt64(packet, result.finalMoney);
	WriteUInt32(packet, result.stockId);
	return packet;
}

std::vector<char> MakeStockTradeResponse(const StockTradeApplicationResult& result) {
	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(
		sizeof(PacketHeader) + 1 + 1 + sizeof(std::uint32_t) +
		sizeof(std::uint32_t) + sizeof(std::uint64_t));
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(PacketType::StockTradeResponse));
	packet.push_back(static_cast<char>(result.action));
	packet.push_back(static_cast<char>(result.result));
	WriteUInt32(packet, result.stockId);
	WriteUInt32(packet, result.quantity);
	WriteUInt64(packet, result.finalMoney);
	return packet;
}

std::vector<char> MakeStockIssueStatusPacket(bool issued, const std::string& stockName) {
	const std::uint16_t stockNameLength = issued
		? static_cast<std::uint16_t>(stockName.size()) : 0;
	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(
		sizeof(PacketHeader) + 1 + sizeof(std::uint16_t) + stockNameLength);
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(PacketType::StockIssueStatus));
	packet.push_back(issued ? 1 : 0);
	WriteUInt16(packet, stockNameLength);
	packet.insert(packet.end(), stockName.begin(), stockName.begin() + stockNameLength);
	return packet;
}

std::vector<char> MakeStockManagementInfoResponse(const StockManagementInfo& info) {
	const std::uint16_t stockNameLength = info.issued
		? static_cast<std::uint16_t>(info.stockName.size()) : 0;
	std::uint16_t holderNameLengths[3] = {};
	std::uint16_t variableSize = stockNameLength;
	for (int i = 0; i < 3; ++i) {
		holderNameLengths[i] = static_cast<std::uint16_t>(info.topHolders[i].playerId.size());
		variableSize = static_cast<std::uint16_t>(variableSize + holderNameLengths[i]);
	}
	const std::uint8_t recentPriceCount =
		static_cast<std::uint8_t>(std::min<size_t>(info.recentPrices.size(), 10));
	std::uint16_t recentTimeLengths[10] = {};
	for (std::uint8_t i = 0; i < recentPriceCount; ++i) {
		recentTimeLengths[i] = static_cast<std::uint16_t>(
			std::min<size_t>(info.recentPrices[i].changedTime.size(), 32));
		variableSize = static_cast<std::uint16_t>(variableSize + recentTimeLengths[i]);
	}

	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(
		sizeof(PacketHeader) + 1 + sizeof(std::uint16_t) + stockNameLength +
		sizeof(std::uint32_t) * 3 + sizeof(std::uint64_t) + sizeof(std::uint32_t) +
		sizeof(std::uint64_t) * 2 +
		(sizeof(std::uint16_t) + sizeof(std::uint32_t)) * 3 +
		1 + recentPriceCount * (sizeof(std::uint64_t) * 4 + sizeof(std::uint16_t)) +
		variableSize - stockNameLength);
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(PacketType::StockManagementInfoResponse));
	packet.push_back(info.issued ? 1 : 0);
	WriteUInt16(packet, stockNameLength);
	packet.insert(packet.end(), info.stockName.begin(), info.stockName.begin() + stockNameLength);
	WriteUInt32(packet, info.soldQuantity);
	WriteUInt32(packet, info.unsoldQuantity);
	WriteUInt32(packet, info.saleableQuantity);
	WriteUInt64(packet, info.issuanceRevenue);
	WriteUInt32(packet, info.recentTradeQuantity);
	WriteUInt64(packet, info.currentPrice);
	WriteUInt64(packet, info.previousPrice);
	for (int i = 0; i < 3; ++i) {
		WriteUInt16(packet, holderNameLengths[i]);
		packet.insert(packet.end(), info.topHolders[i].playerId.begin(),
			info.topHolders[i].playerId.begin() + holderNameLengths[i]);
		WriteUInt32(packet, info.topHolders[i].quantity);
	}
	packet.push_back(static_cast<char>(recentPriceCount));
	for (std::uint8_t i = 0; i < recentPriceCount; ++i) {
		const StockPriceSeeInfo& price = info.recentPrices[i];
		WriteUInt64(packet, price.previousPrice);
		WriteUInt64(packet, price.newPrice);
		WriteUInt64(packet, price.boughtQuantity);
		WriteUInt64(packet, price.soldQuantity);
		WriteUInt16(packet, recentTimeLengths[i]);
		packet.insert(packet.end(), price.changedTime.begin(),
			price.changedTime.begin() + recentTimeLengths[i]);
	}
	return packet;
}

std::vector<char> MakeStockTransactionListResponse(const std::vector<StockTransactionInfo>& infos) {
	const std::uint8_t stockCount = static_cast<std::uint8_t>(std::min<size_t>(infos.size(), 20));
	std::uint16_t variableSize = 0;
	std::uint16_t stockNameLengths[20] = {};
	std::uint16_t issuerDisplayNameLengths[20] = {};
	std::uint8_t recentPriceCounts[20] = {};
	std::uint16_t recentTimeLengths[20][10] = {};
	std::uint16_t fixedRecentSize = 0;
	for (std::uint8_t i = 0; i < stockCount; ++i) {
		stockNameLengths[i] = static_cast<std::uint16_t>(
			std::min<size_t>(infos[i].stockName.size(), 150));
		const std::string& issuerDisplayName = infos[i].issuerNickname.empty()
			? infos[i].issuerId : infos[i].issuerNickname;
		issuerDisplayNameLengths[i] = static_cast<std::uint16_t>(
			std::min<size_t>(issuerDisplayName.size(), 96));
		variableSize = static_cast<std::uint16_t>(
			variableSize + stockNameLengths[i] + issuerDisplayNameLengths[i]);
		recentPriceCounts[i] = static_cast<std::uint8_t>(
			std::min<size_t>(infos[i].recentPrices.size(), 10));
		for (std::uint8_t priceIndex = 0; priceIndex < recentPriceCounts[i]; ++priceIndex) {
			recentTimeLengths[i][priceIndex] = static_cast<std::uint16_t>(
				std::min<size_t>(infos[i].recentPrices[priceIndex].changedTime.size(), 32));
			variableSize = static_cast<std::uint16_t>(
				variableSize + recentTimeLengths[i][priceIndex]);
			fixedRecentSize = static_cast<std::uint16_t>(
				fixedRecentSize + sizeof(std::uint64_t) * 4 + sizeof(std::uint16_t));
		}
	}

	std::vector<char> packet;
	const std::uint16_t packetSize = static_cast<std::uint16_t>(
		sizeof(PacketHeader) + 1 +
		stockCount * (sizeof(std::uint32_t) + sizeof(std::uint16_t) +
			sizeof(std::uint16_t) + sizeof(std::uint64_t) * 2 + sizeof(std::uint32_t) * 3 + 1) +
		fixedRecentSize + variableSize);
	packet.reserve(packetSize);
	WriteUInt16(packet, packetSize);
	WriteUInt16(packet, static_cast<std::uint16_t>(PacketType::StockTransactionListResponse));
	packet.push_back(static_cast<char>(stockCount));
	for (std::uint8_t i = 0; i < stockCount; ++i) {
		const StockTransactionInfo& info = infos[i];
		WriteUInt32(packet, info.stockId);
		WriteUInt16(packet, stockNameLengths[i]);
		packet.insert(packet.end(), info.stockName.begin(),
			info.stockName.begin() + stockNameLengths[i]);
		const std::string& issuerDisplayName = info.issuerNickname.empty()
			? info.issuerId : info.issuerNickname;
		WriteUInt16(packet, issuerDisplayNameLengths[i]);
		packet.insert(packet.end(), issuerDisplayName.begin(),
			issuerDisplayName.begin() + issuerDisplayNameLengths[i]);
		WriteUInt64(packet, info.currentPrice);
		WriteUInt64(packet, info.previousPrice);
		WriteUInt32(packet, info.saleableQuantity);
		WriteUInt32(packet, info.myQuantity);
		WriteUInt32(packet, info.recentTradeQuantity);
		packet.push_back(static_cast<char>(recentPriceCounts[i]));
		for (std::uint8_t priceIndex = 0; priceIndex < recentPriceCounts[i]; ++priceIndex) {
			const StockPriceSeeInfo& price = info.recentPrices[priceIndex];
			WriteUInt64(packet, price.previousPrice);
			WriteUInt64(packet, price.newPrice);
			WriteUInt64(packet, price.boughtQuantity);
			WriteUInt64(packet, price.soldQuantity);
			WriteUInt16(packet, recentTimeLengths[i][priceIndex]);
			packet.insert(packet.end(), price.changedTime.begin(),
				price.changedTime.begin() + recentTimeLengths[i][priceIndex]);
		}
	}
	return packet;
}

bool SendAll(SOCKET socket, const std::vector<char>& packet) {
	int sentBytes = 0;
	while (sentBytes < static_cast<int>(packet.size())) {
		const int sent = send(socket, packet.data() + sentBytes,
			static_cast<int>(packet.size()) - sentBytes, 0);
		if (sent == SOCKET_ERROR || sent == 0) return false;
		sentBytes += sent;
	}
	return true;
}

bool SendTextLine(SOCKET socket, const std::string& text) {
	std::string line = text;
	line.push_back('\n');
	int sentBytes = 0;
	while (sentBytes < static_cast<int>(line.size())) {
		const int sent = send(socket, line.data() + sentBytes,
			static_cast<int>(line.size()) - sentBytes, 0);
		if (sent == SOCKET_ERROR || sent == 0) return false;
		sentBytes += sent;
	}
	return true;
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

void QueuePacket(ClientSession& session, const std::vector<char>& packet) {
	session.sendBuffer.insert(session.sendBuffer.end(), packet.begin(), packet.end());
}

bool QueueMoneyChangedForOnlineSession(std::vector<ClientSession>& sessions,
	std::mutex& sessionsMutex, const std::string& playerId, std::int64_t delta,
	std::uint64_t finalMoney) {
	const std::vector<char> packet = MakeMoneyChangedPacket(delta, finalMoney);
	std::lock_guard<std::mutex> lock(sessionsMutex);
	for (ClientSession& session : sessions) {
		if (!session.authenticated || session.playerId != playerId ||
			session.socket == INVALID_SOCKET) continue;
		QueuePacket(session, packet);
		return true;
	}
	return false;
}

void QueueFinancialChanges(ClientSession& session,
	const std::vector<FinancialMoneyChange>& changes) {
	for (const FinancialMoneyChange& change : changes) {
		QueuePacket(session, MakeMoneyChangedPacket(change.moneyDelta, change.finalMoney));
		if (change.completed)
			QueuePacket(session, MakeFinancialProductCompletedPacket(change.category, change.productId));
	}
}

void QueueFinancialActiveStatuses(ClientSession& session,
	const std::vector<FinancialActiveStatus>& statuses) {
	for (const FinancialActiveStatus& status : statuses)
		QueuePacket(session, MakeFinancialProductActivePacket(status));
}

void QueueStockManagementInfosForAuthenticatedSessions(Database& database,
	std::vector<ClientSession>& sessions, std::mutex& sessionsMutex) {
	std::lock_guard<std::mutex> lock(sessionsMutex);
	for (ClientSession& session : sessions) {
		if (!session.authenticated || session.socket == INVALID_SOCKET) continue;
		StockManagementInfo info;
		std::string reason;
		if (database.LoadStockManagementInfo(session.playerId, info, reason))
			QueuePacket(session, MakeStockManagementInfoResponse(info));
		else
			std::cerr << "[StockManagementInfo] " << session.playerId
			<< " failed: " << reason << '\n';
	}
}

void QueueFinancialClearNotifications(std::vector<ClientSession>& sessions,
	std::mutex& sessionsMutex, const std::vector<FinancialClearResult>& results) {
	std::lock_guard<std::mutex> lock(sessionsMutex);
	for (ClientSession& session : sessions) {
		if (!session.authenticated || session.socket == INVALID_SOCKET) continue;
		for (const FinancialClearResult& result : results) {
			if (result.playerId != session.playerId) continue;
			if (result.savingsCleared)
				QueuePacket(session, MakeFinancialProductCompletedPacket(
					FinancialCategory::Savings, result.savingsId));
			if (result.loanCleared)
				QueuePacket(session, MakeFinancialProductCompletedPacket(
					FinancialCategory::Loan, result.loanId));
			break;
		}
	}
}

void QueueAdminMoneyChanges(std::vector<ClientSession>& sessions,
	std::mutex& sessionsMutex, const std::vector<AdminMoneyChangeResult>& results) {
	std::lock_guard<std::mutex> lock(sessionsMutex);
	for (ClientSession& session : sessions) {
		if (!session.authenticated || session.socket == INVALID_SOCKET) continue;
		for (const AdminMoneyChangeResult& result : results) {
			if (result.playerId != session.playerId) continue;
			QueuePacket(session, MakeMoneyChangedPacket(result.delta, result.finalMoney));
			break;
		}
	}
}

