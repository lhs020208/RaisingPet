// mouse and keyboard input handling for ShopUI.cpp
// This file is intentionally included by ShopUI.cpp to keep helper functions in one translation unit.

void CShopUI::UpdateSettingSliderFromCursor(int sliderIndex, float x, float width, float height)
{
	if (sliderIndex < 0 || sliderIndex > 4) return;
	const XMFLOAT4 bar = GetSettingSliderBarRectangle(sliderIndex, width, height,
		m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y);
	const float ratio = (bar.z > bar.x) ? ((x - bar.x) / (bar.z - bar.x)) : 0.0f;
	const float clampedRatio = min(max(ratio, 0.0f), 1.0f);
	if (sliderIndex == 0)
	{
		m_nSettingPetSizePercent = 10u + static_cast<UINT>(clampedRatio * 90.0f + 0.5f);
		return;
	}
	m_nSettingVolumePercents[sliderIndex - 1] =
		static_cast<UINT>(clampedRatio * 100.0f + 0.5f);
}

bool CShopUI::ProcessSettingBoardClick(HWND hWnd, float x, float y, float width, float height)
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y);
	const XMFLOAT4 close = GetShopCloseRectangle(width, height,
		m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y);
	if (IsPointInRectangle(x, y, close))
	{
		PlayUiClickSound();
		const float halfWidth = (board.z - board.x) * 0.5f;
		const float halfHeight = (board.w - board.y) * 0.5f;
		m_bResetSettingPositionOnNextOpen = board.x < -halfWidth
			|| board.z > width + halfWidth
			|| board.y < -halfHeight
			|| board.w > height + halfHeight;
		m_bSettingActive = false;
		m_bSettingBoardDragging = false;
		m_nDraggingSettingSlider = -1;
		return(true);
	}
	for (int i = 0; i < 3; ++i)
	{
		if (!IsPointInRectangle(x, y, GetSettingCheckBoxRectangle(i, width, height,
			m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y))) continue;
		PlayUiClickSound();
		m_bSettingPetOptions[i] = !m_bSettingPetOptions[i];
		return(true);
	}
	for (int i = 0; i < 5; ++i)
	{
		const XMFLOAT4 bar = GetSettingSliderBarRectangle(i, width, height,
			m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y);
		const XMFLOAT4 handle = GetSettingSliderHandleRectangle(i, GetSettingSliderRatio(i),
			width, height, m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y);
		if (!IsPointInRectangle(x, y, bar) && !IsPointInRectangle(x, y, handle)) continue;
		PlayUiClickSound();
		UpdateSettingSliderFromCursor(i, x, width, height);
		m_nDraggingSettingSlider = i;
		SetCapture(hWnd);
		return(true);
	}
	m_bSettingBoardDragging = true;
	m_xmf2SettingDragLastCursor = XMFLOAT2(x, y);
	SetCapture(hWnd);
	return(true);
}

bool CShopUI::ProcessShopUIClick(float x, float y, float width, float height, UINT money,
	size_t petCount, size_t activePetIndex, const SHOP_TEXT_RENDER_CONTEXT& context,
	bool networkConnected)
{
	const bool settingIconClicked = IsPointInRectangle(x, y, GetSettingIconRectangle(width, height));
	if (settingIconClicked)
	{
		PlayUiClickSound();
		if (m_bSettingActive)
		{
			m_bSettingActive = false;
			m_bSettingBoardDragging = false;
			m_nDraggingSettingSlider = -1;
		}
		else
		{
			if (m_bResetSettingPositionOnNextOpen)
			{
				m_xmf2SettingBoardOffset = XMFLOAT2(0.0f, 0.0f);
				m_bResetSettingPositionOnNextOpen = false;
			}
			m_bSettingActive = true;
			m_bSettingBoardOnTop = true;
		}
		return(true);
	}
	const bool iconClicked = IsPointInRectangle(x, y, GetShopIconRectangle(width, height));
	const bool closeClicked = m_bShopActive && IsPointInRectangle(x, y,
		GetShopCloseRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
	if (iconClicked || closeClicked)
	{
		PlayUiClickSound();
		if (m_bShopActive) DeactivateShop(width, height, activePetIndex);
		else
		{
			if (m_bResetShopPositionOnNextOpen)
			{
				m_xmf2ShopBoardOffset = XMFLOAT2(0.0f, 0.0f);
				m_bResetShopPositionOnNextOpen = false;
			}
			m_bShopActive = true;
			m_bSettingBoardOnTop = false;
			m_eShopPage = SHOP_PAGE::SHOP_MENU;
			m_nSelectedShopSlot = -1;
			m_bStockNameInputActive = false;
			ResetSelectedPet(activePetIndex, petCount);
		}
		return(true);
	}
	if (!m_bShopActive) return(false);
	if (IsPointInRectangle(x, y,
		GetShopBackRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y)))
	{
		PlayUiClickSound();
		if (m_eShopPage == SHOP_PAGE::SHOP_MENU) DeactivateShop(width, height, activePetIndex);
		else if (m_eShopPage == SHOP_PAGE::STOCK_SEE_MYGRAPH)
		{
			m_eShopPage = SHOP_PAGE::STOCK_MANAGEMENT;
			m_nPressedEnhanceButton = -1;
			m_bStockNameInputActive = false;
			m_bStockIssueButtonPressed = false;
		}
		else if (m_eShopPage == SHOP_PAGE::STOCK_SEE_TARGET_GRAPH)
		{
			m_eShopPage = SHOP_PAGE::STOCK_TRANSACTION;
			m_bStockTransactionQuantityInputActive = false;
		}
		else if (m_eShopPage == SHOP_PAGE::STOCK_TRANSACTION
			|| m_eShopPage == SHOP_PAGE::STOCK_MANAGEMENT
			|| m_eShopPage == SHOP_PAGE::STOCK_CANT_PUBLISH)
		{
			m_eShopPage = SHOP_PAGE::STOCK_MENU;
			m_nPressedEnhanceButton = -1;
			m_bStockNameInputActive = false;
			m_bStockIssueButtonPressed = false;
		}
		else
		{
			m_eShopPage = SHOP_PAGE::SHOP_MENU;
			m_nSelectedShopSlot = -1;
			m_nPressedEnhanceButton = -1;
			m_bStockNameInputActive = false;
			m_bStockIssueButtonPressed = false;
			ResetSelectedPet(activePetIndex, petCount);
		}
		return(true);
	}
	if (m_eShopPage == SHOP_PAGE::SHOP_MENU)
	{
		if (!networkConnected && IsPointInRectangle(x, y, GetShopNetworkIconRectangle(width, height,
			m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y)))
		{
			m_bPendingLoginSceneReturnRequest = true;
			return(true);
		}
		for (int i = 0; i < 4; ++i)
		{
			if (!IsPointInRectangle(x, y, GetShopSlotRectangle(i, width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y))) continue;
			if (!networkConnected && (i == 2 || i == 3))
			{
				SpawnNetworkErrorLog(width, height, i);
				return(true);
			}
			PlayUiClickSound();
			switch (i)
			{
			case 0: m_eShopPage = SHOP_PAGE::PET_CHANGE; break;
			case 1: m_eShopPage = SHOP_PAGE::PET_ENHANCE; break;
			case 2: m_eShopPage = SHOP_PAGE::BANK; break;
			case 3: m_eShopPage = SHOP_PAGE::STOCK_MENU; break;
			default: m_eShopPage = SHOP_PAGE::SHOP_MENU; break;
			}
			m_nSelectedShopSlot = i;
			m_bStockNameInputActive = false;
			if (m_eShopPage == SHOP_PAGE::PET_CHANGE)
				ResetSelectedPet(activePetIndex, petCount);
			return(true);
		}
	}
	else if (m_eShopPage == SHOP_PAGE::PET_CHANGE)
	{
		const XMFLOAT4 left = GetPetContentPanelRectangle(false, width, height,
			m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
		const XMFLOAT4 right = GetPetContentPanelRectangle(true, width, height,
			m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
		const XMFLOAT4 confirm = GetPetConfirmationRectangle(width, height,
			m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
		if (IsPointInRectangle(x, y, confirm))
		{
			if (m_nSelectedPetIndex < petCount)
			{
				PlayUiClickSound();
				m_nPendingConfirmedPetIndex = m_nSelectedPetIndex;
			}
			return(true);
		}
		for (size_t row = 0; row < 10; ++row)
		{
			const size_t petIndex = m_nPetScrollOffset + row;
			if (petIndex >= petCount) break;
			if (!IsPointInRectangle(x, y, GetPetListRowRectangle(row, width, height))) continue;
			m_nSelectedPetIndex = petIndex;
			return(true);
		}
		if (IsPointInRectangle(x, y, left) || IsPointInRectangle(x, y, right)) return(true);
	}
	else if (m_eShopPage == SHOP_PAGE::PET_ENHANCE)
	{
		const XMFLOAT4 left = GetPetContentPanelRectangle(false, width, height,
			m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
		const XMFLOAT4 right = GetPetContentPanelRectangle(true, width, height,
			m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
		if (IsPointInRectangle(x, y, left) || IsPointInRectangle(x, y, right)) return(true);
	}
	else if (m_eShopPage == SHOP_PAGE::BANK)
	{
		if (ProcessFinancialClick(x, y, width, height, money, context)) return(true);
	}
	else if (m_eShopPage == SHOP_PAGE::STOCK_MENU)
	{
		if (ProcessStockMenuClick(x, y, width, height)) return(true);
	}
	else if (m_eShopPage == SHOP_PAGE::STOCK_MANAGEMENT)
	{
		if (IsPointInRectangle(x, y, GetStockGraphButtonRectangle(width, height)))
		{
			if (!m_bStockIssued) return(true);
			PlayUiClickSound();
			m_eShopPage = SHOP_PAGE::STOCK_SEE_MYGRAPH;
			m_bStockNameInputActive = false;
			m_bStockIssueButtonPressed = false;
			return(true);
		}
	}
	else if (m_eShopPage == SHOP_PAGE::STOCK_TRANSACTION)
	{
		const XMFLOAT4 left = GetPetContentPanelRectangle(false, width, height,
			m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
		const XMFLOAT4 right = GetPetContentPanelRectangle(true, width, height,
			m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
		if (!m_StockTransactionInfos.empty()
			&& IsPointInRectangle(x, y,
				GetStockTransactionBuyingButtonRectangle(width, height, context)))
		{
			if (!IsStockTradeButtonDisabled(0, money))
			{
				PlayUiClickSound();
				QueueStockTradeRequest(0);
			}
			return(true);
		}
		if (!m_StockTransactionInfos.empty()
			&& IsPointInRectangle(x, y,
				GetStockTransactionSellingButtonRectangle(width, height, context)))
		{
			if (!IsStockTradeButtonDisabled(1, money))
			{
				PlayUiClickSound();
				QueueStockTradeRequest(1);
			}
			return(true);
		}
		if (!m_StockTransactionInfos.empty()
			&& IsPointInRectangle(x, y, GetStockTransactionGraphButtonRectangle(width, height)))
		{
			PlayUiClickSound();
			m_eShopPage = SHOP_PAGE::STOCK_SEE_TARGET_GRAPH;
			m_bStockTransactionQuantityInputActive = false;
			return(true);
		}
		for (size_t row = 0; row < 10; ++row)
		{
			const size_t stockIndex = m_nStockTransactionScrollOffset + row;
			if (stockIndex >= m_StockTransactionInfos.size()) break;
			if (!IsPointInRectangle(x, y,
				GetStockTransactionListRowRectangle(row, width, height))) continue;
			m_nSelectedStockTransactionIndex = stockIndex;
			m_nStockTransactionOrderQuantity = 0;
			m_bStockTransactionQuantityInputActive = false;
			return(true);
		}
		if (!m_StockTransactionInfos.empty()
			&& IsPointInRectangle(x, y, GetStockTransactionQuantityRectangle(width, height)))
		{
			m_bStockTransactionQuantityInputActive = true;
			m_fStockTransactionQuantityCursorBlinkElapsed = 0.0f;
			return(true);
		}
		m_bStockTransactionQuantityInputActive = false;
		if (IsPointInRectangle(x, y, left) || IsPointInRectangle(x, y, right)) return(true);
	}
	else if (m_eShopPage == SHOP_PAGE::STOCK_SEE_TARGET_GRAPH)
	{
		if (!m_StockTransactionInfos.empty()
			&& IsPointInRectangle(x, y,
				GetStockTargetBuyingButtonRectangle(width, height, context)))
		{
			if (!IsStockTradeButtonDisabled(0, money))
			{
				PlayUiClickSound();
				QueueStockTradeRequest(0);
			}
			return(true);
		}
		if (!m_StockTransactionInfos.empty()
			&& IsPointInRectangle(x, y,
				GetStockTargetSellingButtonRectangle(width, height, context)))
		{
			if (!IsStockTradeButtonDisabled(1, money))
			{
				PlayUiClickSound();
				QueueStockTradeRequest(1);
			}
			return(true);
		}
		if (!m_StockTransactionInfos.empty()
			&& IsPointInRectangle(x, y, GetStockTargetQuantityRectangle(width, height)))
		{
			m_bStockTransactionQuantityInputActive = true;
			m_fStockTransactionQuantityCursorBlinkElapsed = 0.0f;
			return(true);
		}
		m_bStockTransactionQuantityInputActive = false;
	}
	return(false);
}

bool CShopUI::IsPointOverClickableButton(float x, float y, float width, float height,
	UINT money, size_t petCount, size_t, const SHOP_TEXT_RENDER_CONTEXT& context,
	bool networkConnected) const
{
	if (IsPointInRectangle(x, y, GetShopIconRectangle(width, height))) return(true);
	if (IsPointInRectangle(x, y, GetSettingIconRectangle(width, height))) return(true);

	const XMFLOAT4 shopBoard = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const XMFLOAT4 settingBoard = GetShopBoardRectangle(width, height,
		m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y);
	const bool inShopBoard = m_bShopActive && IsPointInRectangle(x, y, shopBoard);
	const bool inSettingBoard = m_bSettingActive && IsPointInRectangle(x, y, settingBoard);
	const XMFLOAT4 settingClose = GetShopCloseRectangle(width, height,
		m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y);
	auto isOverSettingControl = [&]() -> bool
	{
		if (IsPointInRectangle(x, y, settingClose)) return(true);
		for (int i = 0; i < 3; ++i)
			if (IsPointInRectangle(x, y, GetSettingCheckBoxRectangle(i, width, height,
				m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y)))
				return(true);
		for (int i = 0; i < 5; ++i)
		{
			const XMFLOAT4 bar = GetSettingSliderBarRectangle(i, width, height,
				m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y);
			const XMFLOAT4 handle = GetSettingSliderHandleRectangle(i, GetSettingSliderRatio(i),
				width, height, m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y);
			if (IsPointInRectangle(x, y, bar) || IsPointInRectangle(x, y, handle)) return(true);
		}
		return(false);
	};

	if (m_bSettingActive && m_bSettingBoardOnTop && inSettingBoard)
		return(isOverSettingControl());
	if (m_bSettingActive && !m_bShopActive && inSettingBoard)
		return(isOverSettingControl());
	if (m_bSettingActive && m_bShopActive && m_bSettingBoardOnTop && inShopBoard)
		return(false);
	if (m_bSettingActive && m_bShopActive && !m_bSettingBoardOnTop
		&& inSettingBoard && !inShopBoard)
		return(false);

	if (!m_bShopActive) return(false);

	if (IsPointInRectangle(x, y,
		GetShopCloseRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y)))
		return(true);
	if (IsPointInRectangle(x, y,
		GetShopBackRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y)))
		return(true);

	if (m_eShopPage == SHOP_PAGE::SHOP_MENU)
	{
		for (int i = 0; i < 4; ++i)
		{
			if (!IsPointInRectangle(x, y, GetShopSlotRectangle(i, width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y))) continue;
			return(networkConnected || (i != 2 && i != 3));
		}
		return(false);
	}
	if (m_eShopPage == SHOP_PAGE::PET_CHANGE)
	{
		return(m_nSelectedPetIndex < petCount
			&& IsPointInRectangle(x, y, GetPetConfirmationRectangle(width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y)));
	}
	if (m_eShopPage == SHOP_PAGE::PET_ENHANCE)
	{
		for (int type = 0; type < 2; ++type)
			if (IsPointInRectangle(x, y, GetEnhanceButtonRectangle(type, width, height)))
				return(true);
		return(false);
	}
	if (m_eShopPage == SHOP_PAGE::BANK)
	{
		for (int category = 0; category < 2; ++category)
			if (IsPointInRectangle(x, y, GetFinancialCategoryButtonRectangle(category, width, height)))
				return(true);
		if (IsPointInRectangle(x, y, GetFinancialLeftButtonRectangle(width, height)))
			return(!m_bFinancialProductActive[m_nFinancialCategory]
				&& m_nFinancialProductIndices[m_nFinancialCategory] > 0);
		if (IsPointInRectangle(x, y, GetFinancialRightButtonRectangle(width, height)))
			return(!m_bFinancialProductActive[m_nFinancialCategory]
				&& m_nFinancialProductIndices[m_nFinancialCategory] < 9
				&& m_nFinancialProductIndices[m_nFinancialCategory]
					< m_nFinancialMaximumProductIndices[m_nFinancialCategory]);
		if (IsPointInRectangle(x, y,
			GetFinancialApplicationButtonRectangle(width, height, money, context)))
			return(!IsFinancialApplicationButtonDisabled(money));
		return(false);
	}
	if (m_eShopPage == SHOP_PAGE::STOCK_MENU)
	{
		return(IsPointInRectangle(x, y, GetStockSlotRectangle(0, width, height))
			|| IsPointInRectangle(x, y, GetStockSlotRectangle(1, width, height)));
	}
	if (m_eShopPage == SHOP_PAGE::STOCK_MANAGEMENT)
	{
		if (IsPointInRectangle(x, y, GetStockIssuanceButtonRectangle(width, height)))
			return(!m_bStockIssued && !m_wstrStockName.empty());
		if (IsPointInRectangle(x, y, GetStockGraphButtonRectangle(width, height)))
			return(m_bStockIssued);
		return(false);
	}
	if (m_eShopPage == SHOP_PAGE::STOCK_TRANSACTION)
	{
		if (m_StockTransactionInfos.empty()) return(false);
		if (IsPointInRectangle(x, y, GetStockTransactionBuyingButtonRectangle(width, height, context)))
			return(!IsStockTradeButtonDisabled(0, money));
		if (IsPointInRectangle(x, y, GetStockTransactionSellingButtonRectangle(width, height, context)))
			return(!IsStockTradeButtonDisabled(1, money));
		return(IsPointInRectangle(x, y, GetStockTransactionGraphButtonRectangle(width, height)));
	}
	if (m_eShopPage == SHOP_PAGE::STOCK_SEE_TARGET_GRAPH)
	{
		if (m_StockTransactionInfos.empty()) return(false);
		if (IsPointInRectangle(x, y, GetStockTargetBuyingButtonRectangle(width, height, context)))
			return(!IsStockTradeButtonDisabled(0, money));
		if (IsPointInRectangle(x, y, GetStockTargetSellingButtonRectangle(width, height, context)))
			return(!IsStockTradeButtonDisabled(1, money));
		return(false);
	}
	return(false);
}

bool CShopUI::OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM)
{
	if (m_bShopActive && (m_eShopPage == SHOP_PAGE::STOCK_TRANSACTION
		|| m_eShopPage == SHOP_PAGE::STOCK_SEE_TARGET_GRAPH)
		&& m_bStockTransactionQuantityInputActive)
	{
		const bool hasSelectedStock = !m_StockTransactionInfos.empty()
			&& m_nSelectedStockTransactionIndex < m_StockTransactionInfos.size();
		if (!hasSelectedStock)
		{
			m_bStockTransactionQuantityInputActive = false;
			return(true);
		}
		const UINT maximumQuantity =
			m_StockTransactionInfos[m_nSelectedStockTransactionIndex].nSaleableQuantity;
		if (message == WM_CHAR)
		{
			const wchar_t ch = static_cast<wchar_t>(wParam);
			if (ch == L'\b')
			{
				m_nStockTransactionOrderQuantity /= 10;
				m_fStockTransactionQuantityCursorBlinkElapsed = 0.0f;
				return(true);
			}
			if (ch >= L'0' && ch <= L'9')
			{
				const UINT digit = static_cast<UINT>(ch - L'0');
				UINT64 nextQuantity = static_cast<UINT64>(m_nStockTransactionOrderQuantity) * 10 + digit;
				if (nextQuantity > maximumQuantity) nextQuantity = maximumQuantity;
				m_nStockTransactionOrderQuantity = static_cast<UINT>(nextQuantity);
				m_fStockTransactionQuantityCursorBlinkElapsed = 0.0f;
				return(true);
			}
			return(true);
		}
		if (message == WM_KEYDOWN)
		{
			switch (wParam)
			{
			case VK_DELETE:
				m_nStockTransactionOrderQuantity = 0;
				m_fStockTransactionQuantityCursorBlinkElapsed = 0.0f;
				return(true);
			case VK_ESCAPE:
				m_bStockTransactionQuantityInputActive = false;
				return(true);
			default:
				return(false);
			}
		}
		return(false);
	}

	if (!m_bShopActive || m_eShopPage != SHOP_PAGE::STOCK_MANAGEMENT || !m_bStockNameInputActive
		|| m_bStockIssued)
		return(false);

	RECT client;
	GetClientRect(hWnd, &client);
	const float width = static_cast<float>(client.right);
	const float height = static_cast<float>(client.bottom);
	const XMFLOAT4 inputRect = GetStockNameInputRectangle(width, height);
	const float inputHeight = inputRect.w - inputRect.y;
	const float textPaddingX = inputHeight * 0.35f;
	const float availableWidth = (inputRect.z - inputRect.x) - textPaddingX * 2.0f;
	const float availableHeight = inputRect.w - inputRect.y;
	const float fontSize = GetStockNameInputFontSize(width, height);
	m_nStockNameCursorIndex = min(m_nStockNameCursorIndex, m_wstrStockName.size());

	if (message == WM_CHAR)
	{
		const wchar_t ch = static_cast<wchar_t>(wParam);
		if (ch == L'\b')
		{
			if (m_nStockNameCursorIndex == 0) return(true);
			m_wstrStockName.erase(m_nStockNameCursorIndex - 1, 1);
			--m_nStockNameCursorIndex;
			m_fStockNameCursorBlinkElapsed = 0.0f;
			return(true);
		}
		if (ch < 0x20 || ch == 0x7F) return(true);

		std::wstring candidate = m_wstrStockName;
		candidate.insert(candidate.begin() + m_nStockNameCursorIndex, ch);
		const float candidateWidth = MeasureStockNameTextWidth(candidate, fontSize,
			availableWidth, availableHeight);
		if (candidateWidth > availableWidth) return(true);

		m_wstrStockName.swap(candidate);
		++m_nStockNameCursorIndex;
		m_fStockNameCursorBlinkElapsed = 0.0f;
		return(true);
	}

	if (message != WM_KEYDOWN) return(false);
	switch (wParam)
	{
	case VK_LEFT:
		if (m_nStockNameCursorIndex > 0) --m_nStockNameCursorIndex;
		break;
	case VK_RIGHT:
		if (m_nStockNameCursorIndex < m_wstrStockName.size()) ++m_nStockNameCursorIndex;
		break;
	case VK_HOME:
		m_nStockNameCursorIndex = 0;
		break;
	case VK_END:
		m_nStockNameCursorIndex = m_wstrStockName.size();
		break;
	case VK_DELETE:
		if (m_nStockNameCursorIndex < m_wstrStockName.size())
			m_wstrStockName.erase(m_nStockNameCursorIndex, 1);
		break;
	case VK_ESCAPE:
		m_bStockNameInputActive = false;
		break;
	default:
		return(false);
	}
	m_fStockNameCursorBlinkElapsed = 0.0f;
	return(true);
}

bool CShopUI::OnProcessingMouseMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
	UINT money, size_t nPetCount, size_t activePetIndex, const SHOP_TEXT_RENDER_CONTEXT& context,
	bool networkConnected)
{
	RECT client;
	GetClientRect(hWnd, &client);
	const float width = static_cast<float>(client.right);
	const float height = static_cast<float>(client.bottom);
	POINT cursor = { static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
	if (message == WM_MOUSEWHEEL) ScreenToClient(hWnd, &cursor);
	const float x = static_cast<float>(cursor.x);
	const float y = static_cast<float>(cursor.y);
	if (m_bShopActive && !networkConnected && IsNetworkRequiredPage())
	{
		ReturnToShopMenuAfterNetworkDisconnected(width, height);
		ReleaseCapture();
		return(true);
	}
	switch (message)
	{
	case WM_MOUSEWHEEL:
		if (m_bShopActive && m_eShopPage == SHOP_PAGE::PET_CHANGE)
		{
			const XMFLOAT4 panel = GetPetContentPanelRectangle(false, width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
			if (IsPointInRectangle(x, y, panel))
			{
				RebuildPetScrollMetricsIfNeeded(nPetCount);
				const int wheelDelta = static_cast<short>(HIWORD(wParam));
				int steps = abs(wheelDelta) / WHEEL_DELTA;
				if (steps < 1) steps = 1;
				while (steps-- > 0)
				{
					if (wheelDelta < 0 && m_nPetScrollOffset < m_nMaximumPetScrollOffset) ++m_nPetScrollOffset;
					else if (wheelDelta > 0 && m_nPetScrollOffset > 0) --m_nPetScrollOffset;
				}
				return(true);
			}
		}
		if (m_bShopActive && m_eShopPage == SHOP_PAGE::STOCK_TRANSACTION)
		{
			const XMFLOAT4 panel = GetPetContentPanelRectangle(false, width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
			if (IsPointInRectangle(x, y, panel))
			{
				const int wheelDelta = static_cast<short>(HIWORD(wParam));
				int steps = abs(wheelDelta) / WHEEL_DELTA;
				if (steps < 1) steps = 1;
				while (steps-- > 0)
				{
					if (wheelDelta < 0
						&& m_nStockTransactionScrollOffset < m_nMaximumStockTransactionScrollOffset)
						++m_nStockTransactionScrollOffset;
					else if (wheelDelta > 0 && m_nStockTransactionScrollOffset > 0)
						--m_nStockTransactionScrollOffset;
				}
				if (m_nSelectedStockTransactionIndex < m_nStockTransactionScrollOffset)
					m_nSelectedStockTransactionIndex = m_nStockTransactionScrollOffset;
				const size_t lastVisible = m_nStockTransactionScrollOffset + 9;
				if (m_nSelectedStockTransactionIndex > lastVisible
					&& lastVisible < m_StockTransactionInfos.size())
					m_nSelectedStockTransactionIndex = lastVisible;
				return(true);
			}
		}
		break;
	case WM_LBUTTONDOWN:
	{
		RebuildPetScrollMetricsIfNeeded(nPetCount);
		const bool iconClicked = IsPointInRectangle(x, y, GetShopIconRectangle(width, height));
		const bool settingIconClicked = IsPointInRectangle(x, y, GetSettingIconRectangle(width, height));
		if (iconClicked || settingIconClicked)
		{
			if (ProcessShopUIClick(x, y, width, height, money, nPetCount, activePetIndex, context,
				networkConnected)) return(true);
		}

		const XMFLOAT4 shopBoard = GetShopBoardRectangle(width, height,
			m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
		const XMFLOAT4 settingBoard = GetShopBoardRectangle(width, height,
			m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y);
		const bool inShopBoard = m_bShopActive && IsPointInRectangle(x, y, shopBoard);
		const bool inSettingBoard = m_bSettingActive && IsPointInRectangle(x, y, settingBoard);
		if (m_bSettingActive && m_bShopActive)
		{
			if (m_bSettingBoardOnTop)
			{
				if (inSettingBoard)
					return(ProcessSettingBoardClick(hWnd, x, y, width, height));
				if (inShopBoard)
				{
					m_bSettingBoardOnTop = false;
					const XMFLOAT4 moneyRect = GetMoneyUiRectangle(width, height, money, context);
					if (!IsPointInRectangle(x, y, moneyRect))
					{
						m_bShopBoardDragging = true;
						m_xmf2ShopDragLastCursor = XMFLOAT2(x, y);
						SetCapture(hWnd);
					}
					return(true);
				}
			}
			else
			{
				if (!inShopBoard && inSettingBoard)
				{
					m_bSettingBoardOnTop = true;
					return(ProcessSettingBoardClick(hWnd, x, y, width, height));
				}
			}
		}
		else if (m_bSettingActive && inSettingBoard)
			return(ProcessSettingBoardClick(hWnd, x, y, width, height));

		if (m_bShopActive && m_eShopPage == SHOP_PAGE::STOCK_MANAGEMENT)
		{
			if (!m_bStockIssued &&
				IsPointInRectangle(x, y, GetStockIssuanceButtonRectangle(width, height)))
			{
				m_bStockIssueButtonPressed = true;
				m_bStockNameInputActive = false;
				SetCapture(hWnd);
				return(true);
			}
			const XMFLOAT4 stockNameInput = GetStockNameInputRectangle(width, height);
			if (IsPointInRectangle(x, y, stockNameInput))
			{
				if (m_bStockIssued) return(true);
				m_bStockNameInputActive = true;
				m_fStockNameCursorBlinkElapsed = 0.0f;
				MoveStockNameCursorFromClick(x, stockNameInput,
					GetStockNameInputFontSize(width, height));
				return(true);
			}
			m_bStockNameInputActive = false;
		}
		if (m_bShopActive && m_eShopPage == SHOP_PAGE::PET_ENHANCE)
		{
			for (int type = 0; type < 2; ++type)
			{
				if (!IsPointInRectangle(x, y, GetEnhanceButtonRectangle(type, width, height))) continue;
				m_nPressedEnhanceButton = type;
				SetCapture(hWnd);
				return(true);
			}
		}
		if (m_bShopActive && m_eShopPage == SHOP_PAGE::PET_CHANGE
			&& m_nMaximumPetScrollOffset > 0
			&& IsPointInRectangle(x, y, GetPetScrollThumbRectangle(width, height)))
		{
			m_bPetScrollDragging = true;
			m_fPetScrollDragLastY = y;
			return(true);
		}
		if (ProcessShopUIClick(x, y, width, height, money, nPetCount, activePetIndex, context,
			networkConnected)) return(true);
		if (m_bShopActive)
		{
			const XMFLOAT4 moneyRect = GetMoneyUiRectangle(width, height, money, context);
			if (IsPointInRectangle(x, y, shopBoard) && !IsPointInRectangle(x, y, moneyRect))
			{
				m_bShopBoardDragging = true;
				m_xmf2ShopDragLastCursor = XMFLOAT2(x, y);
				SetCapture(hWnd);
				return(true);
			}
		}
		break;
	}
	case WM_MOUSEMOVE:
		if (m_nDraggingSettingSlider >= 0)
		{
			const bool outside = x < 0.0f || y < 0.0f || x >= width || y >= height;
			if (!(wParam & MK_LBUTTON) || outside)
			{
				m_nDraggingSettingSlider = -1;
				if (GetCapture() == hWnd) ReleaseCapture();
				return(true);
			}
			UpdateSettingSliderFromCursor(m_nDraggingSettingSlider, x, width, height);
			return(true);
		}
		if (m_bPetScrollDragging)
		{
			const bool outside = x < 0.0f || y < 0.0f || x >= width || y >= height;
			if (!(wParam & MK_LBUTTON) || outside || m_nMaximumPetScrollOffset == 0)
			{
				m_bPetScrollDragging = false;
				if (GetCapture() == hWnd) ReleaseCapture();
				return(true);
			}

			const XMFLOAT4 track = GetPetScrollTrackRectangle(width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
			const XMFLOAT4 thumb = GetPetScrollThumbRectangle(width, height);
			const float pageDistance = ((track.w - track.y) - (thumb.w - thumb.y))
				/ static_cast<float>(m_nMaximumPetScrollOffset);
			const float delta = y - m_fPetScrollDragLastY;
			const int steps = (pageDistance > 0.0f) ? static_cast<int>(fabsf(delta) / pageDistance) : 0;
			if (steps > 0)
			{
				if (delta > 0.0f)
				{
					const size_t remaining = m_nMaximumPetScrollOffset - m_nPetScrollOffset;
					m_nPetScrollOffset += (static_cast<size_t>(steps) < remaining)
						? static_cast<size_t>(steps) : remaining;
					m_fPetScrollDragLastY += pageDistance * steps;
				}
				else
				{
					const size_t move = (static_cast<size_t>(steps) < m_nPetScrollOffset)
						? static_cast<size_t>(steps) : m_nPetScrollOffset;
					m_nPetScrollOffset -= move;
					m_fPetScrollDragLastY -= pageDistance * steps;
				}
			}
			return(true);
		}
		if (m_bShopBoardDragging)
		{
			const bool outside = x < 0.0f || y < 0.0f || x >= width || y >= height;
			if (!(wParam & MK_LBUTTON) || outside)
			{
				m_bShopBoardDragging = false;
				if (GetCapture() == hWnd) ReleaseCapture();
				return(true);
			}
			m_xmf2ShopBoardOffset.x += x - m_xmf2ShopDragLastCursor.x;
			m_xmf2ShopBoardOffset.y += y - m_xmf2ShopDragLastCursor.y;
			m_xmf2ShopDragLastCursor = XMFLOAT2(x, y);
			return(true);
		}
		if (m_bSettingBoardDragging)
		{
			const bool outside = x < 0.0f || y < 0.0f || x >= width || y >= height;
			if (!(wParam & MK_LBUTTON) || outside)
			{
				m_bSettingBoardDragging = false;
				if (GetCapture() == hWnd) ReleaseCapture();
				return(true);
			}
			m_xmf2SettingBoardOffset.x += x - m_xmf2SettingDragLastCursor.x;
			m_xmf2SettingBoardOffset.y += y - m_xmf2SettingDragLastCursor.y;
			m_xmf2SettingDragLastCursor = XMFLOAT2(x, y);
			return(true);
		}
		break;
	case WM_LBUTTONUP:
		if (m_nDraggingSettingSlider >= 0)
		{
			m_nDraggingSettingSlider = -1;
			if (GetCapture() == hWnd) ReleaseCapture();
			return(true);
		}
		if (m_bStockIssueButtonPressed)
		{
			m_bStockIssueButtonPressed = false;
			if (m_bShopActive && m_eShopPage == SHOP_PAGE::STOCK_MANAGEMENT
				&& IsPointInRectangle(x, y, GetStockIssuanceButtonRectangle(width, height))
				&& !m_wstrStockName.empty())
			{
				PlayUiClickSound();
				m_wstrPendingStockIssueName = m_wstrStockName;
				m_bPendingStockIssueRequest = true;
			}
			if (GetCapture() == hWnd) ReleaseCapture();
			return(true);
		}
		if (m_nPressedEnhanceButton >= 0)
		{
			const int pressedButton = m_nPressedEnhanceButton;
			m_nPressedEnhanceButton = -1;
			if (m_bShopActive && m_eShopPage == SHOP_PAGE::PET_ENHANCE
				&& IsPointInRectangle(x, y, GetEnhanceButtonRectangle(pressedButton, width, height)))
			{
				PlayUiClickSound();
				m_nPendingEnhancementType = pressedButton;
			}
			if (GetCapture() == hWnd) ReleaseCapture();
			return(true);
		}
		if (m_bPetScrollDragging)
		{
			m_bPetScrollDragging = false;
			if (GetCapture() == hWnd) ReleaseCapture();
			return(true);
		}
		if (m_bShopBoardDragging)
		{
			m_bShopBoardDragging = false;
			if (GetCapture() == hWnd) ReleaseCapture();
			return(true);
		}
		if (m_bSettingBoardDragging)
		{
			m_bSettingBoardDragging = false;
			if (GetCapture() == hWnd) ReleaseCapture();
			return(true);
		}
		break;
	}
	return(false);
}
