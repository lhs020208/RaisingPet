// requests, server state updates, and general state helpers for ShopUI.cpp
// This file is intentionally included by ShopUI.cpp to keep helper functions in one translation unit.

void CShopUI::ResetSelectedPet(size_t activePetIndex, size_t petCount)
{
	m_nSelectedPetIndex = (activePetIndex < petCount) ? activePetIndex : 0;
}

bool CShopUI::ConsumePetConfirmationRequest(size_t& selectedPetIndex)
{
	if (m_nPendingConfirmedPetIndex == static_cast<size_t>(-1)) return(false);
	selectedPetIndex = m_nPendingConfirmedPetIndex;
	m_nPendingConfirmedPetIndex = static_cast<size_t>(-1);
	return(true);
}

bool CShopUI::ConsumePetEnhancementRequest(int& enhancementType)
{
	if (m_nPendingEnhancementType < 0) return(false);
	enhancementType = m_nPendingEnhancementType;
	m_nPendingEnhancementType = -1;
	return(true);
}

bool CShopUI::ConsumeFinancialProductRequest(int& category, int& productIndex)
{
	if (m_nPendingFinancialCategory < 0 || m_nPendingFinancialProductIndex < 0) return(false);
	category = m_nPendingFinancialCategory;
	productIndex = m_nPendingFinancialProductIndex;
	m_nPendingFinancialCategory = -1;
	m_nPendingFinancialProductIndex = -1;
	return(true);
}

bool CShopUI::ConsumeStockIssueRequest(std::wstring& stockName)
{
	if (!m_bPendingStockIssueRequest) return(false);
	stockName = m_wstrPendingStockIssueName;
	m_wstrPendingStockIssueName.clear();
	m_bPendingStockIssueRequest = false;
	return(true);
}

bool CShopUI::ConsumeStockManagementInfoRequest()
{
	if (!m_bPendingStockManagementInfoRequest) return(false);
	m_bPendingStockManagementInfoRequest = false;
	return(true);
}

bool CShopUI::ConsumeStockTransactionListRequest()
{
	if (!m_bPendingStockTransactionListRequest) return(false);
	m_bPendingStockTransactionListRequest = false;
	return(true);
}

bool CShopUI::ConsumeStockTradeRequest(int& action, UINT& stockId, UINT& quantity)
{
	if (m_nPendingStockTradeAction < 0) return(false);
	action = m_nPendingStockTradeAction;
	stockId = m_nPendingStockTradeStockId;
	quantity = m_nPendingStockTradeQuantity;
	m_nPendingStockTradeAction = -1;
	m_nPendingStockTradeStockId = 0;
	m_nPendingStockTradeQuantity = 0;
	return(true);
}

bool CShopUI::ConsumeLoginSceneReturnRequest()
{
	if (!m_bPendingLoginSceneReturnRequest) return(false);
	m_bPendingLoginSceneReturnRequest = false;
	return(true);
}

void CShopUI::SetStockIssued(bool issued, const std::wstring& stockName)
{
	m_bStockIssued = issued;
	if (!m_bStockIssued)
	{
		m_wstrStockName.clear();
		m_nStockNameCursorIndex = 0;
		m_bStockIssueButtonPressed = false;
		m_bPendingStockIssueRequest = false;
		m_bPendingStockIssueErrorLog = false;
		m_bPendingStockManagementInfoRequest = false;
		m_wstrPendingStockIssueName.clear();
		m_StockManagementInfo = SHOP_STOCK_MANAGEMENT_INFO();
		return;
	}
	if (!stockName.empty())
	{
		m_wstrStockName = stockName;
		m_nStockNameCursorIndex = m_wstrStockName.size();
	}
	m_bStockIssueButtonPressed = false;
	m_bPendingStockIssueRequest = false;
	m_bPendingStockIssueErrorLog = false;
	m_wstrPendingStockIssueName.clear();
	m_bStockNameInputActive = false;
	m_bPendingStockManagementInfoRequest = true;
}

void CShopUI::NotifyStockIssueFailed()
{
	if (m_bStockIssued) return;
	m_bPendingStockIssueErrorLog = true;
}

void CShopUI::NotifyStockTradeFailed(int action)
{
	if (action != 0 && action != 1) return;
	m_bPendingStockTradeFailLog[action] = true;
}

void CShopUI::SetStockManagementInfo(const SHOP_STOCK_MANAGEMENT_INFO& info)
{
	m_StockManagementInfo = info;
	m_bStockIssued = info.bIssued;
	if (!info.wstrStockName.empty())
	{
		m_wstrStockName = info.wstrStockName;
		m_nStockNameCursorIndex = m_wstrStockName.size();
	}
	if (!m_bStockIssued)
	{
		m_wstrStockName.clear();
		m_nStockNameCursorIndex = 0;
	}
	m_bStockIssueButtonPressed = false;
	m_bPendingStockIssueRequest = false;
	m_bPendingStockIssueErrorLog = false;
	m_wstrPendingStockIssueName.clear();
}

void CShopUI::SetStockTransactionInfos(const std::vector<SHOP_STOCK_TRANSACTION_INFO>& infos)
{
	m_StockTransactionInfos = infos;
	m_nMaximumStockTransactionScrollOffset =
		(m_StockTransactionInfos.size() > 10) ? (m_StockTransactionInfos.size() - 10) : 0;
	if (m_nStockTransactionScrollOffset > m_nMaximumStockTransactionScrollOffset)
		m_nStockTransactionScrollOffset = m_nMaximumStockTransactionScrollOffset;
	if (m_StockTransactionInfos.empty())
	{
		m_nSelectedStockTransactionIndex = 0;
		m_nStockTransactionScrollOffset = 0;
		m_nMaximumStockTransactionScrollOffset = 0;
		m_nStockTransactionOrderQuantity = 0;
		m_bStockTransactionQuantityInputActive = false;
		return;
	}
	if (m_nSelectedStockTransactionIndex >= m_StockTransactionInfos.size())
		m_nSelectedStockTransactionIndex = 0;
}

bool CShopUI::IsStockTradeButtonDisabled(int action, UINT money) const
{
	if (action != 0 && action != 1) return(true);
	if (m_StockTransactionInfos.empty()
		|| m_nSelectedStockTransactionIndex >= m_StockTransactionInfos.size())
		return(true);

	const SHOP_STOCK_TRANSACTION_INFO& stock =
		m_StockTransactionInfos[m_nSelectedStockTransactionIndex];
	if (action == 0)
	{
		const UINT64 totalPrice = static_cast<UINT64>(m_nStockTransactionOrderQuantity)
			* static_cast<UINT64>(stock.nCurrentPrice);
		return(totalPrice > static_cast<UINT64>(money));
	}
	return(m_nStockTransactionOrderQuantity > stock.nMyQuantity);
}

void CShopUI::QueueStockTradeRequest(int action)
{
	if (action != 0 && action != 1) return;
	if (m_StockTransactionInfos.empty()
		|| m_nSelectedStockTransactionIndex >= m_StockTransactionInfos.size())
		return;
	m_nPendingStockTradeAction = action;
	m_nPendingStockTradeStockId =
		m_StockTransactionInfos[m_nSelectedStockTransactionIndex].nStockId;
	m_nPendingStockTradeQuantity = m_nStockTransactionOrderQuantity;
	m_bStockTransactionQuantityInputActive = false;
}

float CShopUI::GetMasterVolumeScale() const
{
	return(static_cast<float>(m_nSettingVolumePercents[0]) / 100.0f);
}

float CShopUI::GetClickVolumeScale() const
{
	return(GetMasterVolumeScale() * static_cast<float>(m_nSettingVolumePercents[1]) / 100.0f);
}

float CShopUI::GetErrorVolumeScale() const
{
	return(GetMasterVolumeScale() * static_cast<float>(m_nSettingVolumePercents[2]) / 100.0f);
}

float CShopUI::GetCoinVolumeScale() const
{
	return(GetMasterVolumeScale() * static_cast<float>(m_nSettingVolumePercents[3]) / 100.0f);
}

void CShopUI::GetSettingValues(bool petOptions[3], UINT& petSizePercent, UINT volumePercents[4]) const
{
	for (int i = 0; i < 3; ++i)
		petOptions[i] = m_bSettingPetOptions[i];
	petSizePercent = m_nSettingPetSizePercent;
	for (int i = 0; i < 4; ++i)
		volumePercents[i] = m_nSettingVolumePercents[i];
}

void CShopUI::SetSettingValues(const bool petOptions[3], UINT petSizePercent,
	const UINT volumePercents[4])
{
	for (int i = 0; i < 3; ++i)
		m_bSettingPetOptions[i] = petOptions[i];
	if (petSizePercent < 10) petSizePercent = 10;
	if (petSizePercent > 100) petSizePercent = 100;
	m_nSettingPetSizePercent = petSizePercent;
	for (int i = 0; i < 4; ++i)
		m_nSettingVolumePercents[i] = (volumePercents[i] > 100) ? 100 : volumePercents[i];
	m_bPendingSettingChangeRequest = false;
}

bool CShopUI::ConsumeSettingChangeRequest()
{
	if (!m_bPendingSettingChangeRequest) return(false);
	m_bPendingSettingChangeRequest = false;
	return(true);
}

void CShopUI::PlayUiClickSound() const
{
	g_pFramework->PlayClickSound(GetClickVolumeScale());
}

void CShopUI::PlayUiErrorSound() const
{
	g_pFramework->PlayErrorSound(GetErrorVolumeScale());
}

void CShopUI::SetFinancialProductActive(int category, int productIndex, UINT durationSeconds)
{
	if (category < 0 || category >= 2 || productIndex < 0 || productIndex >= 10) return;
	m_bFinancialProductActive[category] = true;
	m_nActiveFinancialProductIndex[category] = productIndex;
	m_nFinancialProductIndices[category] = productIndex;
	m_nActiveFinancialDurationSeconds[category] = durationSeconds;
	m_fActiveFinancialElapsedSeconds[category] = 0.0f;
}

void CShopUI::ClearFinancialProductActive(int category, int productIndex)
{
	if (category < 0 || category >= 2) return;
	if (productIndex >= 0 && m_nActiveFinancialProductIndex[category] != productIndex) return;
	m_bFinancialProductActive[category] = false;
	m_nActiveFinancialProductIndex[category] = -1;
	m_nActiveFinancialDurationSeconds[category] = 0;
	m_fActiveFinancialElapsedSeconds[category] = 0.0f;
}

void CShopUI::SetFinancialMaximumProductIndex(int category, int productIndex)
{
	if (category < 0 || category >= 2) return;
	if (productIndex < 0) productIndex = 0;
	if (productIndex > 9) productIndex = 9;
	m_nFinancialMaximumProductIndices[category] = productIndex;
	if (!m_bFinancialProductActive[category] && m_nFinancialProductIndices[category] > productIndex)
		m_nFinancialProductIndices[category] = productIndex;
	m_bStockCreationAvailable = (m_nFinancialProgressCounts[0] >= 5
		&& m_nFinancialProgressCounts[1] >= 5);
}

void CShopUI::SetFinancialProgressCount(int category, int progressCount)
{
	if (category < 0 || category >= 2) return;
	if (progressCount < 0) progressCount = 0;
	if (progressCount > 5) progressCount = 5;
	m_nFinancialProgressCounts[category] = progressCount;
	m_bStockCreationAvailable = (m_nFinancialProgressCounts[0] >= 5
		&& m_nFinancialProgressCounts[1] >= 5);
}

void CShopUI::NotifyFinancialApplicationFailed(int category)
{
	if (category < 0 || category >= 2) return;
	m_bPendingFinancialFailLog[category] = true;
}

bool CShopUI::IsPointOver(float x, float y, float width, float height) const
{
	if (IsPointInRectangle(x, y, GetShopIconRectangle(width, height))) return(true);
	if (IsPointInRectangle(x, y, GetSettingIconRectangle(width, height))) return(true);
	if (m_bShopActive && IsPointInRectangle(x, y,
		GetShopBoardRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y)))
		return(true);
	return(m_bSettingActive && IsPointInRectangle(x, y,
		GetShopBoardRectangle(width, height,
			m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y)));
}

void CShopUI::DeactivateShop(float width, float height, size_t activePetIndex)
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float halfWidth = (board.z - board.x) * 0.5f;
	const float halfHeight = (board.w - board.y) * 0.5f;
	m_bResetShopPositionOnNextOpen = board.x < -halfWidth || board.z > width + halfWidth
		|| board.y < -halfHeight || board.w > height + halfHeight;
	m_bShopActive = false;
	m_bShopBoardDragging = false;
	m_bPetScrollDragging = false;
	m_bStockNameInputActive = false;
	m_bStockIssueButtonPressed = false;
	m_nPressedEnhanceButton = -1;
	ResetSelectedPet(activePetIndex, m_nCachedPetCount);
}

