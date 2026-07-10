#pragma once

#include <WinSock2.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

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

class CClientNetworkManager
{
public:
	CClientNetworkManager();
	~CClientNetworkManager();

	bool StartRegister(const std::string& id, const std::string& password);
	bool StartLogin(const std::string& id, const std::string& password);
	void Disconnect();

	bool ConsumeAuthResult(CLIENT_AUTH_REQUEST& request, CLIENT_AUTH_RESULT& result);
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
	std::atomic_bool m_bStopRequested = false;
	std::atomic_bool m_bBusy = false;
	std::atomic_bool m_bConnected = false;
};
