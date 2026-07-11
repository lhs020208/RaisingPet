#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mysql.h>
#include <errmsg.h>
#include <mysqld_error.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
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
			"INSERT INTO `Player` (`PlayerID`, `Password`, `IsOnline`, `Money`) VALUES ('" +
			escapedId + "', '" + escapedPassword + "', FALSE, 0)";

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

	static bool IsAccountTextValid(const std::string& text, std::size_t maxLength) {
		if (text.empty() || text.size() > maxLength) return false;
		for (const char ch : text) {
			if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
				(ch >= '0' && ch <= '9'))) return false;
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

std::string ExecuteAdminCommand(const std::string& command, Database& database,
	std::vector<ClientSession>& sessions, std::mutex& sessionsMutex) {
	std::istringstream input(command);
	std::string commandName;
	input >> commandName;
	if (commandName == "addmoney") {
		std::string playerId;
		std::string deltaText;
		std::string extra;
		input >> playerId >> deltaText >> extra;
		if (playerId.empty() || deltaText.empty() || !extra.empty())
			return "FAIL usage: addmoney {playerId} {amount}";

		std::int64_t delta = 0;
		try {
			size_t parsed = 0;
			delta = std::stoll(deltaText, &parsed);
			if (parsed != deltaText.size()) return "FAIL invalid amount";
		}
		catch (...) {
			return "FAIL invalid amount";
		}

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
		std::string reason;
		if (!database.TryAddMoneyToOnlinePlayer(playerId, delta, finalMoney, reason))
			return "FAIL " + reason;

		QueuePacket(*targetSession, MakeMoneyChangedPacket(delta, finalMoney));

		return "OK " + playerId + " delta=" + std::to_string(delta) +
			" money=" + std::to_string(finalMoney);
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
			packetType != PacketType::LoanApplyRequest)
			return false;
		ProcessPacket(database, session, packetType, payload);
	}
	return true;
}
} // namespace

int main(int argc, char* argv[]) {
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
