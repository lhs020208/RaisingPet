#pragma once

// Internal types, constants, and small RAII helpers for RaisingPetServer.cpp.
// This header is included inside the server anonymous namespace.

constexpr unsigned short kDefaultPort = 7777;
constexpr int kListenBacklog = SOMAXCONN;
constexpr std::uint16_t kMaxPacketSize = 16384;

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
	StockTransactionListRequest = 18,
	StockTransactionListResponse = 19,
	StockTradeRequest = 20,
	StockTradeResponse = 21,
	NicknameSetupRequest = 22,
	NicknameSetupResponse = 23,
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

enum class StockTradeAction : std::uint8_t {
	Buy = 0,
	Sell = 1,
};

enum class StockTradeResult : std::uint8_t {
	Success = 0,
	InvalidRequest = 1,
	NotAuthenticated = 2,
	NotEnoughMoney = 3,
	NotEnoughStock = 4,
	NotEnoughSaleableStock = 5,
	DatabaseError = 6,
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

struct StockTradeApplicationResult {
	StockTradeAction action = StockTradeAction::Buy;
	StockTradeResult result = StockTradeResult::DatabaseError;
	std::uint32_t stockId = 0;
	std::uint32_t quantity = 0;
	std::uint64_t finalMoney = 0;
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
	std::string nickname;
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

struct StockTransactionInfo {
	std::uint32_t stockId = 0;
	std::string stockName;
	std::string issuerId;
	std::string issuerNickname;
	std::uint64_t currentPrice = kInitialStockPrice;
	std::uint64_t previousPrice = kInitialStockPrice;
	std::uint32_t saleableQuantity = 0;
	std::uint32_t myQuantity = 0;
	std::uint32_t recentTradeQuantity = 0;
	std::vector<StockPriceSeeInfo> recentPrices;
};

struct PacketHeader {
	std::uint16_t size = 0;
	std::uint16_t type = 0;
};

struct AuthRequest {
	std::string id;
	std::string password;
};

bool IsUtf8DisplayTextValid(const std::string& text, std::size_t maxBytes);

struct LoginPlayerResult {
	AuthResult result = AuthResult::ServerError;
	bool hasLoggedIn = false;
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

