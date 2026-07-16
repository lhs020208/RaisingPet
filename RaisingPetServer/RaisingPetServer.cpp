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
constexpr unsigned short kDefaultPort = 7777;
constexpr int kListenBacklog = SOMAXCONN;
constexpr std::uint16_t kMaxPacketSize = 1024;

constexpr const char* kDbHost = "127.0.0.1";
constexpr const char* kDbUser = "raisingpet_server";
constexpr const char* kDbPassword = "12345678";
constexpr const char* kDbName = "RaisingPet";
constexpr unsigned int kDbPort = 3306;
constexpr std::uint64_t kStockIssuancePrice = 50000;
constexpr std::uint64_t kInitialStockPrice = 100;
constexpr std::uint32_t kStockTotalQuantity = 1000;
constexpr std::uint32_t kIssuerInitialStockQuantity = 200;
constexpr std::uint32_t kInitialUnsoldStockQuantity = 800;
constexpr std::uint64_t kMinimumStockPrice = 10;

enum class PacketType : std::uint16_t {
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

enum class FinancialResult : std::uint8_t {
	Success = 0,
	InvalidRequest = 1,
	NotAuthenticated = 2,
	AlreadyActive = 3,
	ProductNotFound = 4,
	NotEnoughMoney = 5,
	DatabaseError = 6,
	MoneyLimitExceeded = 7,
};

enum class FinancialCategory : std::uint8_t {
	Savings = 0,
	Loan = 1,
};

enum class StockIssueResult : std::uint8_t {
	Success = 0,
	InvalidRequest = 1,
	NotAuthenticated = 2,
	AlreadyIssued = 3,
	NotEnoughMoney = 4,
	DatabaseError = 5,
};

struct FinancialApplicationResult {
	FinancialResult result = FinancialResult::DatabaseError;
	std::uint32_t productId = 0;
	std::uint32_t durationSeconds = 0;
	std::int64_t moneyDelta = 0;
	std::uint64_t finalMoney = 0;
};

struct FinancialMoneyChange {
	FinancialCategory category = FinancialCategory::Savings;
	std::uint32_t productId = 0;
	std::int64_t moneyDelta = 0;
	std::uint64_t finalMoney = 0;
	bool completed = false;
};

struct FinancialActiveStatus {
	FinancialCategory category = FinancialCategory::Savings;
	std::uint32_t productId = 0;
	std::uint32_t remainingSeconds = 0;
};

struct StockIssueApplicationResult {
	StockIssueResult result = StockIssueResult::DatabaseError;
	std::uint64_t finalMoney = 0;
	std::uint32_t stockId = 0;
};

struct FinancialClearResult {
	std::string playerId;
	bool playerFound = false;
	bool savingsCleared = false;
	bool loanCleared = false;
	std::uint32_t savingsId = 0;
	std::uint32_t loanId = 0;
};

struct AdminMoneyChangeResult {
	std::string playerId;
	std::int64_t delta = 0;
	std::uint64_t finalMoney = 0;
};

struct PlayerSavingsSeeInfo {
	bool active = false;
	std::uint32_t savingsId = 0;
	std::string startTime;
	std::string endTime;
};

struct PlayerLoanSeeInfo {
	bool active = false;
	std::uint32_t loanId = 0;
	std::string startTime;
	std::string repaymentTime;
	bool waitingForCollection = false;
};

struct PlayerSeeInfo {
	std::string playerId;
	std::uint64_t money = 0;
	bool isOnline = false;
	bool isActive = false;
	PlayerSavingsSeeInfo savings;
	PlayerLoanSeeInfo loan;
};

struct StockPriceSeeInfo {
	std::uint64_t previousPrice = 0;
	std::uint64_t newPrice = 0;
	std::uint64_t boughtQuantity = 0;
	std::uint64_t soldQuantity = 0;
	std::string changedTime;
};

struct StockHolderSeeInfo {
	std::string playerId;
	std::uint32_t quantity = 0;
};

struct StockSeeInfo {
	std::uint32_t stockId = 0;
	std::string stockName;
	std::string issuerId;
	std::uint64_t currentPrice = 0;
	std::uint32_t totalQuantity = 0;
	std::uint32_t unsoldQuantity = 0;
	std::vector<StockPriceSeeInfo> recentPrices;
	std::vector<StockHolderSeeInfo> topHolders;
};

struct StockManagementInfo {
	bool issued = false;
	std::string stockName;
	std::uint32_t soldQuantity = 0;
	std::uint32_t unsoldQuantity = 0;
	std::uint32_t saleableQuantity = kInitialUnsoldStockQuantity;
	std::uint64_t issuanceRevenue = 0;
	std::uint32_t recentTradeQuantity = 0;
	std::uint64_t currentPrice = kInitialStockPrice;
	std::uint64_t previousPrice = kInitialStockPrice;
	std::vector<StockPriceSeeInfo> recentPrices;
	StockHolderSeeInfo topHolders[3];
};

struct PacketHeader {
	std::uint16_t size = 0;
	std::uint16_t type = 0;
};

struct AuthRequest {
	std::string id;
	std::string password;
};

struct ClientSession {
	SOCKET socket = INVALID_SOCKET;
	std::string addressText;
	std::vector<char> receiveBuffer;
	std::vector<char> sendBuffer;
	bool authenticated = false;
	std::string playerId;
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
			"INSERT INTO `Player` (`PlayerID`, `Password`, `IsOnline`, `IsActive`, `Money`) VALUES ('" +
			escapedId + "', '" + escapedPassword + "', FALSE, FALSE, 0)";

		if (mysql_query(connection_, query.c_str()) == 0) return AuthResult::Success;
		if (mysql_errno(connection_) == ER_DUP_ENTRY) return AuthResult::AlreadyExists;

		std::cerr << "RegisterPlayer failed: " << mysql_error(connection_) << '\n';
		return AuthResult::DatabaseError;
	}

	AuthResult LoginPlayer(const std::string& id, const std::string& password) {
		if (!IsAccountTextValid(id, 32) || !IsAccountTextValid(password, 64))
			return AuthResult::InvalidFormat;

		const std::string escapedId = Escape(id);
		const std::string query =
			"SELECT `Password` FROM `Player` WHERE `PlayerID` = '" + escapedId + "' LIMIT 1";
		if (mysql_query(connection_, query.c_str()) != 0) {
			std::cerr << "LoginPlayer select failed: " << mysql_error(connection_) << '\n';
			return AuthResult::DatabaseError;
		}

		MYSQL_RES* result = mysql_store_result(connection_);
		if (!result) {
			std::cerr << "LoginPlayer store result failed: " << mysql_error(connection_) << '\n';
			return AuthResult::DatabaseError;
		}

		MYSQL_ROW row = mysql_fetch_row(result);
		if (!row) {
			mysql_free_result(result);
			return AuthResult::NotFound;
		}

		const std::string storedPassword = row[0] ? row[0] : "";
		mysql_free_result(result);
		if (storedPassword != password) return AuthResult::WrongPassword;

		const std::string update =
			"UPDATE `Player` SET `IsOnline` = TRUE WHERE `PlayerID` = '" + escapedId + "'";
		if (mysql_query(connection_, update.c_str()) != 0) {
			std::cerr << "LoginPlayer update failed: " << mysql_error(connection_) << '\n';
			return AuthResult::DatabaseError;
		}

		return AuthResult::Success;
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
			"SELECT `PlayerID`, `Money`, `IsOnline`, `IsActive` FROM `Player` WHERE `PlayerID` = '" +
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
		info.money = row[1] ? std::stoull(row[1]) : 0;
		info.isOnline = row[2] && std::string(row[2]) != "0";
		info.isActive = row[3] && std::string(row[3]) != "0";
		mysql_free_result(result);

		if (detail && !LoadSeeFinancialDetails(escapedId, info, failureReason))
			return false;
		return true;
	}

	bool LoadAllPlayerSeeInfos(bool detail, bool onlineOnly, bool sortByMoney,
		std::vector<PlayerSeeInfo>& infos, std::string& failureReason) {
		infos.clear();
		std::string query = "SELECT `PlayerID`, `Money`, `IsOnline`, `IsActive` FROM `Player`";
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
			info.money = row[1] ? std::stoull(row[1]) : 0;
			info.isOnline = row[2] && std::string(row[2]) != "0";
			info.isActive = row[3] && std::string(row[3]) != "0";
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

	MYSQL* connection_ = nullptr;
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

std::vector<char> MakePacketWithResult(PacketType type, AuthResult result) {
	std::vector<char> packet;
	packet.reserve(sizeof(PacketHeader) + 1);
	WriteUInt16(packet, static_cast<std::uint16_t>(sizeof(PacketHeader) + 1));
	WriteUInt16(packet, static_cast<std::uint16_t>(type));
	packet.push_back(static_cast<char>(result));
	return packet;
}

std::vector<char> MakeAuthResponse(PacketType type, AuthResult result) {
	return MakePacketWithResult(type, result);
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
	output << "ID: " << info.playerId << " | Money: " << info.money;
	if (includeOnline) output << " | Online: " << OnlineText(info.isOnline);
	output << " | Active: " << OnlineText(info.isActive);
}

void AppendDetailedPlayerSeeLines(std::ostringstream& output, const PlayerSeeInfo& info,
	bool includeOnline, int index) {
	if (index > 0) output << "[" << index << "] ";
	output << "ID: " << info.playerId << '\n';
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

void CloseSession(Database& database, ClientSession& session) {
	if (session.authenticated) database.SetPlayerOffline(session.playerId);
	shutdown(session.socket, SD_BOTH);
	closesocket(session.socket);
	std::cout << "[Disconnected] " << session.addressText << '\n';
	session.socket = INVALID_SOCKET;
}

void ProcessPacket(Database& database, ClientSession& session,
	PacketType packetType, const std::vector<char>& payload) {
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
		const AuthResult result = database.LoginPlayer(request.id, request.password);
		if (result == AuthResult::Success) {
			session.authenticated = true;
			session.playerId = request.id;
		}
		QueuePacket(session, MakeAuthResponse(PacketType::LoginResponse, result));
		if (result == AuthResult::Success) {
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
		std::cout << "[Login] " << request.id << " result=" << static_cast<int>(result) << '\n';
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
			packetType != PacketType::MoneyUpdate &&
			packetType != PacketType::SavingsJoinRequest &&
			packetType != PacketType::LoanApplyRequest &&
			packetType != PacketType::StockIssueRequest &&
			packetType != PacketType::StockManagementInfoRequest)
			return false;
		ProcessPacket(database, session, packetType, payload);
	}
	return true;
}

void RunStockPriceUpdateServer() {
	Database stockDatabase;
	if (!stockDatabase.Connect()) {
		std::cerr << "Stock price updater database connection failed.\n";
		return;
	}

	while (true) {
		const auto now = std::chrono::system_clock::now();
		const auto currentHour =
			std::chrono::time_point_cast<std::chrono::hours>(now);
		const auto nextHour = currentHour + std::chrono::hours(1);
		std::this_thread::sleep_until(nextHour);

		int updatedCount = 0;
		std::string failureReason;
		if (stockDatabase.UpdateStockPrices(false, updatedCount, failureReason)) {
			if (updatedCount > 0)
				std::cout << "[StockPriceUpdate] updated=" << updatedCount << '\n';
		}
		else {
			std::cerr << "[StockPriceUpdate] failed: " << failureReason << '\n';
		}
	}
}
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
	std::thread(RunStockPriceUpdateServer).detach();
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
