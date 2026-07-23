// client packet processing for RaisingPetServer.cpp
// This file is intentionally included by RaisingPetServer.cpp to preserve the original internal linkage.

void CloseSession(Database& database, ClientSession& session) {
	if (session.authenticated) database.SetPlayerOffline(session.playerId);
	shutdown(session.socket, SD_BOTH);
	closesocket(session.socket);
	std::cout << "[Disconnected] " << session.addressText << '\n';
	session.socket = INVALID_SOCKET;
}

void ProcessPacket(Database& database, ClientSession& session,
	PacketType packetType, const std::vector<char>& payload) {
	if (packetType == PacketType::NicknameSetupRequest) {
		AuthResult result = AuthResult::NotFound;
		std::string nickname;
		if (!session.authenticated) result = AuthResult::WrongPassword;
		else if (!ParseNicknameSetupRequest(payload, nickname)) result = AuthResult::InvalidFormat;
		else result = database.SetupPlayerNickname(session.playerId, nickname);
		QueuePacket(session, MakePacketWithResult(PacketType::NicknameSetupResponse, result));
		std::cout << "[Nickname] " << (session.authenticated ? session.playerId : "<unauthenticated>")
			<< " result=" << static_cast<int>(result) << '\n';
		return;
	}

	if (packetType == PacketType::MoneyUpdate) {
		if (!session.authenticated || payload.size() != sizeof(std::uint64_t)) return;
		const std::uint64_t money = ReadUInt64(payload.data());
		database.UpdatePlayerMoney(session.playerId, money);
		std::vector<FinancialMoneyChange> changes;
		if (database.ProcessDueFinancialProducts(session.playerId, changes))
			QueueFinancialChanges(session, changes);
		return;
	}

	if (packetType == PacketType::SavingsJoinRequest ||
		packetType == PacketType::LoanApplyRequest) {
		if (!session.authenticated) {
			FinancialApplicationResult result;
			result.result = FinancialResult::NotAuthenticated;
			QueuePacket(session, MakeFinancialApplicationResponse(
				packetType == PacketType::SavingsJoinRequest
				? PacketType::SavingsJoinResponse : PacketType::LoanApplyResponse,
				result));
			return;
		}
		if (payload.size() != sizeof(std::uint32_t)) {
			FinancialApplicationResult result;
			result.result = FinancialResult::InvalidRequest;
			QueuePacket(session, MakeFinancialApplicationResponse(
				packetType == PacketType::SavingsJoinRequest
				? PacketType::SavingsJoinResponse : PacketType::LoanApplyResponse,
				result));
			return;
		}
		const std::uint32_t productId = ReadUInt32(payload.data());
		const FinancialApplicationResult result =
			(packetType == PacketType::SavingsJoinRequest)
			? database.JoinSavings(session.playerId, productId)
			: database.ApplyLoan(session.playerId, productId);
		QueuePacket(session, MakeFinancialApplicationResponse(
			packetType == PacketType::SavingsJoinRequest
			? PacketType::SavingsJoinResponse : PacketType::LoanApplyResponse,
			result));
		std::cout << "[Financial] " << session.playerId
			<< ((packetType == PacketType::SavingsJoinRequest) ? " savings=" : " loan=")
			<< productId << " result=" << static_cast<int>(result.result) << '\n';
		return;
	}

	if (packetType == PacketType::StockIssueRequest) {
		if (!session.authenticated) {
			StockIssueApplicationResult result;
			result.result = StockIssueResult::NotAuthenticated;
			QueuePacket(session, MakeStockIssueResponse(result));
			return;
		}
		std::string stockName;
		if (!ParseStockIssueRequest(payload, stockName)) {
			StockIssueApplicationResult result;
			result.result = StockIssueResult::InvalidRequest;
			QueuePacket(session, MakeStockIssueResponse(result));
			return;
		}
		const StockIssueApplicationResult result =
			database.IssueStock(session.playerId, stockName);
		QueuePacket(session, MakeStockIssueResponse(result));
		if (result.result == StockIssueResult::Success ||
			result.result == StockIssueResult::AlreadyIssued) {
			StockManagementInfo stockInfo;
			std::string stockInfoFailureReason;
			if (database.LoadStockManagementInfo(session.playerId, stockInfo, stockInfoFailureReason))
				QueuePacket(session, MakeStockManagementInfoResponse(stockInfo));
			else
				std::cerr << "[StockManagementInfo] " << session.playerId
					<< " failed: " << stockInfoFailureReason << '\n';
		}
		std::cout << "[StockIssue] " << session.playerId
			<< " stockName=" << stockName
			<< " result=" << static_cast<int>(result.result)
			<< " stockId=" << result.stockId
			<< " money=" << result.finalMoney << '\n';
		return;
	}

	if (packetType == PacketType::StockManagementInfoRequest) {
		StockManagementInfo info;
		if (session.authenticated) {
			std::string reason;
			if (!database.LoadStockManagementInfo(session.playerId, info, reason))
				std::cerr << "[StockManagementInfo] " << session.playerId
					<< " failed: " << reason << '\n';
		}
		QueuePacket(session, MakeStockManagementInfoResponse(info));
		return;
	}
	if (packetType == PacketType::StockTransactionListRequest) {
		std::vector<StockTransactionInfo> infos;
		if (session.authenticated) {
			std::string reason;
			if (!database.LoadStockTransactionInfos(session.playerId, infos, reason))
				std::cerr << "[StockTransactionList] " << session.playerId
				<< " failed: " << reason << '\n';
		}
		QueuePacket(session, MakeStockTransactionListResponse(infos));
		return;
	}
	if (packetType == PacketType::StockTradeRequest) {
		StockTradeAction action = StockTradeAction::Buy;
		std::uint32_t stockId = 0;
		std::uint32_t quantity = 0;
		if (!session.authenticated) {
			StockTradeApplicationResult result;
			result.result = StockTradeResult::NotAuthenticated;
			QueuePacket(session, MakeStockTradeResponse(result));
			return;
		}
		if (!ParseStockTradeRequest(payload, action, stockId, quantity)) {
			StockTradeApplicationResult result;
			result.result = StockTradeResult::InvalidRequest;
			QueuePacket(session, MakeStockTradeResponse(result));
			return;
		}
		const StockTradeApplicationResult result =
			database.ApplyStockTrade(session.playerId, action, stockId, quantity);
		QueuePacket(session, MakeStockTradeResponse(result));
		if (result.result == StockTradeResult::Success) {
			std::vector<StockTransactionInfo> infos;
			std::string reason;
			if (database.LoadStockTransactionInfos(session.playerId, infos, reason))
				QueuePacket(session, MakeStockTransactionListResponse(infos));
			else
				std::cerr << "[StockTransactionList] " << session.playerId
				<< " failed after trade: " << reason << '\n';
		}
		std::cout << "[StockTrade] " << session.playerId
			<< " action=" << static_cast<int>(action)
			<< " stockId=" << stockId
			<< " quantity=" << quantity
			<< " result=" << static_cast<int>(result.result)
			<< " money=" << result.finalMoney << '\n';
		return;
	}

	AuthRequest request;
	if (!ParseAuthRequest(payload, request)) {
		const PacketType responseType = (packetType == PacketType::RegisterRequest)
			? PacketType::RegisterResponse : PacketType::LoginResponse;
		QueuePacket(session, MakeAuthResponse(responseType, AuthResult::InvalidFormat));
		return;
	}

	if (packetType == PacketType::RegisterRequest) {
		const AuthResult result = database.RegisterPlayer(request.id, request.password);
		QueuePacket(session, MakeAuthResponse(PacketType::RegisterResponse, result));
		std::cout << "[Register] " << request.id << " result=" << static_cast<int>(result) << '\n';
		return;
	}

	if (packetType == PacketType::LoginRequest) {
		const LoginPlayerResult loginResult = database.LoginPlayer(request.id, request.password);
		if (loginResult.result == AuthResult::Success) {
			session.authenticated = true;
			session.playerId = request.id;
		}
		QueuePacket(session, MakeLoginResponse(loginResult.result, loginResult.hasLoggedIn));
		if (loginResult.result == AuthResult::Success) {
			std::vector<FinancialMoneyChange> changes;
			if (database.ProcessDueFinancialProducts(session.playerId, changes))
				QueueFinancialChanges(session, changes);
			std::vector<FinancialActiveStatus> statuses;
			if (database.LoadActiveFinancialStatuses(session.playerId, statuses))
				QueueFinancialActiveStatuses(session, statuses);
			bool stockIssued = false;
			std::string issuedStockName;
			std::string stockStatusFailureReason;
			if (database.LoadIssuedStockStatus(session.playerId,
				stockIssued, issuedStockName, stockStatusFailureReason))
				QueuePacket(session, MakeStockIssueStatusPacket(stockIssued, issuedStockName));
			else
				std::cerr << "[StockIssueStatus] " << session.playerId
					<< " failed: " << stockStatusFailureReason << '\n';
			StockManagementInfo stockInfo;
			std::string stockInfoFailureReason;
			if (database.LoadStockManagementInfo(session.playerId, stockInfo, stockInfoFailureReason))
				QueuePacket(session, MakeStockManagementInfoResponse(stockInfo));
			else
				std::cerr << "[StockManagementInfo] " << session.playerId
					<< " failed: " << stockInfoFailureReason << '\n';
		}
		std::cout << "[Login] " << request.id << " result="
			<< static_cast<int>(loginResult.result)
			<< " hasLoggedIn=" << (loginResult.hasLoggedIn ? 1 : 0) << '\n';
		return;
	}
}

bool ProcessReceiveBuffer(Database& database, ClientSession& session) {
	while (session.receiveBuffer.size() >= sizeof(PacketHeader)) {
		const std::uint16_t packetSize = ReadUInt16(session.receiveBuffer.data());
		const std::uint16_t packetTypeValue = ReadUInt16(session.receiveBuffer.data() + 2);
		if (packetSize < sizeof(PacketHeader) || packetSize > kMaxPacketSize) return false;
		if (session.receiveBuffer.size() < packetSize) return true;

		std::vector<char> payload(session.receiveBuffer.begin() + sizeof(PacketHeader),
			session.receiveBuffer.begin() + packetSize);
		session.receiveBuffer.erase(session.receiveBuffer.begin(),
			session.receiveBuffer.begin() + packetSize);

		const PacketType packetType = static_cast<PacketType>(packetTypeValue);
		if (packetType != PacketType::RegisterRequest && packetType != PacketType::LoginRequest &&
			packetType != PacketType::NicknameSetupRequest &&
			packetType != PacketType::MoneyUpdate &&
			packetType != PacketType::SavingsJoinRequest &&
			packetType != PacketType::LoanApplyRequest &&
			packetType != PacketType::StockIssueRequest &&
			packetType != PacketType::StockManagementInfoRequest &&
			packetType != PacketType::StockTransactionListRequest &&
			packetType != PacketType::StockTradeRequest)
			return false;
		ProcessPacket(database, session, packetType, payload);
	}
	return true;
}

void RunStockPriceUpdateServer(std::vector<ClientSession>& sessions,
	std::mutex& sessionsMutex) {
	Database stockDatabase;
	if (!stockDatabase.Connect()) {
		std::cerr << "Stock price updater database connection failed.\n";
		return;
	}

	while (true) {
		const auto now = std::chrono::system_clock::now();
		const auto currentMinute =
			std::chrono::time_point_cast<std::chrono::minutes>(now);
		const auto minutesSinceEpoch =
			std::chrono::duration_cast<std::chrono::minutes>(currentMinute.time_since_epoch()).count();
		const auto minutesToNextUpdate = 10 - (minutesSinceEpoch % 10);
		std::this_thread::sleep_until(currentMinute + std::chrono::minutes(minutesToNextUpdate));

		int updatedCount = 0;
		std::string failureReason;
		if (stockDatabase.UpdateStockPrices(false, updatedCount, failureReason)) {
			if (updatedCount > 0) {
				std::cout << "[StockPriceUpdate] updated=" << updatedCount << '\n';
				QueueStockManagementInfosForAuthenticatedSessions(
					stockDatabase, sessions, sessionsMutex);
			}
		}
		else {
			std::cerr << "[StockPriceUpdate] failed: " << failureReason << '\n';
		}
	}
}
