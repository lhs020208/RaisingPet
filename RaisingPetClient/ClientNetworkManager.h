#pragma once

#include <WinSock2.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

enum class CLIENT_AUTH_REQUEST
{
	NONE,
	REGISTER,
	LOGIN
};

enum class CLIENT_AUTH_RESULT
{
	SUCCESS = 0,
	INVALID_FORMAT = 1,
	ALREADY_EXISTS = 2,
	NOT_FOUND = 3,
	WRONG_PASSWORD = 4,
	DATABASE_ERROR = 5,
	SERVER_ERROR = 6,
	NETWORK_ERROR = 7
};

enum class CLIENT_FINANCIAL_CATEGORY
{
	SAVINGS = 0,
	LOAN = 1
};

enum class CLIENT_FINANCIAL_RESULT
{
	SUCCESS = 0,
	INVALID_REQUEST = 1,
	NOT_AUTHENTICATED = 2,
	ALREADY_ACTIVE = 3,
	PRODUCT_NOT_FOUND = 4,
	NOT_ENOUGH_MONEY = 5,
	DATABASE_ERROR = 6,
	MONEY_LIMIT_EXCEEDED = 7
};

enum class CLIENT_STOCK_ISSUE_RESULT
{
	SUCCESS = 0,
	INVALID_REQUEST = 1,
	NOT_AUTHENTICATED = 2,
	ALREADY_ISSUED = 3,
	NOT_ENOUGH_MONEY = 4,
	DATABASE_ERROR = 5,
	NETWORK_ERROR = 6
};

struct CLIENT_FINANCIAL_APPLICATION_RESULT
{
	CLIENT_FINANCIAL_CATEGORY eCategory = CLIENT_FINANCIAL_CATEGORY::SAVINGS;
	CLIENT_FINANCIAL_RESULT eResult = CLIENT_FINANCIAL_RESULT::DATABASE_ERROR;
	unsigned int nProductId = 0;
	unsigned int nDurationSeconds = 0;
	std::int64_t nMoneyDelta = 0;
	unsigned int nFinalMoney = 0;
};

struct CLIENT_FINANCIAL_COMPLETION
{
	CLIENT_FINANCIAL_CATEGORY eCategory = CLIENT_FINANCIAL_CATEGORY::SAVINGS;
	unsigned int nProductId = 0;
};

struct CLIENT_FINANCIAL_ACTIVE_STATUS
{
	CLIENT_FINANCIAL_CATEGORY eCategory = CLIENT_FINANCIAL_CATEGORY::SAVINGS;
	unsigned int nProductId = 0;
	unsigned int nRemainingSeconds = 0;
};

struct CLIENT_STOCK_ISSUE_APPLICATION_RESULT
{
	CLIENT_STOCK_ISSUE_RESULT eResult = CLIENT_STOCK_ISSUE_RESULT::DATABASE_ERROR;
	unsigned int nFinalMoney = 0;
	unsigned int nStockId = 0;
};

struct CLIENT_STOCK_ISSUE_STATUS
{
	bool bIssued = false;
	std::string strStockNameUtf8;
};

struct CLIENT_STOCK_HOLDER_INFO
{
	std::string strPlayerId;
	unsigned int nQuantity = 0;
};

struct CLIENT_STOCK_PRICE_INFO
{
	unsigned int nPreviousPrice = 0;
	unsigned int nNewPrice = 0;
	unsigned int nBoughtQuantity = 0;
	unsigned int nSoldQuantity = 0;
	std::string strChangedTime;
};

struct CLIENT_STOCK_MANAGEMENT_INFO
{
	bool bIssued = false;
	std::string strStockNameUtf8;
	unsigned int nSoldQuantity = 0;
	unsigned int nUnsoldQuantity = 0;
	unsigned int nSaleableQuantity = 800;
	unsigned int nIssuanceRevenue = 0;
	unsigned int nRecentTradeQuantity = 0;
	unsigned int nCurrentPrice = 100;
	unsigned int nPreviousPrice = 100;
	CLIENT_STOCK_HOLDER_INFO TopHolders[3];
	std::vector<CLIENT_STOCK_PRICE_INFO> RecentPrices;
};

struct CLIENT_STOCK_TRANSACTION_INFO
{
	unsigned int nStockId = 0;
	std::string strStockNameUtf8;
	std::string strIssuerId;
	unsigned int nCurrentPrice = 100;
	unsigned int nPreviousPrice = 100;
	unsigned int nSaleableQuantity = 0;
	unsigned int nMyQuantity = 0;
	unsigned int nRecentTradeQuantity = 0;
};

class CClientNetworkManager
{
public:
	CClientNetworkManager();
	~CClientNetworkManager();

	bool StartRegister(const std::string& id, const std::string& password);
	bool StartLogin(const std::string& id, const std::string& password);
	bool SendMoneyUpdate(unsigned int money);
	bool SendSavingsJoinRequest(unsigned int nProductId);
	bool SendLoanApplyRequest(unsigned int nProductId);
	bool SendStockIssueRequest(const std::string& stockNameUtf8);
	bool SendStockManagementInfoRequest();
	bool SendStockTransactionListRequest();
	void Disconnect();

	bool ConsumeAuthResult(CLIENT_AUTH_REQUEST& request, CLIENT_AUTH_RESULT& result);
	bool ConsumeServerMoneyChange(std::int64_t& deltaMoney, unsigned int& finalMoney);
	bool ConsumeFinancialApplicationResult(CLIENT_FINANCIAL_APPLICATION_RESULT& result);
	bool ConsumeFinancialCompletion(CLIENT_FINANCIAL_COMPLETION& completion);
	bool ConsumeFinancialActiveStatus(CLIENT_FINANCIAL_ACTIVE_STATUS& status);
	bool ConsumeStockIssueResult(CLIENT_STOCK_ISSUE_APPLICATION_RESULT& result);
	bool ConsumeStockIssueStatus(CLIENT_STOCK_ISSUE_STATUS& status);
	bool ConsumeStockManagementInfo(CLIENT_STOCK_MANAGEMENT_INFO& info);
	bool ConsumeStockTransactionList(std::vector<CLIENT_STOCK_TRANSACTION_INFO>& infos);
	bool IsBusy() const { return m_bBusy.load(); }
	bool IsConnected() const;

private:
	struct SERVER_INFORMATION
	{
		std::string ip = "127.0.0.1";
		unsigned short port = 7777;
	};

	bool StartAuthRequest(CLIENT_AUTH_REQUEST request, const std::string& id,
		const std::string& password);
	void AuthThread(CLIENT_AUTH_REQUEST request, std::string id, std::string password);
	SERVER_INFORMATION LoadServerInformation() const;
	void StoreResult(CLIENT_AUTH_REQUEST request, CLIENT_AUTH_RESULT result);
	void CloseSocket();

	std::thread m_WorkerThread;
	mutable std::mutex m_Mutex;
	SOCKET m_Socket = INVALID_SOCKET;
	CLIENT_AUTH_REQUEST m_CompletedRequest = CLIENT_AUTH_REQUEST::NONE;
	CLIENT_AUTH_RESULT m_CompletedResult = CLIENT_AUTH_RESULT::SERVER_ERROR;
	bool m_bHasCompletedResult = false;
	std::vector<std::pair<std::int64_t, unsigned int>> m_ServerMoneyChanges;
	std::vector<CLIENT_FINANCIAL_APPLICATION_RESULT> m_FinancialApplicationResults;
	std::vector<CLIENT_FINANCIAL_COMPLETION> m_FinancialCompletions;
	std::vector<CLIENT_FINANCIAL_ACTIVE_STATUS> m_FinancialActiveStatuses;
	std::vector<CLIENT_STOCK_ISSUE_APPLICATION_RESULT> m_StockIssueResults;
	std::vector<CLIENT_STOCK_ISSUE_STATUS> m_StockIssueStatuses;
	std::vector<CLIENT_STOCK_MANAGEMENT_INFO> m_StockManagementInfos;
	std::vector<std::vector<CLIENT_STOCK_TRANSACTION_INFO>> m_StockTransactionLists;
	std::atomic_bool m_bStopRequested = false;
	std::atomic_bool m_bBusy = false;
	std::atomic_bool m_bConnected = false;
};
