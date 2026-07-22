// financial page click handling and UI logs for ShopUI.cpp
// This file is intentionally included by ShopUI.cpp to keep helper functions in one translation unit.

bool CShopUI::ProcessFinancialClick(float x, float y, float width, float height,
	UINT money, const SHOP_TEXT_RENDER_CONTEXT& context)
{
	for (int category = 0; category < 2; ++category)
	{
		if (!IsPointInRectangle(x, y, GetFinancialCategoryButtonRectangle(category, width, height))) continue;
		g_pFramework->PlayClickSound();
		m_nFinancialCategory = category;
		return(true);
	}

	if (IsPointInRectangle(x, y, GetFinancialLeftButtonRectangle(width, height)))
	{
		if (m_bFinancialProductActive[m_nFinancialCategory]) return(true);
		int& index = m_nFinancialProductIndices[m_nFinancialCategory];
		if (index > 0)
		{
			g_pFramework->PlayClickSound();
			--index;
		}
		return(true);
	}
	if (IsPointInRectangle(x, y, GetFinancialRightButtonRectangle(width, height)))
	{
		if (m_bFinancialProductActive[m_nFinancialCategory]) return(true);
		int& index = m_nFinancialProductIndices[m_nFinancialCategory];
		if (index < 9 && index < m_nFinancialMaximumProductIndices[m_nFinancialCategory])
		{
			g_pFramework->PlayClickSound();
			++index;
		}
		return(true);
	}
	if (IsPointInRectangle(x, y, GetFinancialApplicationButtonRectangle(width, height, money, context)))
	{
		if (!IsFinancialApplicationButtonDisabled(money))
		{
			g_pFramework->PlayClickSound();
			m_nPendingFinancialCategory = m_nFinancialCategory;
			m_nPendingFinancialProductIndex = m_nFinancialProductIndices[m_nFinancialCategory];
		}
		return(true);
	}

	return(IsPointInRectangle(x, y, GetFinancialBankFrameRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y)));
}

bool CShopUI::IsFinancialApplicationButtonDisabled(UINT money) const
{
	if (m_nFinancialCategory != 0 && m_nFinancialCategory != 1) return(true);
	const int productIndex = m_nFinancialProductIndices[m_nFinancialCategory];
	if (productIndex < 0 || productIndex > 9) return(true);
	if (productIndex > m_nFinancialMaximumProductIndices[m_nFinancialCategory]) return(true);
	if (m_bFinancialProductActive[m_nFinancialCategory]) return(true);

	if (m_nFinancialCategory == 0)
	{
		const FINANCIAL_PRODUCT_DATA& product = gInstallmentSavingsProducts[productIndex];
		if (product.nFirstMoney > money) return(true);
	}
	return(false);
}

void CShopUI::RenderFinancialFailLogs(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	UINT money, const SHOP_TEXT_RENDER_CONTEXT& context)
{
	if (!camera) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	for (int category = 0; category < 2; ++category)
	{
		if (m_bPendingFinancialFailLog[category])
		{
			const XMFLOAT4 button = GetFinancialApplicationButtonRectangle(width, height, money, context);
			const XMFLOAT4 board = GetShopBoardRectangle(width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
			const float boardWidth = board.z - board.x;
			const float logWidth = boardWidth * 0.54f;
			const float logHeight = logWidth * (133.0f / 1756.0f);
			const float centerX = (button.x + button.z) * 0.5f;
			const float top = button.y - logHeight - 12.0f;

			SHOP_NETWORK_ERROR_LOG log;
			log.rectangle = XMFLOAT4(centerX - logWidth * 0.5f, top,
				centerX + logWidth * 0.5f, top + logHeight);
			log.fElapsedTime = 0.0f;
			m_FinancialFailLogs[category].clear();
			m_FinancialFailLogs[category].push_back(log);
			m_bPendingFinancialFailLog[category] = false;
			g_pFramework->PlayErrorSound();
		}

		for (const SHOP_NETWORK_ERROR_LOG& log : m_FinancialFailLogs[category])
		{
			const float alphaRatio = max(0.0f, 1.0f - log.fElapsedTime);
			const UINT alpha = max(1u, static_cast<UINT>(alphaRatio * 255.0f + 0.5f));
			const float moveY = -20.0f * log.fElapsedTime;
			RenderUiImage(commandList, camera, m_FinancialFailLogResources[category],
				XMFLOAT4(log.rectangle.x, log.rectangle.y + moveY,
					log.rectangle.z, log.rectangle.w + moveY),
				(alpha << 24) | 0x00FFFFFF);
		}
	}
}

void CShopUI::SpawnNetworkErrorLog(float width, float height, int slotIndex)
{
	SHOP_NETWORK_ERROR_LOG log;
	log.rectangle = GetShopNetworkErrorLogRectangle(slotIndex, width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	log.fElapsedTime = 0.0f;
	m_NetworkErrorLogs.clear();
	m_NetworkErrorLogs.push_back(log);
	g_pFramework->PlayErrorSound();
}

void CShopUI::SpawnNetworkErrorLogAtNetworkIcon(float width, float height)
{
	SHOP_NETWORK_ERROR_LOG log;
	log.rectangle = GetShopNetworkIconErrorLogRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	log.fElapsedTime = 0.0f;
	m_NetworkErrorLogs.clear();
	m_NetworkErrorLogs.push_back(log);
	g_pFramework->PlayErrorSound();
}

bool CShopUI::IsNetworkRequiredPage() const
{
	switch (m_eShopPage)
	{
	case SHOP_PAGE::BANK:
	case SHOP_PAGE::STOCK_MENU:
	case SHOP_PAGE::STOCK_TRANSACTION:
	case SHOP_PAGE::STOCK_CANT_PUBLISH:
	case SHOP_PAGE::STOCK_MANAGEMENT:
	case SHOP_PAGE::STOCK_SEE_MYGRAPH:
	case SHOP_PAGE::STOCK_SEE_TARGET_GRAPH:
		return(true);
	default:
		return(false);
	}
}

void CShopUI::ReturnToShopMenuAfterNetworkDisconnected(float width, float height)
{
	m_eShopPage = SHOP_PAGE::SHOP_MENU;
	m_nSelectedShopSlot = -1;
	m_nPressedEnhanceButton = -1;
	m_bShopBoardDragging = false;
	m_bPetScrollDragging = false;
	m_bStockNameInputActive = false;
	m_bStockIssueButtonPressed = false;
	m_bStockTransactionQuantityInputActive = false;
	m_nPendingFinancialCategory = -1;
	m_nPendingFinancialProductIndex = -1;
	m_bPendingStockIssueRequest = false;
	m_bPendingStockManagementInfoRequest = false;
	m_bPendingStockTransactionListRequest = false;
	m_nPendingStockTradeAction = -1;
	m_nPendingStockTradeStockId = 0;
	m_nPendingStockTradeQuantity = 0;
	SpawnNetworkErrorLogAtNetworkIcon(width, height);
}

void CShopUI::RenderStockIssueErrorLogs(ID3D12GraphicsCommandList* commandList, CCamera* camera)
{
	if (m_bPendingStockIssueErrorLog && camera)
	{
		const float width = camera->m_d3dViewport.Width;
		const float height = camera->m_d3dViewport.Height;
		const XMFLOAT4 button = GetStockIssuanceButtonRectangle(width, height);
		const XMFLOAT4 board = GetShopBoardRectangle(width, height,
			m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
		const float boardWidth = board.z - board.x;
		const float logWidth = boardWidth * 0.54f;
		const float logHeight = logWidth * (133.0f / 1757.0f);
		const float centerX = (button.x + button.z) * 0.5f;
		const float top = button.y - logHeight - 12.0f;

		SHOP_NETWORK_ERROR_LOG log;
		log.rectangle = XMFLOAT4(centerX - logWidth * 0.5f, top,
			centerX + logWidth * 0.5f, top + logHeight);
		log.fElapsedTime = 0.0f;
		m_StockIssueErrorLogs.clear();
		m_StockIssueErrorLogs.push_back(log);
		m_bPendingStockIssueErrorLog = false;
		g_pFramework->PlayErrorSound();
	}

	for (const SHOP_NETWORK_ERROR_LOG& log : m_StockIssueErrorLogs)
	{
		const float alphaRatio = max(0.0f, 1.0f - log.fElapsedTime);
		const UINT alpha = max(1u, static_cast<UINT>(alphaRatio * 255.0f + 0.5f));
		const float moveY = -20.0f * log.fElapsedTime;
		RenderUiImage(commandList, camera, m_IssuanceStockErrorLogResource,
			XMFLOAT4(log.rectangle.x, log.rectangle.y + moveY,
				log.rectangle.z, log.rectangle.w + moveY),
			(alpha << 24) | 0x00FFFFFF);
	}
}

void CShopUI::RenderStockTradeFailLogs(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	UINT money, const SHOP_TEXT_RENDER_CONTEXT& context)
{
	if (!camera) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;

	auto getButtonRectangle = [&](int action) -> XMFLOAT4
	{
		if (m_eShopPage == SHOP_PAGE::STOCK_SEE_TARGET_GRAPH)
			return(action == 0)
				? GetStockTargetBuyingButtonRectangle(width, height, context)
				: GetStockTargetSellingButtonRectangle(width, height, context);
		return(action == 0)
			? GetStockTransactionBuyingButtonRectangle(width, height, context)
			: GetStockTransactionSellingButtonRectangle(width, height, context);
	};

	for (int action = 0; action < 2; ++action)
	{
		if (m_bPendingStockTradeFailLog[action])
		{
			const XMFLOAT4 button = getButtonRectangle(action);
			const XMFLOAT4 board = GetShopBoardRectangle(width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
			const float boardWidth = board.z - board.x;
			const float logWidth = boardWidth * 0.44f;
			const float logHeight = logWidth * ((action == 0) ? (134.0f / 1757.0f)
				: (133.0f / 1757.0f));
			const float centerX = (button.x + button.z) * 0.5f;
			const float top = button.y - logHeight - 12.0f;

			SHOP_NETWORK_ERROR_LOG log;
			log.rectangle = XMFLOAT4(centerX - logWidth * 0.5f, top,
				centerX + logWidth * 0.5f, top + logHeight);
			log.fElapsedTime = 0.0f;
			m_StockTradeFailLogs[action].clear();
			m_StockTradeFailLogs[action].push_back(log);
			m_bPendingStockTradeFailLog[action] = false;
			g_pFramework->PlayErrorSound();
		}

		UI_IMAGE_RESOURCE& resource = (action == 0)
			? m_StockBuyingFailLogResource : m_StockSellingFailLogResource;
		for (const SHOP_NETWORK_ERROR_LOG& log : m_StockTradeFailLogs[action])
		{
			const float alphaRatio = max(0.0f, 1.0f - log.fElapsedTime);
			const UINT alpha = max(1u, static_cast<UINT>(alphaRatio * 255.0f + 0.5f));
			const float moveY = -20.0f * log.fElapsedTime;
			RenderUiImage(commandList, camera, resource,
				XMFLOAT4(log.rectangle.x, log.rectangle.y + moveY,
					log.rectangle.z, log.rectangle.w + moveY),
				(alpha << 24) | 0x00FFFFFF);
		}
	}
	(void)money;
}

