// admin command server for RaisingPetServer.cpp
// This file is intentionally included by RaisingPetServer.cpp to preserve the original internal linkage.

std::string ToUpperAscii(std::string text) {
	for (char& ch : text) {
		if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 'a' + 'A');
	}
	return text;
}

bool ParseClearILMode(const std::string& modeText, bool& clearSavings, bool& clearLoan) {
	const std::string mode = ToUpperAscii(modeText);
	clearSavings = mode.find('I') != std::string::npos;
	clearLoan = mode.find('L') != std::string::npos;
	return (mode == "I" || mode == "L" || mode == "IL" || mode == "LI");
}

bool ParseOnOffMode(const std::string& modeText, bool& value) {
	const std::string mode = ToUpperAscii(modeText);
	if (mode == "ON" || mode == "TRUE" || mode == "1") {
		value = true;
		return true;
	}
	if (mode == "OFF" || mode == "FALSE" || mode == "0") {
		value = false;
		return true;
	}
	return false;
}

struct SeeCommandOptions {
	bool detail = false;
	bool onlineOnly = false;
	bool sortByMoney = false;
};

bool ParseSeeOptions(std::istringstream& input, SeeCommandOptions& options) {
	std::string token;
	while (input >> token) {
		const std::string upper = ToUpperAscii(token);
		for (const char ch : upper) {
			if (ch == 'D') options.detail = true;
			else if (ch == 'I') options.onlineOnly = true;
			else if (ch == 'M') options.sortByMoney = true;
			else return false;
		}
	}
	return true;
}

std::string EscapeAdminLineResponse(const std::string& text) {
	std::string escaped;
	escaped.reserve(text.size());
	for (const char ch : text) {
		if (ch == '\n') escaped += "\\n";
		else if (ch == '\t') escaped += "\\t";
		else if (ch != '\r') escaped.push_back(ch);
	}
	return escaped;
}

std::string OnlineText(bool online) {
	return online ? "ON" : "OFF";
}

void AppendBasicPlayerSeeLine(std::ostringstream& output, const PlayerSeeInfo& info,
	bool includeOnline, int index) {
	if (index > 0) output << index << ". ";
	output << "ID: " << info.playerId
		<< " | Nickname: " << (info.nickname.empty() ? "-" : info.nickname)
		<< " | Money: " << info.money;
	if (includeOnline) output << " | Online: " << OnlineText(info.isOnline);
	output << " | Active: " << OnlineText(info.isActive);
}

void AppendDetailedPlayerSeeLines(std::ostringstream& output, const PlayerSeeInfo& info,
	bool includeOnline, int index) {
	if (index > 0) output << "[" << index << "] ";
	output << "ID: " << info.playerId << '\n';
	output << "  Nickname: " << (info.nickname.empty() ? "-" : info.nickname) << '\n';
	output << "  Money: " << info.money << '\n';
	if (includeOnline) output << "  Online: " << OnlineText(info.isOnline) << '\n';
	output << "  Active: " << OnlineText(info.isActive) << '\n';
	if (info.savings.active) {
		output << "  Savings: true | SavingsID: " << info.savings.savingsId
			<< " | Start: " << info.savings.startTime
			<< " | End: " << info.savings.endTime << '\n';
	}
	else {
		output << "  Savings: false\n";
	}
	if (info.loan.active) {
		output << "  Loan: true | LoanID: " << info.loan.loanId
			<< " | Start: " << info.loan.startTime
			<< " | Repayment: " << info.loan.repaymentTime
			<< " | WaitingCollection: " << (info.loan.waitingForCollection ? "true" : "false");
	}
	else {
		output << "  Loan: false";
	}
}

std::string FormatSingleSeeResponse(const PlayerSeeInfo& info, const SeeCommandOptions& options) {
	std::ostringstream output;
	if (options.detail) {
		output << "OK Player Detail\n";
		AppendDetailedPlayerSeeLines(output, info, false, 0);
	}
	else {
		output << "OK Player\n";
		AppendBasicPlayerSeeLine(output, info, false, 0);
	}
	return EscapeAdminLineResponse(output.str());
}

std::string FormatAllSeeResponse(const std::vector<PlayerSeeInfo>& infos,
	const SeeCommandOptions& options) {
	std::ostringstream output;
	output << (options.detail ? "OK Players Detail" : "OK Players")
		<< " count=" << infos.size();
	for (std::size_t i = 0; i < infos.size(); ++i) {
		output << '\n';
		if (options.detail) {
			AppendDetailedPlayerSeeLines(output, infos[i], true, static_cast<int>(i + 1));
		}
		else {
			AppendBasicPlayerSeeLine(output, infos[i], true, static_cast<int>(i + 1));
		}
	}
	return EscapeAdminLineResponse(output.str());
}

std::string FormatStockSeeResponse(const std::vector<StockSeeInfo>& infos,
	const SeeCommandOptions& options) {
	std::ostringstream output;
	output << (options.detail ? "OK Stocks Detail" : "OK Stocks")
		<< " count=" << infos.size();
	for (std::size_t i = 0; i < infos.size(); ++i) {
		const StockSeeInfo& info = infos[i];
		const std::uint32_t saleableQuantity =
			(info.totalQuantity > kIssuerInitialStockQuantity)
			? (info.totalQuantity - kIssuerInitialStockQuantity) : info.totalQuantity;
		const std::uint32_t soldQuantity =
			(saleableQuantity > info.unsoldQuantity)
			? (saleableQuantity - info.unsoldQuantity) : 0;

		output << '\n' << (i + 1) << ". "
			<< "StockID: " << info.stockId
			<< " | Name: " << info.stockName
			<< " | Issuer: " << info.issuerId
			<< " | Price: " << info.currentPrice
			<< " | Sold: " << soldQuantity << "/" << saleableQuantity;

		if (!options.detail) continue;

		output << '\n' << "  RecentPrices:";
		if (info.recentPrices.empty()) {
			output << " none";
		}
		else {
			for (const StockPriceSeeInfo& price : info.recentPrices) {
				output << '\n'
					<< "    " << price.changedTime
					<< " | " << price.previousPrice << " -> " << price.newPrice
					<< " | Bought: " << price.boughtQuantity
					<< " | SoldPressure: " << price.soldQuantity;
			}
		}

		output << '\n' << "  TopHolders:";
		if (info.topHolders.empty()) {
			output << " none";
		}
		else {
			for (std::size_t holderIndex = 0; holderIndex < info.topHolders.size(); ++holderIndex) {
				const StockHolderSeeInfo& holder = info.topHolders[holderIndex];
				output << '\n'
					<< "    " << (holderIndex + 1) << ". "
					<< holder.playerId << ": " << holder.quantity;
			}
		}
	}
	return EscapeAdminLineResponse(output.str());
}

bool HasAuthenticatedSessions(const std::vector<ClientSession>& sessions) {
	for (const ClientSession& session : sessions) {
		if (session.authenticated && session.socket != INVALID_SOCKET) return true;
	}
	return false;
}

std::string ExecuteAdminCommand(const std::string& command, Database& database,
	std::vector<ClientSession>& sessions, std::mutex& sessionsMutex) {
	std::istringstream input(command);
	std::string commandName;
	input >> commandName;
	if (ToUpperAscii(commandName) == "SEE") {
		std::string playerId;
		input >> playerId;
		if (playerId.empty()) return "FAIL usage: see {playerId|_allplayer|_stock} [D] [I] [M]";

		SeeCommandOptions options;
		if (!ParseSeeOptions(input, options))
			return "FAIL invalid see option";

		std::string reason;
		if (playerId == "_stock") {
			std::vector<StockSeeInfo> infos;
			if (!database.LoadAllStockSeeInfos(options.detail, options.sortByMoney, infos, reason))
				return "FAIL " + reason;
			return FormatStockSeeResponse(infos, options);
		}

		if (playerId == "_allplayer") {
			std::vector<PlayerSeeInfo> infos;
			if (!database.LoadAllPlayerSeeInfos(options.detail, options.onlineOnly,
				options.sortByMoney, infos, reason))
				return "FAIL " + reason;
			return FormatAllSeeResponse(infos, options);
		}

		PlayerSeeInfo info;
		if (!database.LoadPlayerSeeInfo(playerId, options.detail, info, reason))
			return "FAIL " + reason;
		return FormatSingleSeeResponse(info, options);
	}
	if (ToUpperAscii(commandName) == "ACTIVE") {
		std::string playerId;
		std::string modeText;
		std::string extra;
		input >> playerId >> modeText >> extra;
		if (playerId.empty() || modeText.empty() || !extra.empty())
			return "FAIL usage: active {playerId} {on|off}";
		bool active = false;
		if (!ParseOnOffMode(modeText, active))
			return "FAIL invalid active mode";

		std::string reason;
		if (!database.SetPlayerActive(playerId, active, reason))
			return "FAIL " + reason;
		return "OK active " + playerId + " " + OnlineText(active);
	}
	if (ToUpperAscii(commandName) == "UPDATE") {
		std::string targetText;
		std::string extra;
		input >> targetText >> extra;
		if (ToUpperAscii(targetText) != "STOCK" || !extra.empty())
			return "FAIL usage: update stock";

		int updatedCount = 0;
		std::string reason;
		if (!database.UpdateStockPrices(true, updatedCount, reason))
			return "FAIL " + reason;
		if (updatedCount > 0)
			QueueStockManagementInfosForAuthenticatedSessions(database, sessions, sessionsMutex);
		return "OK update stock updated=" + std::to_string(updatedCount);
	}
	if (ToUpperAscii(commandName) == "DELETEPLAYER") {
		std::string playerId;
		std::string extra;
		input >> playerId >> extra;
		if (playerId.empty() || !extra.empty())
			return "FAIL usage: deleteplayer {playerId|_allplayer}";

		std::string reason;
		if (playerId == "_allplayer") {
			{
				std::lock_guard<std::mutex> lock(sessionsMutex);
				if (HasAuthenticatedSessions(sessions))
					return "FAIL players are online";
			}
			std::uint64_t deletedCount = 0;
			if (!database.DeleteAllPlayersIfOffline(deletedCount, reason))
				return "FAIL " + reason;
			return "OK deleteplayer _allplayer deleted=" + std::to_string(deletedCount);
		}

		{
			std::lock_guard<std::mutex> lock(sessionsMutex);
			for (const ClientSession& session : sessions) {
				if (!session.authenticated || session.playerId != playerId ||
					session.socket == INVALID_SOCKET) continue;
				return "FAIL player is online";
			}
		}

		if (!database.DeletePlayerIfOffline(playerId, reason))
			return "FAIL " + reason;
		return "OK deleteplayer " + playerId;
	}
	if (commandName == "addmoney") {
		std::string playerId;
		std::string deltaText;
		std::string extra;
		input >> playerId >> deltaText >> extra;
		if (playerId.empty() || deltaText.empty() || !extra.empty())
			return "FAIL usage: addmoney {playerId|_allplayer} {amount|-all}";

		std::int64_t delta = 0;
		const bool clearAllMoney = (deltaText == "-all");
		if (!clearAllMoney) {
			try {
				size_t parsed = 0;
				delta = std::stoll(deltaText, &parsed);
				if (parsed != deltaText.size()) return "FAIL invalid amount";
			}
			catch (...) {
				return "FAIL invalid amount";
			}
		}

		if (playerId == "_allplayer") {
			std::vector<std::string> playerIds;
			std::string reason;
			if (!database.LoadAllPlayerIds(playerIds, reason))
				return "FAIL " + reason;
			int applied = 0;
			int skipped = 0;
			std::vector<AdminMoneyChangeResult> moneyChanges;
			for (const std::string& targetPlayerId : playerIds) {
				std::uint64_t finalMoney = 0;
				std::int64_t actualDelta = delta;
				std::string playerReason;
				const bool success = clearAllMoney
					? database.TryClearOnlinePlayerMoney(targetPlayerId, actualDelta, finalMoney, playerReason)
					: database.TryAddMoneyToOnlinePlayer(targetPlayerId, actualDelta, finalMoney, playerReason);
				if (!success) {
					++skipped;
					continue;
				}
				moneyChanges.push_back({ targetPlayerId, actualDelta, finalMoney });
				++applied;
			}
			QueueAdminMoneyChanges(sessions, sessionsMutex, moneyChanges);
			return "OK addmoney _allplayer applied=" + std::to_string(applied) +
				" skipped=" + std::to_string(skipped);
		}
		else {
			std::lock_guard<std::mutex> lock(sessionsMutex);
			ClientSession* targetSession = nullptr;
			for (ClientSession& session : sessions) {
				if (!session.authenticated || session.playerId != playerId ||
					session.socket == INVALID_SOCKET) continue;
				targetSession = &session;
				break;
			}
			if (!targetSession) return "FAIL player is offline";

			std::uint64_t finalMoney = 0;
			std::int64_t actualDelta = delta;
			std::string reason;
			const bool success = clearAllMoney
				? database.TryClearOnlinePlayerMoney(playerId, actualDelta, finalMoney, reason)
				: database.TryAddMoneyToOnlinePlayer(playerId, actualDelta, finalMoney, reason);
			if (!success) return "FAIL " + reason;

			QueuePacket(*targetSession, MakeMoneyChangedPacket(actualDelta, finalMoney));

			return "OK " + playerId + " delta=" + std::to_string(actualDelta) +
				" money=" + std::to_string(finalMoney);
		}
	}
	if (ToUpperAscii(commandName) == "CLEARIL") {
		std::string playerId;
		std::string modeText;
		std::string extra;
		input >> playerId >> modeText >> extra;
		if (playerId.empty() || modeText.empty() || !extra.empty())
			return "FAIL usage: clearIL {playerId|_allplayer} {I|L|IL}";
		bool clearSavings = false;
		bool clearLoan = false;
		if (!ParseClearILMode(modeText, clearSavings, clearLoan))
			return "FAIL invalid clearIL mode";

		if (playerId == "_allplayer") {
			std::vector<FinancialClearResult> results;
			std::string reason;
			if (!database.ClearFinancialProductsForAll(clearSavings, clearLoan, results, reason))
				return "FAIL " + reason;
			QueueFinancialClearNotifications(sessions, sessionsMutex, results);
			int savingsCount = 0;
			int loanCount = 0;
			for (const FinancialClearResult& result : results) {
				if (result.savingsCleared) ++savingsCount;
				if (result.loanCleared) ++loanCount;
			}
			return "OK clearIL _allplayer savings=" + std::to_string(savingsCount) +
				" loans=" + std::to_string(loanCount);
		}

		FinancialClearResult result;
		std::string reason;
		if (!database.ClearFinancialProducts(playerId, clearSavings, clearLoan, result, reason))
			return "FAIL " + reason;
		QueueFinancialClearNotifications(sessions, sessionsMutex, std::vector<FinancialClearResult>{ result });
		return "OK clearIL " + playerId +
			" savings=" + std::to_string(result.savingsCleared ? 1 : 0) +
			" loans=" + std::to_string(result.loanCleared ? 1 : 0);
	}
	if (ToUpperAscii(commandName) == "DB") {
		std::string subCommand;
		std::string extra;
		input >> subCommand >> extra;
		if (ToUpperAscii(subCommand) != "CLEAR" || !extra.empty())
			return "FAIL usage: DB CLEAR";
		{
			std::lock_guard<std::mutex> lock(sessionsMutex);
			if (HasAuthenticatedSessions(sessions))
				return "FAIL players are online";
		}
		std::string reason;
		if (!database.ResetRuntimeStatePreservingAccounts(reason))
			return "FAIL " + reason;
		return "OK DB CLEAR";
	}
	if (commandName.empty()) return "FAIL empty command";
	return "FAIL unknown command";
}

void HandleAdminClient(SOCKET adminSocket, const std::string& addressText,
	std::vector<ClientSession>& sessions, std::mutex& sessionsMutex) {
	char header[sizeof(PacketHeader)]{};
	if (!ReceiveAll(adminSocket, header, sizeof(header))) {
		closesocket(adminSocket);
		return;
	}

	const std::uint16_t packetSize = ReadUInt16(header);
	const std::uint16_t packetType = ReadUInt16(header + 2);
	if (packetSize < sizeof(PacketHeader) || packetSize > kMaxPacketSize ||
		packetType != static_cast<std::uint16_t>(PacketType::AdminLoginRequest)) {
		closesocket(adminSocket);
		return;
	}

	std::vector<char> payload(packetSize - sizeof(PacketHeader));
	if (!ReceiveAll(adminSocket, payload.data(), static_cast<int>(payload.size()))) {
		closesocket(adminSocket);
		return;
	}

	AuthRequest request;
	AuthResult result = AuthResult::InvalidFormat;
	if (ParseAuthRequest(payload, request)) {
		result = (request.id == "admin" && request.password == "12345678")
			? AuthResult::Success : AuthResult::WrongPassword;
	}

	SendAll(adminSocket, MakePacketWithResult(PacketType::AdminLoginResponse, result));
	if (result != AuthResult::Success) {
		std::cout << "[Admin Login Failed] " << addressText << '\n';
		closesocket(adminSocket);
		return;
	}

	std::cout << "[Admin Connected] " << addressText << '\n';
	Database adminDatabase;
	if (!adminDatabase.Connect()) {
		SendTextLine(adminSocket, "FAIL admin database connection failed");
		closesocket(adminSocket);
		return;
	}

	std::array<char, 1024> buffer{};
	std::string commandBuffer;
	while (true) {
		const int received = recv(adminSocket, buffer.data(), static_cast<int>(buffer.size()), 0);
		if (received <= 0) break;
		commandBuffer.append(buffer.data(), buffer.data() + received);
		size_t newlinePosition = std::string::npos;
		while ((newlinePosition = commandBuffer.find('\n')) != std::string::npos) {
			std::string command = commandBuffer.substr(0, newlinePosition);
			if (!command.empty() && command.back() == '\r') command.pop_back();
			commandBuffer.erase(0, newlinePosition + 1);
			const std::string response = ExecuteAdminCommand(
				command, adminDatabase, sessions, sessionsMutex);
			std::cout << "[Admin Command] " << command << " -> " << response << '\n';
			if (!SendTextLine(adminSocket, response)) break;
		}
	}

	shutdown(adminSocket, SD_BOTH);
	closesocket(adminSocket);
	std::cout << "[Admin Disconnected] " << addressText << '\n';
}

void RunAdminServer(unsigned short adminPort, std::vector<ClientSession>& sessions,
	std::mutex& sessionsMutex) {
	const SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		std::cerr << "admin socket failed: " << WSAGetLastError() << '\n';
		return;
	}

	sockaddr_in serverAddress{};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(adminPort);

	if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddress),
		sizeof(serverAddress)) == SOCKET_ERROR) {
		std::cerr << "admin bind failed: " << WSAGetLastError() << '\n';
		closesocket(listenSocket);
		return;
	}

	if (listen(listenSocket, kListenBacklog) == SOCKET_ERROR) {
		std::cerr << "admin listen failed: " << WSAGetLastError() << '\n';
		closesocket(listenSocket);
		return;
	}

	std::cout << "RaisingPet admin server is listening on 0.0.0.0:" << adminPort << '\n';
	while (true) {
		sockaddr_in clientAddress{};
		int clientAddressLength = sizeof(clientAddress);
		const SOCKET adminSocket = accept(listenSocket,
			reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressLength);
		if (adminSocket == INVALID_SOCKET) {
			std::cerr << "admin accept failed: " << WSAGetLastError() << '\n';
			continue;
		}

		char addressBuffer[INET_ADDRSTRLEN]{};
		inet_ntop(AF_INET, &clientAddress.sin_addr, addressBuffer, sizeof(addressBuffer));
		const unsigned short clientPort = ntohs(clientAddress.sin_port);
		const std::string addressText = std::string(addressBuffer) + ':' + std::to_string(clientPort);
		std::thread(HandleAdminClient, adminSocket, addressText,
			std::ref(sessions), std::ref(sessionsMutex)).detach();
	}
}

