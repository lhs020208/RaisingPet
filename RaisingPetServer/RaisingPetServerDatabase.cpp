// database implementation for RaisingPetServer.cpp
// This file is intentionally included by RaisingPetServer.cpp to preserve the original internal linkage.

class Database {
public:
	Database() = default;
	~Database() { Disconnect(); }

	bool Connect() {
		connection_ = mysql_init(nullptr);
		if (!connection_) {
			std::cerr << "mysql_init failed\n";
			return false;
		}

		if (!mysql_real_connect(connection_, kDbHost, kDbUser, kDbPassword, kDbName,
			kDbPort, nullptr, 0)) {
			std::cerr << "MySQL connection failed: " << mysql_error(connection_) << '\n';
			Disconnect();
			return false;
		}
		if (mysql_set_character_set(connection_, "utf8mb4") != 0) {
			std::cerr << "mysql_set_character_set failed: " << mysql_error(connection_) << '\n';
			Disconnect();
			return false;
		}

		std::cout << "MySQL connected successfully.\n";
		if (mysql_query(connection_, "SELECT VERSION()") == 0) {
			MYSQL_RES* result = mysql_store_result(connection_);
			if (result) {
				MYSQL_ROW row = mysql_fetch_row(result);
				if (row && row[0]) std::cout << "MySQL version: " << row[0] << '\n';
				mysql_free_result(result);
			}
		}
		return true;
	}

	void Disconnect() {
		if (!connection_) return;
		mysql_close(connection_);
		connection_ = nullptr;
	}

	AuthResult RegisterPlayer(const std::string& id, const std::string& password) {
		if (!IsAccountTextValid(id, 32) || !IsAccountTextValid(password, 64))
			return AuthResult::InvalidFormat;

		const std::string escapedId = Escape(id);
		const std::string escapedPassword = Escape(password);
		const std::string query =
			"INSERT INTO `Player` (`PlayerID`, `Password`, `Nickname`, `HasLoggedIn`, "
			"`IsOnline`, `IsActive`, `Money`) VALUES ('" +
			escapedId + "', '" + escapedPassword + "', NULL, FALSE, FALSE, FALSE, 0)";

		if (mysql_query(connection_, query.c_str()) == 0) return AuthResult::Success;
		if (mysql_errno(connection_) == ER_DUP_ENTRY) return AuthResult::AlreadyExists;

		std::cerr << "RegisterPlayer failed: " << mysql_error(connection_) << '\n';
		return AuthResult::DatabaseError;
	}

	LoginPlayerResult LoginPlayer(const std::string& id, const std::string& password) {
		LoginPlayerResult loginResult;
		if (!IsAccountTextValid(id, 32) || !IsAccountTextValid(password, 64))
		{
			loginResult.result = AuthResult::InvalidFormat;
			return loginResult;
		}

		const std::string escapedId = Escape(id);
		const std::string query =
			"SELECT `Password`, `HasLoggedIn` FROM `Player` WHERE `PlayerID` = '" +
			escapedId + "' LIMIT 1";
		if (mysql_query(connection_, query.c_str()) != 0) {
			std::cerr << "LoginPlayer select failed: " << mysql_error(connection_) << '\n';
			loginResult.result = AuthResult::DatabaseError;
			return loginResult;
		}

		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			std::cerr << "LoginPlayer store result failed: " << mysql_error(connection_) << '\n';
			loginResult.result = AuthResult::DatabaseError;
			return loginResult;
		}

		MYSQL_ROW row = mysql_fetch_row(result);
		if (!row) {
			mysql_free_result(result);
			loginResult.result = AuthResult::NotFound;
			return loginResult;
		}

		const std::string storedPassword = row[0] ? row[0] : "";
		loginResult.hasLoggedIn = row[1] && std::atoi(row[1]) != 0;
		mysql_free_result(result);
		if (storedPassword != password) {
			loginResult.result = AuthResult::WrongPassword;
			return loginResult;
		}

		const std::string update =
			"UPDATE `Player` SET `IsOnline` = TRUE WHERE `PlayerID` = '" + escapedId + "'";
		if (mysql_query(connection_, update.c_str()) != 0) {
			std::cerr << "LoginPlayer update failed: " << mysql_error(connection_) << '\n';
			loginResult.result = AuthResult::DatabaseError;
			return loginResult;
		}

		loginResult.result = AuthResult::Success;
		return loginResult;
	}

	AuthResult SetupPlayerNickname(const std::string& id, const std::string& nickname) {
		if (!IsAccountTextValid(id, 32) || !IsUtf8DisplayTextValid(nickname, 96))
			return AuthResult::InvalidFormat;

		const std::string escapedId = Escape(id);
		const std::string escapedNickname = Escape(nickname);
		const std::string update =
			"UPDATE `Player` SET `Nickname` = '" + escapedNickname +
			"', `HasLoggedIn` = TRUE WHERE `PlayerID` = '" + escapedId + "'";

		if (mysql_query(connection_, update.c_str()) == 0)
		{
			if (mysql_affected_rows(connection_) == 1) return AuthResult::Success;
			return AuthResult::NotFound;
		}
		if (mysql_errno(connection_) == ER_DUP_ENTRY) return AuthResult::AlreadyExists;

		std::cerr << "SetupPlayerNickname failed: " << mysql_error(connection_) << '\n';
		return AuthResult::DatabaseError;
	}

	void SetPlayerOffline(const std::string& id) {
		if (!IsAccountTextValid(id, 32)) return;
		const std::string query =
			"UPDATE `Player` SET `IsOnline` = FALSE WHERE `PlayerID` = '" + Escape(id) + "'";
		if (mysql_query(connection_, query.c_str()) != 0)
			std::cerr << "SetPlayerOffline failed: " << mysql_error(connection_) << '\n';
	}

	void UpdatePlayerMoney(const std::string& id, std::uint64_t money) {
		if (!IsAccountTextValid(id, 32)) return;
		const std::string query =
			"UPDATE `Player` SET `Money` = " + std::to_string(money) +
			" WHERE `PlayerID` = '" + Escape(id) + "'";
		if (mysql_query(connection_, query.c_str()) != 0)
			std::cerr << "UpdatePlayerMoney failed: " << mysql_error(connection_) << '\n';
	}

	bool TryAddMoneyToOnlinePlayer(const std::string& id, std::int64_t delta,
		std::uint64_t& finalMoney, std::string& failureReason) {
		if (!IsAccountTextValid(id, 32)) {
			failureReason = "invalid player id";
			return false;
		}

		const std::string escapedId = Escape(id);
		if (mysql_query(connection_, "START TRANSACTION") != 0) {
			failureReason = std::string("failed to start transaction: ") + mysql_error(connection_);
			return false;
		}

		const std::string select =
			"SELECT `Money`, `IsOnline` FROM `Player` WHERE `PlayerID` = '" +
			escapedId + "' LIMIT 1 FOR UPDATE";
		if (mysql_query(connection_, select.c_str()) != 0) {
			failureReason = std::string("failed to select player: ") + mysql_error(connection_);
			mysql_query(connection_, "ROLLBACK");
			return false;
		}

		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read player: ") + mysql_error(connection_);
			mysql_query(connection_, "ROLLBACK");
			return false;
		}

		MYSQL_ROW row = mysql_fetch_row(result);
		if (!row) {
			mysql_free_result(result);
			failureReason = "player not found";
			mysql_query(connection_, "ROLLBACK");
			return false;
		}

		const std::uint64_t currentMoney = row[0] ? std::stoull(row[0]) : 0;
		const bool isOnline = row[1] && std::string(row[1]) != "0";
		mysql_free_result(result);

		if (!isOnline) {
			failureReason = "player is offline";
			mysql_query(connection_, "ROLLBACK");
			return false;
		}
		if (delta < 0 && currentMoney < static_cast<std::uint64_t>(-delta)) {
			failureReason = "not enough money";
			mysql_query(connection_, "ROLLBACK");
			return false;
		}
		if (delta > 0 && currentMoney > UINT_MAX - static_cast<std::uint64_t>(delta)) {
			failureReason = "money would exceed client limit";
			mysql_query(connection_, "ROLLBACK");
			return false;
		}

		finalMoney = (delta >= 0)
			? currentMoney + static_cast<std::uint64_t>(delta)
			: currentMoney - static_cast<std::uint64_t>(-delta);
		const std::string update =
			"UPDATE `Player` SET `Money` = " + std::to_string(finalMoney) +
			" WHERE `PlayerID` = '" + escapedId + "'";
		if (mysql_query(connection_, update.c_str()) != 0) {
			failureReason = std::string("failed to update money: ") + mysql_error(connection_);
			mysql_query(connection_, "ROLLBACK");
			return false;
		}
		if (mysql_query(connection_, "COMMIT") != 0) {
			failureReason = std::string("failed to commit: ") + mysql_error(connection_);
			mysql_query(connection_, "ROLLBACK");
			return false;
		}
		return true;
	}

	FinancialApplicationResult JoinSavings(const std::string& id, std::uint32_t savingsId) {
		FinancialApplicationResult application;
		application.productId = savingsId;
		if (!IsAccountTextValid(id, 32) || savingsId == 0)
			return WithResult(application, FinancialResult::InvalidRequest);

		const std::string escapedId = Escape(id);
		if (mysql_query(connection_, "START TRANSACTION") != 0)
			return WithDatabaseError(application, "JoinSavings start transaction");

		if (HasActiveRow("PlayerSavings", escapedId)) {
			mysql_query(connection_, "ROLLBACK");
			return WithResult(application, FinancialResult::AlreadyActive);
		}

		std::uint64_t depositMoney = 0;
		std::uint64_t returnMoney = 0;
		std::uint32_t duration = 0;
		if (!LoadSavingsProduct(savingsId, depositMoney, returnMoney, duration)) {
			mysql_query(connection_, "ROLLBACK");
			return WithResult(application, FinancialResult::ProductNotFound);
		}

		std::uint64_t currentMoney = 0;
		bool isOnline = false;
		if (!LoadPlayerMoneyForUpdate(escapedId, currentMoney, isOnline)) {
			mysql_query(connection_, "ROLLBACK");
			return WithDatabaseError(application, "JoinSavings select player");
		}
		if (!isOnline) {
			mysql_query(connection_, "ROLLBACK");
			return WithResult(application, FinancialResult::NotAuthenticated);
		}
		if (currentMoney < depositMoney) {
			mysql_query(connection_, "ROLLBACK");
			return WithResult(application, FinancialResult::NotEnoughMoney);
		}

		const std::uint64_t finalMoney = currentMoney - depositMoney;
		const std::string updateMoney =
			"UPDATE `Player` SET `Money` = " + std::to_string(finalMoney) +
			" WHERE `PlayerID` = '" + escapedId + "'";
		if (mysql_query(connection_, updateMoney.c_str()) != 0) {
			mysql_query(connection_, "ROLLBACK");
			return WithDatabaseError(application, "JoinSavings update money");
		}

		const std::string insertSavings =
			"INSERT INTO `PlayerSavings` (`PlayerID`, `SavingsID`, `StartTime`, `EndTime`) "
			"VALUES ('" + escapedId + "', " + std::to_string(savingsId) +
			", NOW(), DATE_ADD(NOW(), INTERVAL " + std::to_string(duration) + " SECOND))";
		if (mysql_query(connection_, insertSavings.c_str()) != 0) {
			mysql_query(connection_, "ROLLBACK");
			if (mysql_errno(connection_) == ER_DUP_ENTRY)
				return WithResult(application, FinancialResult::AlreadyActive);
			return WithDatabaseError(application, "JoinSavings insert");
		}

		if (mysql_query(connection_, "COMMIT") != 0) {
			mysql_query(connection_, "ROLLBACK");
			return WithDatabaseError(application, "JoinSavings commit");
		}

		application.result = FinancialResult::Success;
		application.durationSeconds = duration;
		application.moneyDelta = -static_cast<std::int64_t>(depositMoney);
		application.finalMoney = finalMoney;
		return application;
	}

	FinancialApplicationResult ApplyLoan(const std::string& id, std::uint32_t loanId) {
		FinancialApplicationResult application;
		application.productId = loanId;
		if (!IsAccountTextValid(id, 32) || loanId == 0)
			return WithResult(application, FinancialResult::InvalidRequest);

		const std::string escapedId = Escape(id);
		if (mysql_query(connection_, "START TRANSACTION") != 0)
			return WithDatabaseError(application, "ApplyLoan start transaction");

		if (HasActiveRow("PlayerLoan", escapedId)) {
			mysql_query(connection_, "ROLLBACK");
			return WithResult(application, FinancialResult::AlreadyActive);
		}

		std::uint64_t borrowMoney = 0;
		std::uint64_t repaymentMoney = 0;
		std::uint32_t duration = 0;
		if (!LoadLoanProduct(loanId, borrowMoney, repaymentMoney, duration)) {
			mysql_query(connection_, "ROLLBACK");
			return WithResult(application, FinancialResult::ProductNotFound);
		}

		std::uint64_t currentMoney = 0;
		bool isOnline = false;
		if (!LoadPlayerMoneyForUpdate(escapedId, currentMoney, isOnline)) {
			mysql_query(connection_, "ROLLBACK");
			return WithDatabaseError(application, "ApplyLoan select player");
		}
		if (!isOnline) {
			mysql_query(connection_, "ROLLBACK");
			return WithResult(application, FinancialResult::NotAuthenticated);
		}
		if (currentMoney > UINT_MAX - borrowMoney) {
			mysql_query(connection_, "ROLLBACK");
			return WithResult(application, FinancialResult::MoneyLimitExceeded);
		}

		const std::uint64_t finalMoney = currentMoney + borrowMoney;
		const std::string updateMoney =
			"UPDATE `Player` SET `Money` = " + std::to_string(finalMoney) +
			" WHERE `PlayerID` = '" + escapedId + "'";
		if (mysql_query(connection_, updateMoney.c_str()) != 0) {
			mysql_query(connection_, "ROLLBACK");
			return WithDatabaseError(application, "ApplyLoan update money");
		}

		const std::string insertLoan =
			"INSERT INTO `PlayerLoan` (`PlayerID`, `LoanID`, `StartTime`, `RepaymentTime`, `IsWaitingForCollection`) "
			"VALUES ('" + escapedId + "', " + std::to_string(loanId) +
			", NOW(), DATE_ADD(NOW(), INTERVAL " + std::to_string(duration) + " SECOND), FALSE)";
		if (mysql_query(connection_, insertLoan.c_str()) != 0) {
			mysql_query(connection_, "ROLLBACK");
			if (mysql_errno(connection_) == ER_DUP_ENTRY)
				return WithResult(application, FinancialResult::AlreadyActive);
			return WithDatabaseError(application, "ApplyLoan insert");
		}

		if (mysql_query(connection_, "COMMIT") != 0) {
			mysql_query(connection_, "ROLLBACK");
			return WithDatabaseError(application, "ApplyLoan commit");
		}

		application.result = FinancialResult::Success;
		application.durationSeconds = duration;
		application.moneyDelta = static_cast<std::int64_t>(borrowMoney);
		application.finalMoney = finalMoney;
		return application;
	}

	StockIssueApplicationResult IssueStock(const std::string& id,
		const std::string& stockName) {
		StockIssueApplicationResult application;
		if (!IsAccountTextValid(id, 32) || !IsStockNameTextValid(stockName))
			return WithStockIssueResult(application, StockIssueResult::InvalidRequest);

		const std::string escapedId = Escape(id);
		const std::string escapedStockName = Escape(stockName);
		if (mysql_query(connection_, "START TRANSACTION") != 0)
			return WithStockDatabaseError(application, "IssueStock start transaction");

		std::uint64_t currentMoney = 0;
		bool isOnline = false;
		if (!LoadPlayerMoneyForUpdate(escapedId, currentMoney, isOnline)) {
			mysql_query(connection_, "ROLLBACK");
			return WithStockDatabaseError(application, "IssueStock select player");
		}
		application.finalMoney = currentMoney;
		if (!isOnline) {
			mysql_query(connection_, "ROLLBACK");
			return WithStockIssueResult(application, StockIssueResult::NotAuthenticated);
		}

		if (HasIssuedStockForUpdate(escapedId)) {
			mysql_query(connection_, "ROLLBACK");
			return WithStockIssueResult(application, StockIssueResult::AlreadyIssued);
		}

		if (currentMoney < kStockIssuancePrice) {
			mysql_query(connection_, "ROLLBACK");
			return WithStockIssueResult(application, StockIssueResult::NotEnoughMoney);
		}

		const std::uint64_t finalMoney = currentMoney - kStockIssuancePrice;
		const std::string updateMoney =
			"UPDATE `Player` SET `Money` = " + std::to_string(finalMoney) +
			" WHERE `PlayerID` = '" + escapedId + "'";
		if (mysql_query(connection_, updateMoney.c_str()) != 0) {
			mysql_query(connection_, "ROLLBACK");
			return WithStockDatabaseError(application, "IssueStock update money");
		}

		const std::string insertStock =
			"INSERT INTO `Stock` (`StockName`, `IssuerID`, `CurrentPrice`, `TotalQuantity`, "
			"`UnsoldQuantity`, `PriceUpdatedTime`) VALUES ('" +
			escapedStockName + "', '" + escapedId + "', " +
			std::to_string(kInitialStockPrice) + ", " +
			std::to_string(kStockTotalQuantity) + ", " +
			std::to_string(kInitialUnsoldStockQuantity) + ", NOW())";
		if (mysql_query(connection_, insertStock.c_str()) != 0) {
			const unsigned int error = mysql_errno(connection_);
			mysql_query(connection_, "ROLLBACK");
			if (error == ER_DUP_ENTRY)
				return WithStockIssueResult(application, StockIssueResult::AlreadyIssued);
			return WithStockDatabaseError(application, "IssueStock insert stock");
		}

		const std::uint32_t stockId = static_cast<std::uint32_t>(mysql_insert_id(connection_));
		const std::string insertHolding =
			"INSERT INTO `StockHolding` (`PlayerID`, `StockID`, `Quantity`) VALUES ('" +
			escapedId + "', " + std::to_string(stockId) + ", " +
			std::to_string(kIssuerInitialStockQuantity) + ")";
		if (mysql_query(connection_, insertHolding.c_str()) != 0) {
			mysql_query(connection_, "ROLLBACK");
			return WithStockDatabaseError(application, "IssueStock insert holding");
		}

		const std::string insertPrice =
			"INSERT INTO `StockPrice` (`StockID`, `PreviousPrice`, `NewPrice`, "
			"`BoughtQuantity`, `SoldQuantity`, `ChangeReason`, `ChangedTime`) VALUES (" +
			std::to_string(stockId) + ", " + std::to_string(kInitialStockPrice) + ", " +
			std::to_string(kInitialStockPrice) + ", 0, 0, 'ISSUANCE', NOW())";
		if (mysql_query(connection_, insertPrice.c_str()) != 0) {
			mysql_query(connection_, "ROLLBACK");
			return WithStockDatabaseError(application, "IssueStock insert price");
		}

		if (mysql_query(connection_, "COMMIT") != 0) {
			mysql_query(connection_, "ROLLBACK");
			return WithStockDatabaseError(application, "IssueStock commit");
		}

		application.result = StockIssueResult::Success;
		application.finalMoney = finalMoney;
		application.stockId = stockId;
		return application;
	}

	bool UpdateStockPrices(bool forceUpdate, int& updatedCount, std::string& failureReason) {
		updatedCount = 0;
		if (mysql_query(connection_, "START TRANSACTION") != 0) {
			failureReason = std::string("failed to start stock price transaction: ") + mysql_error(connection_);
			return false;
		}

		std::string selectStocks =
			"SELECT s.`StockID`, s.`CurrentPrice`, p.`IsOnline`, p.`IsActive` "
			"FROM `Stock` s "
			"JOIN `Player` p ON p.`PlayerID` = s.`IssuerID` ";
		if (!forceUpdate)
			selectStocks += "WHERE s.`PriceUpdatedTime` < DATE_FORMAT(NOW(), '%Y-%m-%d %H:00:00') ";
		selectStocks += "FOR UPDATE";
		if (mysql_query(connection_, selectStocks.c_str()) != 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to select stocks for price update: ") + mysql_error(connection_);
			return false;
		}

		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to read stocks for price update: ") + mysql_error(connection_);
			return false;
		}

		struct StockPriceUpdateTarget {
			std::uint32_t stockId = 0;
			std::uint64_t currentPrice = 0;
			bool issuerOnline = false;
			bool issuerActive = false;
		};
		std::vector<StockPriceUpdateTarget> targets;
		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(result)) != nullptr) {
			StockPriceUpdateTarget target;
			target.stockId = row[0] ? static_cast<std::uint32_t>(std::stoul(row[0])) : 0;
			target.currentPrice = row[1] ? std::stoull(row[1]) : 0;
			target.issuerOnline = row[2] && std::string(row[2]) != "0";
			target.issuerActive = row[3] && std::string(row[3]) != "0";
			if (target.stockId != 0 && target.currentPrice != 0)
				targets.push_back(target);
		}
		mysql_free_result(result);

		static thread_local std::mt19937 randomEngine{ std::random_device{}() };
		std::uniform_real_distribution<double> baseDistribution(-2.0, 2.0);

		for (const StockPriceUpdateTarget& target : targets) {
			std::uint64_t boughtQuantity = 0;
			const std::string boughtQuery =
				"SELECT COALESCE(SUM(`Quantity`), 0) FROM `StockTrade` "
				"WHERE `StockID` = " + std::to_string(target.stockId) +
				" AND `TradeTime` >= DATE_SUB(NOW(), INTERVAL 1 HOUR) "
				"AND `TradeTime` <= NOW()";
			if (!QueryUInt64Scalar(boughtQuery, boughtQuantity)) {
				mysql_query(connection_, "ROLLBACK");
				failureReason = "failed to aggregate stock bought quantity";
				return false;
			}

			std::uint64_t soldPressureQuantity = 0;
			const std::string soldPressureQuery =
				"SELECT COALESCE(SUM(`Quantity`), 0) FROM `StockSale` "
				"WHERE `StockID` = " + std::to_string(target.stockId);
			if (!QueryUInt64Scalar(soldPressureQuery, soldPressureQuantity)) {
				mysql_query(connection_, "ROLLBACK");
				failureReason = "failed to aggregate stock sale pressure";
				return false;
			}

			const double baseRate = baseDistribution(randomEngine);
			double onlineBonusRate = 0.0;
			if (target.issuerOnline) {
				const double maxOnlineBonus = target.issuerActive ? 6.0 : 3.0;
				std::uniform_real_distribution<double> onlineDistribution(0.0, maxOnlineBonus);
				onlineBonusRate = onlineDistribution(randomEngine);
			}

			const std::uint64_t netBoughtQuantity =
				(boughtQuantity > soldPressureQuantity) ? (boughtQuantity - soldPressureQuantity) : 0;
			const std::uint64_t netSoldPressureQuantity =
				(soldPressureQuantity > boughtQuantity) ? (soldPressureQuantity - boughtQuantity) : 0;
			const double rawBuyBonusRate = static_cast<double>(netBoughtQuantity) / 20.0;
			const double buyBonusRate = (rawBuyBonusRate > 5.0) ? 5.0 : rawBuyBonusRate;
			const double rawSellPenaltyRate = static_cast<double>(netSoldPressureQuantity) / 50.0;
			const double sellPenaltyRate = (rawSellPenaltyRate > 4.0) ? 4.0 : rawSellPenaltyRate;
			double finalRate = baseRate + onlineBonusRate + buyBonusRate - sellPenaltyRate;
			if (finalRate < -5.0) finalRate = -5.0;
			else if (finalRate > 10.0) finalRate = 10.0;

			const double changedPrice =
				static_cast<double>(target.currentPrice) * (100.0 + finalRate) / 100.0;
			std::uint64_t newPrice = static_cast<std::uint64_t>(std::llround(changedPrice));
			if (newPrice < kMinimumStockPrice) newPrice = kMinimumStockPrice;

			std::ostringstream reason;
			reason.setf(std::ios::fixed);
			reason.precision(2);
			reason << "base=" << baseRate
				<< ";online=" << onlineBonusRate
				<< ";buy=" << buyBonusRate
				<< ";sell=-" << sellPenaltyRate
				<< ";final=" << finalRate;
			const std::string escapedReason = Escape(reason.str());

			const std::string updateStock =
				"UPDATE `Stock` SET `CurrentPrice` = " + std::to_string(newPrice) +
				", `PriceUpdatedTime` = NOW() "
				"WHERE `StockID` = " + std::to_string(target.stockId);
			if (mysql_query(connection_, updateStock.c_str()) != 0) {
				mysql_query(connection_, "ROLLBACK");
				failureReason = std::string("failed to update stock price: ") + mysql_error(connection_);
				return false;
			}

			const std::string insertPrice =
				"INSERT INTO `StockPrice` (`StockID`, `PreviousPrice`, `NewPrice`, "
				"`BoughtQuantity`, `SoldQuantity`, `ChangeReason`, `ChangedTime`) VALUES (" +
				std::to_string(target.stockId) + ", " +
				std::to_string(target.currentPrice) + ", " +
				std::to_string(newPrice) + ", " +
				std::to_string(boughtQuantity) + ", " +
				std::to_string(soldPressureQuantity) + ", '" +
				escapedReason + "', NOW())";
			if (mysql_query(connection_, insertPrice.c_str()) != 0) {
				mysql_query(connection_, "ROLLBACK");
				failureReason = std::string("failed to insert stock price history: ") + mysql_error(connection_);
				return false;
			}
			++updatedCount;
		}

		if (mysql_query(connection_, "COMMIT") != 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to commit stock price update: ") + mysql_error(connection_);
			return false;
		}
		return true;
	}

	bool ProcessDueFinancialProducts(const std::string& id,
		std::vector<FinancialMoneyChange>& changes) {
		if (!IsAccountTextValid(id, 32)) return false;
		const std::string escapedId = Escape(id);

		if (mysql_query(connection_, "START TRANSACTION") != 0) {
			std::cerr << "ProcessDueFinancialProducts start failed: " << mysql_error(connection_) << '\n';
			return false;
		}

		std::uint64_t currentMoney = 0;
		bool isOnline = false;
		if (!LoadPlayerMoneyForUpdate(escapedId, currentMoney, isOnline)) {
			mysql_query(connection_, "ROLLBACK");
			return false;
		}
		if (!isOnline) {
			mysql_query(connection_, "ROLLBACK");
			return true;
		}

		std::uint32_t savingsId = 0;
		std::uint64_t returnMoney = 0;
		if (LoadDueSavings(escapedId, savingsId, returnMoney)) {
			if (currentMoney <= UINT_MAX - returnMoney) {
				currentMoney += returnMoney;
				const std::string updateMoney =
					"UPDATE `Player` SET `Money` = " + std::to_string(currentMoney) +
					" WHERE `PlayerID` = '" + escapedId + "'";
				if (mysql_query(connection_, updateMoney.c_str()) != 0) {
					mysql_query(connection_, "ROLLBACK");
					return false;
				}
				const std::string deleteSavings =
					"DELETE FROM `PlayerSavings` WHERE `PlayerID` = '" + escapedId + "'";
				if (mysql_query(connection_, deleteSavings.c_str()) != 0) {
					mysql_query(connection_, "ROLLBACK");
					return false;
				}
				changes.push_back({ FinancialCategory::Savings, savingsId,
					static_cast<std::int64_t>(returnMoney), currentMoney, true });
			}
		}

		std::uint32_t loanId = 0;
		std::uint64_t repaymentMoney = 0;
		bool waitingForCollection = false;
		if (LoadDueOrWaitingLoan(escapedId, loanId, repaymentMoney, waitingForCollection)) {
			if (currentMoney >= repaymentMoney) {
				currentMoney -= repaymentMoney;
				const std::string updateMoney =
					"UPDATE `Player` SET `Money` = " + std::to_string(currentMoney) +
					" WHERE `PlayerID` = '" + escapedId + "'";
				if (mysql_query(connection_, updateMoney.c_str()) != 0) {
					mysql_query(connection_, "ROLLBACK");
					return false;
				}
				const std::string deleteLoan =
					"DELETE FROM `PlayerLoan` WHERE `PlayerID` = '" + escapedId + "'";
				if (mysql_query(connection_, deleteLoan.c_str()) != 0) {
					mysql_query(connection_, "ROLLBACK");
					return false;
				}
				changes.push_back({ FinancialCategory::Loan, loanId,
					-static_cast<std::int64_t>(repaymentMoney), currentMoney, true });
			}
			else if (!waitingForCollection) {
				const std::string markWaiting =
					"UPDATE `PlayerLoan` SET `IsWaitingForCollection` = TRUE "
					"WHERE `PlayerID` = '" + escapedId + "'";
				if (mysql_query(connection_, markWaiting.c_str()) != 0) {
					mysql_query(connection_, "ROLLBACK");
					return false;
				}
			}
		}

		if (mysql_query(connection_, "COMMIT") != 0) {
			mysql_query(connection_, "ROLLBACK");
			return false;
		}
		return true;
	}

	bool LoadActiveFinancialStatuses(const std::string& id,
		std::vector<FinancialActiveStatus>& statuses) {
		if (!IsAccountTextValid(id, 32)) return false;
		const std::string escapedId = Escape(id);
		if (!LoadActiveSavingsStatus(escapedId, statuses)) return false;
		if (!LoadActiveLoanStatus(escapedId, statuses)) return false;
		return true;
	}

	bool LoadAllPlayerIds(std::vector<std::string>& playerIds, std::string& failureReason) {
		playerIds.clear();
		if (mysql_query(connection_, "SELECT `PlayerID` FROM `Player`") != 0) {
			failureReason = std::string("failed to select players: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read players: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(result)) != nullptr) {
			if (row[0]) playerIds.push_back(row[0]);
		}
		mysql_free_result(result);
		return true;
	}

	bool LoadPlayerSeeInfo(const std::string& id, bool detail,
		PlayerSeeInfo& info, std::string& failureReason) {
		info = {};
		if (!IsAccountTextValid(id, 32)) {
			failureReason = "invalid player id";
			return false;
		}

		const std::string escapedId = Escape(id);
		const std::string query =
			"SELECT `PlayerID`, COALESCE(`Nickname`, ''), `Money`, `IsOnline`, `IsActive` "
			"FROM `Player` WHERE `PlayerID` = '" +
			escapedId + "' LIMIT 1";
		if (mysql_query(connection_, query.c_str()) != 0) {
			failureReason = std::string("failed to select player: ") + mysql_error(connection_);
			return false;
		}

		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read player: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_ROW row = mysql_fetch_row(result);
		if (!row) {
			mysql_free_result(result);
			failureReason = "player not found";
			return false;
		}
		info.playerId = row[0] ? row[0] : "";
		info.nickname = row[1] ? row[1] : "";
		info.money = row[2] ? std::stoull(row[2]) : 0;
		info.isOnline = row[3] && std::string(row[3]) != "0";
		info.isActive = row[4] && std::string(row[4]) != "0";
		mysql_free_result(result);

		if (detail && !LoadSeeFinancialDetails(escapedId, info, failureReason))
			return false;
		return true;
	}

	bool LoadAllPlayerSeeInfos(bool detail, bool onlineOnly, bool sortByMoney,
		std::vector<PlayerSeeInfo>& infos, std::string& failureReason) {
		infos.clear();
		std::string query = "SELECT `PlayerID`, COALESCE(`Nickname`, ''), `Money`, `IsOnline`, `IsActive` FROM `Player`";
		if (onlineOnly) query += " WHERE `IsOnline` = TRUE";
		if (sortByMoney) query += " ORDER BY `Money` DESC, `PlayerID` ASC";

		if (mysql_query(connection_, query.c_str()) != 0) {
			failureReason = std::string("failed to select players: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read players: ") + mysql_error(connection_);
			return false;
		}

		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(result)) != nullptr) {
			PlayerSeeInfo info;
			info.playerId = row[0] ? row[0] : "";
			info.nickname = row[1] ? row[1] : "";
			info.money = row[2] ? std::stoull(row[2]) : 0;
			info.isOnline = row[3] && std::string(row[3]) != "0";
			info.isActive = row[4] && std::string(row[4]) != "0";
			infos.push_back(info);
		}
		mysql_free_result(result);

		if (detail) {
			for (PlayerSeeInfo& info : infos) {
				const std::string escapedId = Escape(info.playerId);
				if (!LoadSeeFinancialDetails(escapedId, info, failureReason))
					return false;
			}
		}
		return true;
	}

	bool LoadAllStockSeeInfos(bool detail, bool sortByCurrentPrice,
		std::vector<StockSeeInfo>& infos, std::string& failureReason) {
		infos.clear();
		std::string query =
			"SELECT `StockID`, `StockName`, `IssuerID`, `CurrentPrice`, "
			"`TotalQuantity`, `UnsoldQuantity` FROM `Stock`";
		if (sortByCurrentPrice)
			query += " ORDER BY `CurrentPrice` DESC, `StockID` ASC";
		else
			query += " ORDER BY `StockID` ASC";

		if (mysql_query(connection_, query.c_str()) != 0) {
			failureReason = std::string("failed to select stocks: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read stocks: ") + mysql_error(connection_);
			return false;
		}

		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(result)) != nullptr) {
			StockSeeInfo info;
			info.stockId = row[0] ? static_cast<std::uint32_t>(std::stoul(row[0])) : 0;
			info.stockName = row[1] ? row[1] : "";
			info.issuerId = row[2] ? row[2] : "";
			info.currentPrice = row[3] ? std::stoull(row[3]) : 0;
			info.totalQuantity = row[4] ? static_cast<std::uint32_t>(std::stoul(row[4])) : 0;
			info.unsoldQuantity = row[5] ? static_cast<std::uint32_t>(std::stoul(row[5])) : 0;
			if (info.stockId != 0) infos.push_back(info);
		}
		mysql_free_result(result);

		if (detail) {
			for (StockSeeInfo& info : infos) {
				if (!LoadStockRecentPrices(info.stockId, info.recentPrices, failureReason))
					return false;
				if (!LoadStockTopHolders(info.stockId, info.topHolders, failureReason))
					return false;
			}
		}
		return true;
	}

	bool LoadIssuedStockStatus(const std::string& id, bool& issued,
		std::string& stockName, std::string& failureReason) {
		issued = false;
		stockName.clear();
		if (!IsAccountTextValid(id, 32)) {
			failureReason = "invalid player id";
			return false;
		}

		const std::string escapedId = Escape(id);
		const std::string query =
			"SELECT `StockName` FROM `Stock` WHERE `IssuerID` = '" + escapedId + "' LIMIT 1";
		if (mysql_query(connection_, query.c_str()) != 0) {
			failureReason = std::string("failed to select issued stock: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read issued stock: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_ROW row = mysql_fetch_row(result);
		if (row) {
			issued = true;
			stockName = row[0] ? row[0] : "";
		}
		mysql_free_result(result);
		return true;
	}

	bool LoadStockManagementInfo(const std::string& id,
		StockManagementInfo& info, std::string& failureReason) {
		info = {};
		info.saleableQuantity = kInitialUnsoldStockQuantity;
		if (!IsAccountTextValid(id, 32)) {
			failureReason = "invalid player id";
			return false;
		}

		const std::string escapedId = Escape(id);
		const std::string query =
			"SELECT `StockID`, `StockName`, `TotalQuantity`, `UnsoldQuantity` "
			"FROM `Stock` WHERE `IssuerID` = '" + escapedId + "' LIMIT 1";
		if (mysql_query(connection_, query.c_str()) != 0) {
			failureReason = std::string("failed to select stock management info: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read stock management info: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_ROW row = mysql_fetch_row(result);
		if (!row) {
			mysql_free_result(result);
			return true;
		}

		info.issued = true;
		const std::uint32_t stockId = row[0] ? static_cast<std::uint32_t>(std::stoul(row[0])) : 0;
		info.stockName = row[1] ? row[1] : "";
		const std::uint32_t totalQuantity = row[2] ? static_cast<std::uint32_t>(std::stoul(row[2])) : kStockTotalQuantity;
		info.unsoldQuantity = row[3] ? static_cast<std::uint32_t>(std::stoul(row[3])) : 0;
		mysql_free_result(result);

		info.saleableQuantity = (totalQuantity > kIssuerInitialStockQuantity)
			? (totalQuantity - kIssuerInitialStockQuantity) : totalQuantity;
		info.soldQuantity = (info.saleableQuantity > info.unsoldQuantity)
			? (info.saleableQuantity - info.unsoldQuantity) : 0;

		std::uint64_t revenue = 0;
		const std::string revenueQuery =
			"SELECT COALESCE(SUM(`Quantity` * `Price`), 0) FROM `StockTrade` "
			"WHERE `StockID` = " + std::to_string(stockId) + " AND `SellerID` IS NULL";
		if (!QueryUInt64Scalar(revenueQuery, revenue)) {
			failureReason = "failed to aggregate stock issuance revenue";
			return false;
		}
		info.issuanceRevenue = revenue;

		std::uint64_t recentQuantity = 0;
		const std::string recentQuery =
			"SELECT `Quantity` FROM `StockTrade` "
			"WHERE `StockID` = " + std::to_string(stockId) +
			" ORDER BY `TradeTime` DESC, `TradeID` DESC LIMIT 1";
		if (!QueryUInt64Scalar(recentQuery, recentQuantity)) {
			failureReason = "failed to load recent stock trade quantity";
			return false;
		}
		info.recentTradeQuantity = static_cast<std::uint32_t>(
			(recentQuantity > UINT_MAX) ? UINT_MAX : recentQuantity);

		const std::string priceQuery =
			"SELECT `PreviousPrice`, `NewPrice` FROM `StockPrice` "
			"WHERE `StockID` = " + std::to_string(stockId) +
			" ORDER BY `ChangedTime` DESC, `StockPriceID` DESC LIMIT 1";
		if (mysql_query(connection_, priceQuery.c_str()) != 0) {
			failureReason = std::string("failed to select stock price summary: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* priceResult = mysql_store_result(connection_);
		if (!priceResult) {
			failureReason = std::string("failed to read stock price summary: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_ROW priceRow = mysql_fetch_row(priceResult);
		if (priceRow) {
			info.previousPrice = priceRow[0] ? std::stoull(priceRow[0]) : kInitialStockPrice;
			info.currentPrice = priceRow[1] ? std::stoull(priceRow[1]) : kInitialStockPrice;
		}
		mysql_free_result(priceResult);

		std::vector<StockHolderSeeInfo> holders;
		if (!LoadStockTopHoldersIncludingSales(stockId, holders, failureReason))
			return false;
		for (size_t i = 0; i < holders.size() && i < 3; ++i)
			info.topHolders[i] = holders[i];

		if (!LoadStockRecentPrices(stockId, info.recentPrices, failureReason))
			return false;
		return true;
	}

	bool LoadStockTransactionInfos(const std::string& id,
		std::vector<StockTransactionInfo>& infos, std::string& failureReason) {
		infos.clear();
		if (!IsAccountTextValid(id, 32)) {
			failureReason = "invalid player id";
			return false;
		}

		const std::string escapedId = Escape(id);
		const std::string query =
			"SELECT `S`.`StockID`, `S`.`StockName`, `S`.`IssuerID`, "
			"COALESCE(NULLIF(`P`.`Nickname`, ''), `S`.`IssuerID`), `S`.`CurrentPrice`, "
			"COALESCE((SELECT `SP`.`PreviousPrice` FROM `StockPrice` `SP` "
			"WHERE `SP`.`StockID` = `S`.`StockID` "
			"ORDER BY `SP`.`ChangedTime` DESC, `SP`.`StockPriceID` DESC LIMIT 1), `S`.`CurrentPrice`), "
			"(`S`.`UnsoldQuantity` + COALESCE((SELECT SUM(`SS`.`Quantity`) FROM `StockSale` `SS` "
			"WHERE `SS`.`StockID` = `S`.`StockID`), 0)), "
			"COALESCE((SELECT `SH`.`Quantity` FROM `StockHolding` `SH` "
			"WHERE `SH`.`StockID` = `S`.`StockID` AND `SH`.`PlayerID` = '" + escapedId + "'), 0), "
			"COALESCE((SELECT `ST`.`Quantity` FROM `StockTrade` `ST` "
			"WHERE `ST`.`StockID` = `S`.`StockID` "
			"ORDER BY `ST`.`TradeTime` DESC, `ST`.`TradeID` DESC LIMIT 1), 0) "
			"FROM `Stock` `S` "
			"JOIN `Player` `P` ON `P`.`PlayerID` = `S`.`IssuerID` "
			"WHERE `S`.`IssuerID` <> '" + escapedId + "' "
			"ORDER BY `S`.`StockID` ASC";
		if (mysql_query(connection_, query.c_str()) != 0) {
			failureReason = std::string("failed to select stock transaction infos: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read stock transaction infos: ") + mysql_error(connection_);
			return false;
		}

		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(result)) != nullptr) {
			StockTransactionInfo info;
			info.stockId = row[0] ? static_cast<std::uint32_t>(std::stoul(row[0])) : 0;
			info.stockName = row[1] ? row[1] : "";
			info.issuerId = row[2] ? row[2] : "";
			info.issuerNickname = row[3] ? row[3] : info.issuerId;
			info.currentPrice = row[4] ? std::stoull(row[4]) : kInitialStockPrice;
			info.previousPrice = row[5] ? std::stoull(row[5]) : info.currentPrice;
			const std::uint64_t saleable = row[6] ? std::stoull(row[6]) : 0;
			const std::uint64_t myQuantity = row[7] ? std::stoull(row[7]) : 0;
			const std::uint64_t recentTrade = row[8] ? std::stoull(row[8]) : 0;
			info.saleableQuantity = static_cast<std::uint32_t>(
				(saleable > UINT_MAX) ? UINT_MAX : saleable);
			info.myQuantity = static_cast<std::uint32_t>(
				(myQuantity > UINT_MAX) ? UINT_MAX : myQuantity);
			info.recentTradeQuantity = static_cast<std::uint32_t>(
				(recentTrade > UINT_MAX) ? UINT_MAX : recentTrade);
			if (info.stockId != 0) {
				if (!LoadStockRecentPrices(info.stockId, info.recentPrices, failureReason)) {
					mysql_free_result(result);
					return false;
				}
				infos.push_back(info);
			}
		}
		mysql_free_result(result);
		return true;
	}

	StockTradeApplicationResult ApplyStockTrade(const std::string& id,
		StockTradeAction action, std::uint32_t stockId, std::uint32_t quantity) {
		StockTradeApplicationResult application;
		application.action = action;
		application.stockId = stockId;
		application.quantity = quantity;
		if (!IsAccountTextValid(id, 32) || stockId == 0 || quantity == 0)
			return WithStockTradeResult(application, StockTradeResult::InvalidRequest);

		const std::string escapedId = Escape(id);
		if (mysql_query(connection_, "START TRANSACTION") != 0)
			return WithStockTradeDatabaseError(application, "ApplyStockTrade start transaction");

		std::uint64_t currentMoney = 0;
		bool isOnline = false;
		if (!LoadPlayerMoneyForUpdate(escapedId, currentMoney, isOnline)) {
			mysql_query(connection_, "ROLLBACK");
			return WithStockTradeDatabaseError(application, "ApplyStockTrade select player");
		}
		application.finalMoney = currentMoney;
		if (!isOnline) {
			mysql_query(connection_, "ROLLBACK");
			return WithStockTradeResult(application, StockTradeResult::NotAuthenticated);
		}

		std::uint64_t currentPrice = 0;
		std::uint32_t unsoldQuantity = 0;
		std::string issuerId;
		if (!LoadStockForTradeUpdate(stockId, currentPrice, unsoldQuantity, issuerId)) {
			mysql_query(connection_, "ROLLBACK");
			return WithStockTradeResult(application, StockTradeResult::InvalidRequest);
		}
		if (issuerId == id) {
			mysql_query(connection_, "ROLLBACK");
			return WithStockTradeResult(application, StockTradeResult::InvalidRequest);
		}

		if (action == StockTradeAction::Buy) {
			const std::uint64_t totalPrice =
				currentPrice * static_cast<std::uint64_t>(quantity);
			if (currentMoney < totalPrice) {
				mysql_query(connection_, "ROLLBACK");
				return WithStockTradeResult(application, StockTradeResult::NotEnoughMoney);
			}
			if (!BuyStockForPlayer(escapedId, stockId, quantity, currentPrice,
				unsoldQuantity, totalPrice)) {
				mysql_query(connection_, "ROLLBACK");
				return WithStockTradeResult(application,
					m_lastStockTradeFailureIsSaleable_ ? StockTradeResult::NotEnoughSaleableStock
					: StockTradeResult::DatabaseError);
			}
			application.finalMoney = currentMoney - totalPrice;
		}
		else if (action == StockTradeAction::Sell) {
			if (!SellStockForPlayer(escapedId, stockId, quantity, currentPrice)) {
				mysql_query(connection_, "ROLLBACK");
				return WithStockTradeResult(application,
					m_lastStockTradeFailureIsStock_ ? StockTradeResult::NotEnoughStock
					: StockTradeResult::DatabaseError);
			}
			application.finalMoney = currentMoney;
		}
		else {
			mysql_query(connection_, "ROLLBACK");
			return WithStockTradeResult(application, StockTradeResult::InvalidRequest);
		}

		if (mysql_query(connection_, "COMMIT") != 0) {
			mysql_query(connection_, "ROLLBACK");
			return WithStockTradeDatabaseError(application, "ApplyStockTrade commit");
		}
		application.result = StockTradeResult::Success;
		return application;
	}

	bool ClearFinancialProducts(const std::string& id, bool clearSavings, bool clearLoan,
		FinancialClearResult& clearResult, std::string& failureReason) {
		clearResult = {};
		clearResult.playerId = id;
		if (!clearSavings && !clearLoan) {
			failureReason = "nothing to clear";
			return false;
		}
		if (!IsAccountTextValid(id, 32)) {
			failureReason = "invalid player id";
			return false;
		}

		const std::string escapedId = Escape(id);
		if (mysql_query(connection_, "START TRANSACTION") != 0) {
			failureReason = std::string("failed to start transaction: ") + mysql_error(connection_);
			return false;
		}

		if (!PlayerExistsForUpdate(escapedId)) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = "player not found";
			return false;
		}
		clearResult.playerFound = true;

		if (clearSavings && !LoadSavingsForClear(escapedId, clearResult.savingsId)) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = "failed to read savings";
			return false;
		}
		if (clearLoan && !LoadLoanForClear(escapedId, clearResult.loanId)) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = "failed to read loan";
			return false;
		}

		if (clearSavings && clearResult.savingsId != 0) {
			const std::string query =
				"DELETE FROM `PlayerSavings` WHERE `PlayerID` = '" + escapedId + "'";
			if (mysql_query(connection_, query.c_str()) != 0) {
				mysql_query(connection_, "ROLLBACK");
				failureReason = std::string("failed to delete savings: ") + mysql_error(connection_);
				return false;
			}
			clearResult.savingsCleared = mysql_affected_rows(connection_) > 0;
		}
		if (clearLoan && clearResult.loanId != 0) {
			const std::string query =
				"DELETE FROM `PlayerLoan` WHERE `PlayerID` = '" + escapedId + "'";
			if (mysql_query(connection_, query.c_str()) != 0) {
				mysql_query(connection_, "ROLLBACK");
				failureReason = std::string("failed to delete loan: ") + mysql_error(connection_);
				return false;
			}
			clearResult.loanCleared = mysql_affected_rows(connection_) > 0;
		}

		if (mysql_query(connection_, "COMMIT") != 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to commit: ") + mysql_error(connection_);
			return false;
		}

		if ((clearSavings && clearResult.savingsCleared) ||
			(clearLoan && clearResult.loanCleared))
			return true;
		failureReason = "player has no requested product";
		return false;
	}

	bool ClearFinancialProductsForAll(bool clearSavings, bool clearLoan,
		std::vector<FinancialClearResult>& clearResults, std::string& failureReason) {
		clearResults.clear();
		if (!clearSavings && !clearLoan) {
			failureReason = "nothing to clear";
			return false;
		}
		if (mysql_query(connection_, "START TRANSACTION") != 0) {
			failureReason = std::string("failed to start transaction: ") + mysql_error(connection_);
			return false;
		}

		if (clearSavings && !LoadAllSavingsForClear(clearResults)) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = "failed to read savings";
			return false;
		}
		if (clearLoan && !LoadAllLoansForClear(clearResults)) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = "failed to read loans";
			return false;
		}

		if (clearSavings &&
			mysql_query(connection_, "DELETE FROM `PlayerSavings`") != 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to delete savings: ") + mysql_error(connection_);
			return false;
		}
		if (clearLoan &&
			mysql_query(connection_, "DELETE FROM `PlayerLoan`") != 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to delete loans: ") + mysql_error(connection_);
			return false;
		}

		if (mysql_query(connection_, "COMMIT") != 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to commit: ") + mysql_error(connection_);
			return false;
		}
		for (FinancialClearResult& result : clearResults) {
			result.playerFound = true;
			result.savingsCleared = result.savingsId != 0;
			result.loanCleared = result.loanId != 0;
		}
		return true;
	}

	bool TryClearOnlinePlayerMoney(const std::string& id, std::int64_t& delta,
		std::uint64_t& finalMoney, std::string& failureReason) {
		if (!IsAccountTextValid(id, 32)) {
			failureReason = "invalid player id";
			return false;
		}

		const std::string escapedId = Escape(id);
		if (mysql_query(connection_, "START TRANSACTION") != 0) {
			failureReason = std::string("failed to start transaction: ") + mysql_error(connection_);
			return false;
		}

		std::uint64_t currentMoney = 0;
		bool isOnline = false;
		if (!LoadPlayerMoneyForUpdate(escapedId, currentMoney, isOnline)) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = "player not found";
			return false;
		}
		if (!isOnline) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = "player is offline";
			return false;
		}

		const std::string update =
			"UPDATE `Player` SET `Money` = 0 WHERE `PlayerID` = '" + escapedId + "'";
		if (mysql_query(connection_, update.c_str()) != 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to update money: ") + mysql_error(connection_);
			return false;
		}
		if (mysql_query(connection_, "COMMIT") != 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to commit: ") + mysql_error(connection_);
			return false;
		}
		delta = -static_cast<std::int64_t>(currentMoney);
		finalMoney = 0;
		return true;
	}

	bool ResetRuntimeStatePreservingAccounts(std::string& failureReason) {
		const char* queries[] = {
			"DELETE FROM `StockPrice`",
			"DELETE FROM `StockTrade`",
			"DELETE FROM `StockSale`",
			"DELETE FROM `StockHolding`",
			"DELETE FROM `Stock`",
			"DELETE FROM `PlayerLoan`",
			"DELETE FROM `PlayerSavings`",
			"UPDATE `Player` SET `Money` = 0, `IsOnline` = FALSE, `IsActive` = FALSE"
		};
		for (const char* query : queries) {
			if (mysql_query(connection_, query) == 0) continue;
			failureReason = std::string("failed query [") + query + "]: " + mysql_error(connection_);
			return false;
		}
		return true;
	}

	bool SetPlayerActive(const std::string& id, bool active, std::string& failureReason) {
		if (!IsAccountTextValid(id, 32)) {
			failureReason = "invalid player id";
			return false;
		}

		const std::string escapedId = Escape(id);
		const std::string existsQuery =
			"SELECT 1 FROM `Player` WHERE `PlayerID` = '" + escapedId + "' LIMIT 1";
		if (mysql_query(connection_, existsQuery.c_str()) != 0) {
			failureReason = std::string("failed to select player: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read player: ") + mysql_error(connection_);
			return false;
		}
		const bool playerFound = (mysql_fetch_row(result) != nullptr);
		mysql_free_result(result);
		if (!playerFound) {
			failureReason = "player not found";
			return false;
		}

		const std::string updateQuery =
			std::string("UPDATE `Player` SET `IsActive` = ") + (active ? "TRUE" : "FALSE") +
			" WHERE `PlayerID` = '" + escapedId + "'";
		if (mysql_query(connection_, updateQuery.c_str()) != 0) {
			failureReason = std::string("failed to update player activity: ") + mysql_error(connection_);
			return false;
		}
		return true;
	}

	bool DeletePlayerIfOffline(const std::string& id, std::string& failureReason) {
		if (!IsAccountTextValid(id, 32)) {
			failureReason = "invalid player id";
			return false;
		}

		const std::string escapedId = Escape(id);
		if (mysql_query(connection_, "START TRANSACTION") != 0) {
			failureReason = std::string("failed to start transaction: ") + mysql_error(connection_);
			return false;
		}

		if (!EnsurePlayerExistsAndOfflineForUpdate(escapedId, failureReason)) {
			mysql_query(connection_, "ROLLBACK");
			return false;
		}

		if (!DeletePlayerRuntimeRows(escapedId, failureReason)) {
			mysql_query(connection_, "ROLLBACK");
			return false;
		}

		const std::string deletePlayer =
			"DELETE FROM `Player` WHERE `PlayerID` = '" + escapedId + "'";
		if (mysql_query(connection_, deletePlayer.c_str()) != 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to delete player: ") + mysql_error(connection_);
			return false;
		}
		if (mysql_affected_rows(connection_) == 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = "player not found";
			return false;
		}

		if (mysql_query(connection_, "COMMIT") != 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to commit: ") + mysql_error(connection_);
			return false;
		}
		return true;
	}

	bool DeleteAllPlayersIfOffline(std::uint64_t& deletedCount, std::string& failureReason) {
		deletedCount = 0;
		if (mysql_query(connection_, "START TRANSACTION") != 0) {
			failureReason = std::string("failed to start transaction: ") + mysql_error(connection_);
			return false;
		}

		const char* onlineQuery =
			"SELECT `PlayerID` FROM `Player` WHERE `IsOnline` = TRUE LIMIT 1 FOR UPDATE";
		if (mysql_query(connection_, onlineQuery) != 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to check online players: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to read online players: ") + mysql_error(connection_);
			return false;
		}
		const bool hasOnlinePlayer = mysql_fetch_row(result) != nullptr;
		mysql_free_result(result);
		if (hasOnlinePlayer) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = "players are online";
			return false;
		}

		const char* queries[] = {
			"DELETE FROM `StockPrice`",
			"DELETE FROM `StockTrade`",
			"DELETE FROM `StockSale`",
			"DELETE FROM `StockHolding`",
			"DELETE FROM `Stock`",
			"DELETE FROM `PlayerLoan`",
			"DELETE FROM `PlayerSavings`"
		};
		for (const char* query : queries) {
			if (mysql_query(connection_, query) == 0) continue;
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed query [") + query + "]: " + mysql_error(connection_);
			return false;
		}

		if (mysql_query(connection_, "DELETE FROM `Player`") != 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to delete players: ") + mysql_error(connection_);
			return false;
		}
		deletedCount = static_cast<std::uint64_t>(mysql_affected_rows(connection_));

		if (mysql_query(connection_, "COMMIT") != 0) {
			mysql_query(connection_, "ROLLBACK");
			failureReason = std::string("failed to commit: ") + mysql_error(connection_);
			return false;
		}
		return true;
	}

private:
	static FinancialApplicationResult WithResult(FinancialApplicationResult result,
		FinancialResult financialResult) {
		result.result = financialResult;
		return result;
	}

	FinancialApplicationResult WithDatabaseError(FinancialApplicationResult result,
		const char* context) {
		std::cerr << context << " failed: " << mysql_error(connection_) << '\n';
		result.result = FinancialResult::DatabaseError;
		return result;
	}

	static StockIssueApplicationResult WithStockIssueResult(
		StockIssueApplicationResult result, StockIssueResult stockResult) {
		result.result = stockResult;
		return result;
	}

	StockIssueApplicationResult WithStockDatabaseError(StockIssueApplicationResult result,
		const char* context) {
		std::cerr << context << " failed: " << mysql_error(connection_) << '\n';
		result.result = StockIssueResult::DatabaseError;
		return result;
	}

	static StockTradeApplicationResult WithStockTradeResult(
		StockTradeApplicationResult result, StockTradeResult stockResult) {
		result.result = stockResult;
		return result;
	}

	StockTradeApplicationResult WithStockTradeDatabaseError(StockTradeApplicationResult result,
		const char* context) {
		std::cerr << context << " failed: " << mysql_error(connection_) << '\n';
		result.result = StockTradeResult::DatabaseError;
		return result;
	}

	static bool IsAccountTextValid(const std::string& text, std::size_t maxLength) {
		if (text.empty() || text.size() > maxLength) return false;
		for (const char ch : text) {
			if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
				(ch >= '0' && ch <= '9'))) return false;
		}
		return true;
	}

	static bool IsStockNameTextValid(const std::string& text) {
		if (text.empty() || text.size() > 150) return false;
		for (unsigned char ch : text) {
			if (ch < 0x20 || ch == 0x7F) return false;
		}
		return true;
	}

	bool HasActiveRow(const char* tableName, const std::string& escapedId) {
		const std::string query =
			std::string("SELECT 1 FROM `") + tableName + "` WHERE `PlayerID` = '" +
			escapedId + "' LIMIT 1 FOR UPDATE";
		if (mysql_query(connection_, query.c_str()) != 0) {
			std::cerr << "HasActiveRow failed: " << mysql_error(connection_) << '\n';
			return true;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return true;
		const bool exists = mysql_fetch_row(result) != nullptr;
		mysql_free_result(result);
		return exists;
	}

	bool HasIssuedStockForUpdate(const std::string& escapedId) {
		const std::string query =
			"SELECT `StockID` FROM `Stock` WHERE `IssuerID` = '" + escapedId +
			"' LIMIT 1 FOR UPDATE";
		if (mysql_query(connection_, query.c_str()) != 0) {
			std::cerr << "HasIssuedStockForUpdate failed: " << mysql_error(connection_) << '\n';
			return true;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return true;
		const bool exists = mysql_fetch_row(result) != nullptr;
		mysql_free_result(result);
		return exists;
	}

	bool QueryUInt64Scalar(const std::string& query, std::uint64_t& value) {
		value = 0;
		if (mysql_query(connection_, query.c_str()) != 0) {
			std::cerr << "QueryUInt64Scalar failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			std::cerr << "QueryUInt64Scalar store result failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		MYSQL_ROW row = mysql_fetch_row(result);
		if (row && row[0]) value = std::stoull(row[0]);
		mysql_free_result(result);
		return true;
	}

	bool LoadStockForTradeUpdate(std::uint32_t stockId, std::uint64_t& currentPrice,
		std::uint32_t& unsoldQuantity, std::string& issuerId) {
		const std::string query =
			"SELECT `CurrentPrice`, `UnsoldQuantity`, `IssuerID` FROM `Stock` "
			"WHERE `StockID` = " + std::to_string(stockId) + " LIMIT 1 FOR UPDATE";
		if (mysql_query(connection_, query.c_str()) != 0) {
			std::cerr << "LoadStockForTradeUpdate failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return false;
		MYSQL_ROW row = mysql_fetch_row(result);
		if (!row) {
			mysql_free_result(result);
			return false;
		}
		currentPrice = row[0] ? std::stoull(row[0]) : 0;
		unsoldQuantity = row[1] ? static_cast<std::uint32_t>(std::stoul(row[1])) : 0;
		issuerId = row[2] ? row[2] : "";
		mysql_free_result(result);
		return currentPrice > 0;
	}

	bool InsertStockTrade(std::uint32_t stockId, const std::string& escapedBuyerId,
		const std::string* escapedSellerId, std::uint32_t quantity, std::uint64_t price) {
		std::string query =
			"INSERT INTO `StockTrade` (`StockID`, `BuyerID`, `SellerID`, `Quantity`, `Price`, `TradeTime`) "
			"VALUES (" + std::to_string(stockId) + ", '" + escapedBuyerId + "', ";
		query += escapedSellerId ? ("'" + *escapedSellerId + "'") : "NULL";
		query += ", " + std::to_string(quantity) + ", " + std::to_string(price) + ", NOW())";
		if (mysql_query(connection_, query.c_str()) != 0) {
			std::cerr << "InsertStockTrade failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		return true;
	}

	bool BuyStockForPlayer(const std::string& escapedBuyerId, std::uint32_t stockId,
		std::uint32_t quantity, std::uint64_t currentPrice,
		std::uint32_t unsoldQuantity, std::uint64_t totalPrice) {
		m_lastStockTradeFailureIsSaleable_ = false;
		m_lastStockTradeFailureIsStock_ = false;

		struct SaleRow {
			std::uint64_t saleId = 0;
			std::string sellerId;
			std::uint32_t quantity = 0;
		};
		std::vector<SaleRow> saleRows;
		std::uint64_t saleQuantity = 0;
		const std::string saleQuery =
			"SELECT `SaleID`, `SellerID`, `Quantity` FROM `StockSale` "
			"WHERE `StockID` = " + std::to_string(stockId) +
			" AND `SellerID` <> '" + escapedBuyerId + "' "
			"ORDER BY `RegisteredTime` ASC, `SaleID` ASC FOR UPDATE";
		if (mysql_query(connection_, saleQuery.c_str()) != 0) {
			std::cerr << "BuyStockForPlayer select sale failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		MYSQL_RES* saleResult = mysql_store_result(connection_);
		if (!saleResult) return false;
		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(saleResult)) != nullptr) {
			SaleRow saleRow;
			saleRow.saleId = row[0] ? std::stoull(row[0]) : 0;
			saleRow.sellerId = row[1] ? row[1] : "";
			saleRow.quantity = row[2] ? static_cast<std::uint32_t>(std::stoul(row[2])) : 0;
			if (saleRow.saleId != 0 && !saleRow.sellerId.empty() && saleRow.quantity != 0) {
				saleRows.push_back(saleRow);
				saleQuantity += saleRow.quantity;
			}
		}
		mysql_free_result(saleResult);

		if (static_cast<std::uint64_t>(unsoldQuantity) + saleQuantity < quantity) {
			m_lastStockTradeFailureIsSaleable_ = true;
			return false;
		}

		const std::string updateBuyerMoney =
			"UPDATE `Player` SET `Money` = `Money` - " + std::to_string(totalPrice) +
			" WHERE `PlayerID` = '" + escapedBuyerId + "'";
		if (mysql_query(connection_, updateBuyerMoney.c_str()) != 0) {
			std::cerr << "BuyStockForPlayer update buyer money failed: " << mysql_error(connection_) << '\n';
			return false;
		}

		const std::string upsertHolding =
			"INSERT INTO `StockHolding` (`PlayerID`, `StockID`, `Quantity`) VALUES ('" +
			escapedBuyerId + "', " + std::to_string(stockId) + ", " + std::to_string(quantity) +
			") ON DUPLICATE KEY UPDATE `Quantity` = `Quantity` + VALUES(`Quantity`)";
		if (mysql_query(connection_, upsertHolding.c_str()) != 0) {
			std::cerr << "BuyStockForPlayer upsert buyer holding failed: " << mysql_error(connection_) << '\n';
			return false;
		}

		std::uint32_t remainingQuantity = quantity;
		const std::uint32_t fromUnsold = min(remainingQuantity, unsoldQuantity);
		if (fromUnsold > 0) {
			const std::string updateUnsold =
				"UPDATE `Stock` SET `UnsoldQuantity` = `UnsoldQuantity` - " +
				std::to_string(fromUnsold) + " WHERE `StockID` = " + std::to_string(stockId);
			if (mysql_query(connection_, updateUnsold.c_str()) != 0) {
				std::cerr << "BuyStockForPlayer update unsold failed: " << mysql_error(connection_) << '\n';
				return false;
			}
			if (!InsertStockTrade(stockId, escapedBuyerId, nullptr, fromUnsold, currentPrice))
				return false;
			remainingQuantity -= fromUnsold;
		}

		for (const SaleRow& saleRow : saleRows) {
			if (remainingQuantity == 0) break;
			const std::uint32_t boughtQuantity = min(remainingQuantity, saleRow.quantity);
			const std::string escapedSellerId = Escape(saleRow.sellerId);
			if (boughtQuantity == saleRow.quantity) {
				const std::string deleteSale =
					"DELETE FROM `StockSale` WHERE `SaleID` = " +
					std::to_string(saleRow.saleId);
				if (mysql_query(connection_, deleteSale.c_str()) != 0) {
					std::cerr << "BuyStockForPlayer delete sale failed: " << mysql_error(connection_) << '\n';
					return false;
				}
			}
			else {
				const std::string updateSale =
					"UPDATE `StockSale` SET `Quantity` = `Quantity` - " +
					std::to_string(boughtQuantity) + " WHERE `SaleID` = " +
					std::to_string(saleRow.saleId);
				if (mysql_query(connection_, updateSale.c_str()) != 0) {
					std::cerr << "BuyStockForPlayer update sale failed: " << mysql_error(connection_) << '\n';
					return false;
				}
			}
			const std::uint64_t sellerMoney = currentPrice * static_cast<std::uint64_t>(boughtQuantity);
			const std::string updateSellerMoney =
				"UPDATE `Player` SET `Money` = `Money` + " + std::to_string(sellerMoney) +
				" WHERE `PlayerID` = '" + escapedSellerId + "'";
			if (mysql_query(connection_, updateSellerMoney.c_str()) != 0) {
				std::cerr << "BuyStockForPlayer update seller money failed: " << mysql_error(connection_) << '\n';
				return false;
			}
			if (!InsertStockTrade(stockId, escapedBuyerId, &escapedSellerId,
				boughtQuantity, currentPrice))
				return false;
			remainingQuantity -= boughtQuantity;
		}
		return remainingQuantity == 0;
	}

	bool SellStockForPlayer(const std::string& escapedSellerId, std::uint32_t stockId,
		std::uint32_t quantity, std::uint64_t currentPrice) {
		(void)currentPrice;
		m_lastStockTradeFailureIsSaleable_ = false;
		m_lastStockTradeFailureIsStock_ = false;
		const std::string holdingQuery =
			"SELECT `Quantity` FROM `StockHolding` WHERE `PlayerID` = '" +
			escapedSellerId + "' AND `StockID` = " + std::to_string(stockId) +
			" LIMIT 1 FOR UPDATE";
		if (mysql_query(connection_, holdingQuery.c_str()) != 0) {
			std::cerr << "SellStockForPlayer select holding failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return false;
		MYSQL_ROW row = mysql_fetch_row(result);
		const std::uint32_t holdingQuantity =
			(row && row[0]) ? static_cast<std::uint32_t>(std::stoul(row[0])) : 0;
		mysql_free_result(result);
		if (holdingQuantity < quantity) {
			m_lastStockTradeFailureIsStock_ = true;
			return false;
		}

		if (holdingQuantity == quantity) {
			const std::string deleteHolding =
				"DELETE FROM `StockHolding` WHERE `PlayerID` = '" + escapedSellerId +
				"' AND `StockID` = " + std::to_string(stockId);
			if (mysql_query(connection_, deleteHolding.c_str()) != 0) {
				std::cerr << "SellStockForPlayer delete holding failed: " << mysql_error(connection_) << '\n';
				return false;
			}
		}
		else {
			const std::string updateHolding =
				"UPDATE `StockHolding` SET `Quantity` = `Quantity` - " +
				std::to_string(quantity) + " WHERE `PlayerID` = '" + escapedSellerId +
				"' AND `StockID` = " + std::to_string(stockId);
			if (mysql_query(connection_, updateHolding.c_str()) != 0) {
				std::cerr << "SellStockForPlayer update holding failed: " << mysql_error(connection_) << '\n';
				return false;
			}
		}

		const std::string insertSale =
			"INSERT INTO `StockSale` (`SellerID`, `StockID`, `Quantity`, `RegisteredTime`) "
			"VALUES ('" + escapedSellerId + "', " + std::to_string(stockId) + ", " +
			std::to_string(quantity) + ", NOW())";
		if (mysql_query(connection_, insertSale.c_str()) != 0) {
			std::cerr << "SellStockForPlayer insert sale failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		return true;
	}

	bool LoadStockRecentPrices(std::uint32_t stockId,
		std::vector<StockPriceSeeInfo>& prices, std::string& failureReason) {
		prices.clear();
		const std::string query =
			"SELECT `PreviousPrice`, `NewPrice`, `BoughtQuantity`, `SoldQuantity`, `ChangedTime` "
			"FROM `StockPrice` WHERE `StockID` = " + std::to_string(stockId) +
			" ORDER BY `ChangedTime` DESC, `StockPriceID` DESC LIMIT 10";
		if (mysql_query(connection_, query.c_str()) != 0) {
			failureReason = std::string("failed to select stock prices: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read stock prices: ") + mysql_error(connection_);
			return false;
		}

		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(result)) != nullptr) {
			StockPriceSeeInfo price;
			price.previousPrice = row[0] ? std::stoull(row[0]) : 0;
			price.newPrice = row[1] ? std::stoull(row[1]) : 0;
			price.boughtQuantity = row[2] ? std::stoull(row[2]) : 0;
			price.soldQuantity = row[3] ? std::stoull(row[3]) : 0;
			price.changedTime = row[4] ? row[4] : "";
			prices.push_back(price);
		}
		mysql_free_result(result);
		return true;
	}

	bool LoadStockTopHolders(std::uint32_t stockId,
		std::vector<StockHolderSeeInfo>& holders, std::string& failureReason) {
		holders.clear();
		const std::string query =
			"SELECT `PlayerID`, `Quantity` FROM `StockHolding` WHERE `StockID` = " +
			std::to_string(stockId) + " ORDER BY `Quantity` DESC, `PlayerID` ASC LIMIT 3";
		if (mysql_query(connection_, query.c_str()) != 0) {
			failureReason = std::string("failed to select stock holders: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read stock holders: ") + mysql_error(connection_);
			return false;
		}

		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(result)) != nullptr) {
			StockHolderSeeInfo holder;
			holder.playerId = row[0] ? row[0] : "";
			holder.quantity = row[1] ? static_cast<std::uint32_t>(std::stoul(row[1])) : 0;
			holders.push_back(holder);
		}
		mysql_free_result(result);
		return true;
	}

	bool LoadStockTopHoldersIncludingSales(std::uint32_t stockId,
		std::vector<StockHolderSeeInfo>& holders, std::string& failureReason) {
		holders.clear();
		const std::string query =
			"SELECT `PlayerID`, SUM(`Quantity`) AS `TotalQuantity` FROM ("
			"SELECT `PlayerID`, `Quantity` FROM `StockHolding` WHERE `StockID` = " +
			std::to_string(stockId) +
			" UNION ALL "
			"SELECT `SellerID` AS `PlayerID`, `Quantity` FROM `StockSale` WHERE `StockID` = " +
			std::to_string(stockId) +
			") holder GROUP BY `PlayerID` ORDER BY `TotalQuantity` DESC, `PlayerID` ASC LIMIT 3";
		if (mysql_query(connection_, query.c_str()) != 0) {
			failureReason = std::string("failed to select stock holders including sales: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read stock holders including sales: ") + mysql_error(connection_);
			return false;
		}

		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(result)) != nullptr) {
			StockHolderSeeInfo holder;
			holder.playerId = row[0] ? row[0] : "";
			holder.quantity = row[1] ? static_cast<std::uint32_t>(std::stoul(row[1])) : 0;
			holders.push_back(holder);
		}
		mysql_free_result(result);
		return true;
	}

	bool LoadPlayerMoneyForUpdate(const std::string& escapedId,
		std::uint64_t& money, bool& isOnline) {
		const std::string query =
			"SELECT `Money`, `IsOnline` FROM `Player` WHERE `PlayerID` = '" +
			escapedId + "' LIMIT 1 FOR UPDATE";
		if (mysql_query(connection_, query.c_str()) != 0) {
			std::cerr << "LoadPlayerMoneyForUpdate failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return false;
		MYSQL_ROW row = mysql_fetch_row(result);
		if (!row) {
			mysql_free_result(result);
			return false;
		}
		money = row[0] ? std::stoull(row[0]) : 0;
		isOnline = row[1] && std::string(row[1]) != "0";
		mysql_free_result(result);
		return true;
	}

	bool PlayerExistsForUpdate(const std::string& escapedId) {
		const std::string query =
			"SELECT 1 FROM `Player` WHERE `PlayerID` = '" + escapedId + "' LIMIT 1 FOR UPDATE";
		if (mysql_query(connection_, query.c_str()) != 0) {
			std::cerr << "PlayerExistsForUpdate failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return false;
		const bool exists = mysql_fetch_row(result) != nullptr;
		mysql_free_result(result);
		return exists;
	}

	bool EnsurePlayerExistsAndOfflineForUpdate(const std::string& escapedId,
		std::string& failureReason) {
		const std::string query =
			"SELECT `IsOnline` FROM `Player` WHERE `PlayerID` = '" +
			escapedId + "' LIMIT 1 FOR UPDATE";
		if (mysql_query(connection_, query.c_str()) != 0) {
			failureReason = std::string("failed to select player: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read player: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_ROW row = mysql_fetch_row(result);
		if (!row) {
			mysql_free_result(result);
			failureReason = "player not found";
			return false;
		}
		const bool isOnline = row[0] && std::string(row[0]) != "0";
		mysql_free_result(result);
		if (isOnline) {
			failureReason = "player is online";
			return false;
		}
		return true;
	}

	bool DeletePlayerRuntimeRows(const std::string& escapedId, std::string& failureReason) {
		const std::string issuedStockSubquery =
			"(SELECT `StockID` FROM `Stock` WHERE `IssuerID` = '" + escapedId + "')";
		const std::string queries[] = {
			"DELETE FROM `StockTrade` WHERE `BuyerID` = '" + escapedId +
				"' OR `SellerID` = '" + escapedId + "' OR `StockID` IN " + issuedStockSubquery,
			"DELETE FROM `StockSale` WHERE `SellerID` = '" + escapedId +
				"' OR `StockID` IN " + issuedStockSubquery,
			"DELETE FROM `StockHolding` WHERE `PlayerID` = '" + escapedId +
				"' OR `StockID` IN " + issuedStockSubquery,
			"DELETE FROM `StockPrice` WHERE `StockID` IN " + issuedStockSubquery,
			"DELETE FROM `Stock` WHERE `IssuerID` = '" + escapedId + "'",
			"DELETE FROM `PlayerLoan` WHERE `PlayerID` = '" + escapedId + "'",
			"DELETE FROM `PlayerSavings` WHERE `PlayerID` = '" + escapedId + "'"
		};
		for (const std::string& query : queries) {
			if (mysql_query(connection_, query.c_str()) == 0) continue;
			failureReason = std::string("failed query [") + query + "]: " + mysql_error(connection_);
			return false;
		}
		return true;
	}

	bool LoadSavingsForClear(const std::string& escapedId, std::uint32_t& savingsId) {
		savingsId = 0;
		const std::string query =
			"SELECT `SavingsID` FROM `PlayerSavings` WHERE `PlayerID` = '" +
			escapedId + "' LIMIT 1 FOR UPDATE";
		if (mysql_query(connection_, query.c_str()) != 0) {
			std::cerr << "LoadSavingsForClear failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return false;
		MYSQL_ROW row = mysql_fetch_row(result);
		if (row && row[0]) savingsId = static_cast<std::uint32_t>(std::stoul(row[0]));
		mysql_free_result(result);
		return true;
	}

	bool LoadLoanForClear(const std::string& escapedId, std::uint32_t& loanId) {
		loanId = 0;
		const std::string query =
			"SELECT `LoanID` FROM `PlayerLoan` WHERE `PlayerID` = '" +
			escapedId + "' LIMIT 1 FOR UPDATE";
		if (mysql_query(connection_, query.c_str()) != 0) {
			std::cerr << "LoadLoanForClear failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return false;
		MYSQL_ROW row = mysql_fetch_row(result);
		if (row && row[0]) loanId = static_cast<std::uint32_t>(std::stoul(row[0]));
		mysql_free_result(result);
		return true;
	}

	FinancialClearResult& FindOrCreateClearResult(
		std::vector<FinancialClearResult>& results, const std::string& playerId) {
		for (FinancialClearResult& result : results) {
			if (result.playerId == playerId) return result;
		}
		FinancialClearResult result;
		result.playerId = playerId;
		results.push_back(result);
		return results.back();
	}

	bool LoadAllSavingsForClear(std::vector<FinancialClearResult>& results) {
		const char* query = "SELECT `PlayerID`, `SavingsID` FROM `PlayerSavings` FOR UPDATE";
		if (mysql_query(connection_, query) != 0) {
			std::cerr << "LoadAllSavingsForClear failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		MYSQL_RES* resultSet = mysql_store_result(connection_);
		if (!resultSet) return false;
		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(resultSet)) != nullptr) {
			if (!row[0] || !row[1]) continue;
			FinancialClearResult& result = FindOrCreateClearResult(results, row[0]);
			result.savingsId = static_cast<std::uint32_t>(std::stoul(row[1]));
		}
		mysql_free_result(resultSet);
		return true;
	}

	bool LoadAllLoansForClear(std::vector<FinancialClearResult>& results) {
		const char* query = "SELECT `PlayerID`, `LoanID` FROM `PlayerLoan` FOR UPDATE";
		if (mysql_query(connection_, query) != 0) {
			std::cerr << "LoadAllLoansForClear failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		MYSQL_RES* resultSet = mysql_store_result(connection_);
		if (!resultSet) return false;
		MYSQL_ROW row = nullptr;
		while ((row = mysql_fetch_row(resultSet)) != nullptr) {
			if (!row[0] || !row[1]) continue;
			FinancialClearResult& result = FindOrCreateClearResult(results, row[0]);
			result.loanId = static_cast<std::uint32_t>(std::stoul(row[1]));
		}
		mysql_free_result(resultSet);
		return true;
	}

	bool LoadSeeFinancialDetails(const std::string& escapedId,
		PlayerSeeInfo& info, std::string& failureReason) {
		if (!LoadSeeSavings(escapedId, info.savings, failureReason)) return false;
		if (!LoadSeeLoan(escapedId, info.loan, failureReason)) return false;
		return true;
	}

	bool LoadSeeSavings(const std::string& escapedId,
		PlayerSavingsSeeInfo& savings, std::string& failureReason) {
		savings = {};
		const std::string query =
			"SELECT `SavingsID`, `StartTime`, `EndTime` FROM `PlayerSavings` "
			"WHERE `PlayerID` = '" + escapedId + "' LIMIT 1";
		if (mysql_query(connection_, query.c_str()) != 0) {
			failureReason = std::string("failed to select savings: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read savings: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_ROW row = mysql_fetch_row(result);
		if (row) {
			savings.active = true;
			savings.savingsId = row[0] ? static_cast<std::uint32_t>(std::stoul(row[0])) : 0;
			savings.startTime = row[1] ? row[1] : "";
			savings.endTime = row[2] ? row[2] : "";
		}
		mysql_free_result(result);
		return true;
	}

	bool LoadSeeLoan(const std::string& escapedId,
		PlayerLoanSeeInfo& loan, std::string& failureReason) {
		loan = {};
		const std::string query =
			"SELECT `LoanID`, `StartTime`, `RepaymentTime`, `IsWaitingForCollection` "
			"FROM `PlayerLoan` WHERE `PlayerID` = '" + escapedId + "' LIMIT 1";
		if (mysql_query(connection_, query.c_str()) != 0) {
			failureReason = std::string("failed to select loan: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			failureReason = std::string("failed to read loan: ") + mysql_error(connection_);
			return false;
		}
		MYSQL_ROW row = mysql_fetch_row(result);
		if (row) {
			loan.active = true;
			loan.loanId = row[0] ? static_cast<std::uint32_t>(std::stoul(row[0])) : 0;
			loan.startTime = row[1] ? row[1] : "";
			loan.repaymentTime = row[2] ? row[2] : "";
			loan.waitingForCollection = row[3] && std::string(row[3]) != "0";
		}
		mysql_free_result(result);
		return true;
	}

	bool LoadSavingsProduct(std::uint32_t savingsId, std::uint64_t& depositMoney,
		std::uint64_t& returnMoney, std::uint32_t& duration) {
		const std::string query =
			"SELECT `DepositMoney`, `ReturnMoney`, `Duration` FROM `InstallmentSavings` "
			"WHERE `SavingsID` = " + std::to_string(savingsId) + " LIMIT 1";
		if (mysql_query(connection_, query.c_str()) != 0) return false;
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return false;
		MYSQL_ROW row = mysql_fetch_row(result);
		if (!row) {
			mysql_free_result(result);
			return false;
		}
		depositMoney = row[0] ? std::stoull(row[0]) : 0;
		returnMoney = row[1] ? std::stoull(row[1]) : 0;
		duration = row[2] ? static_cast<std::uint32_t>(std::stoul(row[2])) : 0;
		mysql_free_result(result);
		return true;
	}

	bool LoadLoanProduct(std::uint32_t loanId, std::uint64_t& borrowMoney,
		std::uint64_t& repaymentMoney, std::uint32_t& duration) {
		const std::string query =
			"SELECT `BorrowMoney`, `RepaymentMoney`, `Duration` FROM `Loan` "
			"WHERE `LoanID` = " + std::to_string(loanId) + " LIMIT 1";
		if (mysql_query(connection_, query.c_str()) != 0) return false;
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return false;
		MYSQL_ROW row = mysql_fetch_row(result);
		if (!row) {
			mysql_free_result(result);
			return false;
		}
		borrowMoney = row[0] ? std::stoull(row[0]) : 0;
		repaymentMoney = row[1] ? std::stoull(row[1]) : 0;
		duration = row[2] ? static_cast<std::uint32_t>(std::stoul(row[2])) : 0;
		mysql_free_result(result);
		return true;
	}

	bool LoadDueSavings(const std::string& escapedId, std::uint32_t& savingsId,
		std::uint64_t& returnMoney) {
		const std::string query =
			"SELECT ps.`SavingsID`, s.`ReturnMoney` FROM `PlayerSavings` ps "
			"JOIN `InstallmentSavings` s ON s.`SavingsID` = ps.`SavingsID` "
			"WHERE ps.`PlayerID` = '" + escapedId + "' AND ps.`EndTime` <= NOW() "
			"LIMIT 1 FOR UPDATE";
		if (mysql_query(connection_, query.c_str()) != 0) return false;
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return false;
		MYSQL_ROW row = mysql_fetch_row(result);
		if (!row) {
			mysql_free_result(result);
			return false;
		}
		savingsId = row[0] ? static_cast<std::uint32_t>(std::stoul(row[0])) : 0;
		returnMoney = row[1] ? std::stoull(row[1]) : 0;
		mysql_free_result(result);
		return true;
	}

	bool LoadDueOrWaitingLoan(const std::string& escapedId, std::uint32_t& loanId,
		std::uint64_t& repaymentMoney, bool& waitingForCollection) {
		const std::string query =
			"SELECT pl.`LoanID`, l.`RepaymentMoney`, pl.`IsWaitingForCollection` "
			"FROM `PlayerLoan` pl JOIN `Loan` l ON l.`LoanID` = pl.`LoanID` "
			"WHERE pl.`PlayerID` = '" + escapedId + "' "
			"AND (pl.`RepaymentTime` <= NOW() OR pl.`IsWaitingForCollection` = TRUE) "
			"LIMIT 1 FOR UPDATE";
		if (mysql_query(connection_, query.c_str()) != 0) return false;
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return false;
		MYSQL_ROW row = mysql_fetch_row(result);
		if (!row) {
			mysql_free_result(result);
			return false;
		}
		loanId = row[0] ? static_cast<std::uint32_t>(std::stoul(row[0])) : 0;
		repaymentMoney = row[1] ? std::stoull(row[1]) : 0;
		waitingForCollection = row[2] && std::string(row[2]) != "0";
		mysql_free_result(result);
		return true;
	}

	bool LoadActiveSavingsStatus(const std::string& escapedId,
		std::vector<FinancialActiveStatus>& statuses) {
		const std::string query =
			"SELECT `SavingsID`, GREATEST(TIMESTAMPDIFF(SECOND, NOW(), `EndTime`), 0) "
			"FROM `PlayerSavings` WHERE `PlayerID` = '" + escapedId + "' LIMIT 1";
		if (mysql_query(connection_, query.c_str()) != 0) {
			std::cerr << "LoadActiveSavingsStatus failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return false;
		MYSQL_ROW row = mysql_fetch_row(result);
		if (row) {
			statuses.push_back({ FinancialCategory::Savings,
				row[0] ? static_cast<std::uint32_t>(std::stoul(row[0])) : 0,
				row[1] ? static_cast<std::uint32_t>(std::stoul(row[1])) : 0 });
		}
		mysql_free_result(result);
		return true;
	}

	bool LoadActiveLoanStatus(const std::string& escapedId,
		std::vector<FinancialActiveStatus>& statuses) {
		const std::string query =
			"SELECT `LoanID`, GREATEST(TIMESTAMPDIFF(SECOND, NOW(), `RepaymentTime`), 0) "
			"FROM `PlayerLoan` WHERE `PlayerID` = '" + escapedId + "' LIMIT 1";
		if (mysql_query(connection_, query.c_str()) != 0) {
			std::cerr << "LoadActiveLoanStatus failed: " << mysql_error(connection_) << '\n';
			return false;
		}
		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) return false;
		MYSQL_ROW row = mysql_fetch_row(result);
		if (row) {
			statuses.push_back({ FinancialCategory::Loan,
				row[0] ? static_cast<std::uint32_t>(std::stoul(row[0])) : 0,
				row[1] ? static_cast<std::uint32_t>(std::stoul(row[1])) : 0 });
		}
		mysql_free_result(result);
		return true;
	}

	std::string Escape(const std::string& text) {
		std::string escaped(text.size() * 2 + 1, '\0');
		const unsigned long length = mysql_real_escape_string(
			connection_, &escaped[0], text.data(), static_cast<unsigned long>(text.size()));
		escaped.resize(length);
		return escaped;
	}

	bool m_lastStockTradeFailureIsSaleable_ = false;
	bool m_lastStockTradeFailureIsStock_ = false;
	MYSQL* connection_ = nullptr;
};

