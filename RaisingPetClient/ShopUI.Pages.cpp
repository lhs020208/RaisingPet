// shop page rendering for ShopUI.cpp
// This file is intentionally included by ShopUI.cpp to keep helper functions in one translation unit.

void CShopUI::RenderEnhancementPage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	CPet* activePet, UINT money, const SHOP_TEXT_RENDER_CONTEXT& context)
{
	if (!activePet) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	auto nextValue = [](UINT value, int type) -> UINT
	{
		const UINT maxValue = (type == 0) ? 1000 : 10000;
		if (value >= maxValue) return value;
		UINT64 enhanced = 0;
		if (type == 0)
			enhanced = (value < 100) ? static_cast<UINT64>(value) + 1
				: (static_cast<UINT64>(value) * 101 + 99) / 100;
		else
			enhanced = (value < 1000) ? static_cast<UINT64>(value) + 10
				: (static_cast<UINT64>(value) * 101 + 99) / 100;
		if (enhanced > maxValue) enhanced = maxValue;
		return static_cast<UINT>(enhanced);
	};
	auto enhancementPrice = [](UINT value, int type) -> UINT
	{
		const UINT maxValue = (type == 0) ? 1000 : 10000;
		if (value >= maxValue) return UINT_MAX;
		const UINT64 price = (type == 0)
			? 100 + (static_cast<UINT64>(value) * value * 7 + 9) / 10
			: 50 + (static_cast<UINT64>(value) * 3 + 1) / 2;
		return static_cast<UINT>((price > UINT_MAX) ? UINT_MAX : price);
	};
	auto measureText = [&context](const std::string& text, float scale) -> float
	{
		if (!context.pGlyphResources) return(0.0f);
		const float gap = (scale * 8.0f > 1.0f) ? scale * 8.0f : 1.0f;
		const float space = scale * 70.0f;
		float result = 0.0f;
		for (char ch : text)
		{
			if (ch == ' ') { result += space; continue; }
			for (const TEXT_GLYPH_RESOURCE& glyph : *context.pGlyphResources)
				if (glyph.ch == ch) { result += glyph.fPixelWidth * scale + gap; break; }
		}
		if (!text.empty() && result >= gap) result -= gap;
		return result;
	};

	const UINT currentValues[2] = { activePet->GetPay(), activePet->GetMaxPossession() };
	for (int type = 0; type < 2; ++type)
	{
		const XMFLOAT4 panel = GetPetContentPanelRectangle(type != 0, width, height,
			m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
		const float panelWidth = panel.z - panel.x;
		const float panelHeight = panel.w - panel.y;
		RenderUiImage(commandList, camera, m_EmptySquareResources[type], panel);

		const float logWidth = panelWidth * 0.84f;
		const float logHeight = logWidth * (193.0f / 737.0f);
		const float logLeft = (panel.x + panel.z - logWidth) * 0.5f;
		RenderUiImage(commandList, camera, m_PetEnhanceLogResources[type],
			XMFLOAT4(logLeft, panel.y + panelHeight * 0.05f,
				logLeft + logWidth, panel.y + panelHeight * 0.05f + logHeight));

		const float frameWidth = panelWidth * 0.75f;
		const float frameHeight = frameWidth * (162.0f / 743.0f);
		const float frameLeft = (panel.x + panel.z - frameWidth) * 0.5f;
		const float frameTops[2] = { panel.y + panelHeight * 0.30f, panel.y + panelHeight * 0.56f };
		const bool maxEnhanced = (type == 0) ? (currentValues[type] >= 1000) : (currentValues[type] >= 10000);
		const UINT values[2] = { currentValues[type], nextValue(currentValues[type], type) };
		for (int frameIndex = 0; frameIndex < 2; ++frameIndex)
		{
			const XMFLOAT4 frame(frameLeft, frameTops[frameIndex], frameLeft + frameWidth,
				frameTops[frameIndex] + frameHeight);
			RenderUiImage(commandList, camera, m_EmptyFrameResource, frame);
			const std::string text = (maxEnhanced && frameIndex == 1)
				? "-"
				: (((type == 1)
					? FormatPossessionTwoDecimals(values[frameIndex]) : FormatPossession(values[frameIndex]))
					+ ((type == 0) ? "$ / S" : "$"));
			const float scale = 0.12f;
			const float textLeft = (frame.x + frame.z - measureText(text, scale)) * 0.5f;
			const float visibleHeight = 190.0f * scale;
			const float textTop = frame.y + ((frame.w - frame.y - visibleHeight) * 0.5f)
				+ 129.0f * scale - frameHeight * 0.35f;
			RenderTextLine(commandList, camera, text, textLeft, textTop, scale, 0x00000000, context);
		}

		const UINT buttonTint = (m_nPressedEnhanceButton == type) ? 0x00BFBFBF : 0x00FFFFFF;
		RenderUiImage(commandList, camera, m_PetEnhanceButtonResource,
			GetEnhanceButtonRectangle(type, width, height), buttonTint);
		const XMFLOAT4 priceFrame = GetEnhancePriceRectangle(type, width, height);
		RenderUiImage(commandList, camera, m_PetEnhancePriceFrameResource, priceFrame);
		const UINT price = enhancementPrice(currentValues[type], type);
		const std::string priceText = maxEnhanced ? "-" : (FormatPossession(price) + "$");
		const float priceScale = 0.12f;
		const float priceLeft = (priceFrame.x + priceFrame.z - measureText(priceText, priceScale)) * 0.5f;
		const float priceVisibleHeight = 190.0f * priceScale;
		const float priceTop = priceFrame.y + ((priceFrame.w - priceFrame.y - priceVisibleHeight) * 0.5f)
			+ 129.0f * priceScale - (priceFrame.w - priceFrame.y) * 0.38f;
		RenderTextLine(commandList, camera, priceText, priceLeft, priceTop, priceScale,
			(!maxEnhanced && money < price) ? 0x00FF0000 : 0x00000000, context);
	}
}

void CShopUI::RenderFinancialPage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	UINT money, const SHOP_TEXT_RENDER_CONTEXT& context)
{
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	const XMFLOAT4 bank = GetFinancialBankFrameRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	RenderUiImage(commandList, camera, m_BankFrameResource, bank);

	for (int category = 0; category < 2; ++category)
	{
		RenderUiImage(commandList, camera, m_FinancialCategoryButtonResources[category],
			GetFinancialCategoryButtonRectangle(category, width, height),
			(category == m_nFinancialCategory) ? 0x00BFBFBF : 0x00FFFFFF);
	}

	const int productIndex = m_nFinancialProductIndices[m_nFinancialCategory];
	const bool categoryLocked = m_bFinancialProductActive[m_nFinancialCategory];
	const UINT arrowTint = (categoryLocked || productIndex == 0) ? 0x00BFBFBF : 0x00FFFFFF;
	const UINT rightArrowTint = (categoryLocked || productIndex == 9 ||
		productIndex >= m_nFinancialMaximumProductIndices[m_nFinancialCategory]) ? 0x00BFBFBF : 0x00FFFFFF;
	RenderUiImage(commandList, camera, m_FinancialLeftButtonResource,
		GetFinancialLeftButtonRectangle(width, height), arrowTint);
	RenderUiImage(commandList, camera, m_FinancialRightButtonResource,
		GetFinancialRightButtonRectangle(width, height), rightArrowTint);
	RenderUiImage(commandList, camera, m_FinancialProductNameResources[m_nFinancialCategory][productIndex],
		GetFinancialProductNameRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y),
		(categoryLocked && m_nActiveFinancialProductIndex[m_nFinancialCategory] == productIndex)
		? 0x00BFBFBF : 0x00FFFFFF);

	const XMFLOAT4 timerFrame = GetFinancialTimerFrameRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	RenderUiImage(commandList, camera, m_FinancialTimerFrameResource, timerFrame);

	const FINANCIAL_PRODUCT_DATA& product = (m_nFinancialCategory == 0)
		? gInstallmentSavingsProducts[productIndex] : gLoanProducts[productIndex];
	auto renderCenteredText = [this, commandList, camera, &context]
		(const std::string& text, const XMFLOAT4& rect, float scale, UINT color)
	{
		const float textLeft = (rect.x + rect.z - MeasureShopTextWidth(text, scale, context)) * 0.5f;
		const float visibleHeight = 190.0f * scale;
		const float textTop = rect.y + ((rect.w - rect.y - visibleHeight) * 0.5f)
			+ 129.0f * scale - (rect.w - rect.y) * 0.32f;
		RenderTextLine(commandList, camera, text, textLeft, textTop, scale, color, context);
	};

	UINT displaySeconds = product.nDurationSeconds;
	if (categoryLocked)
	{
		const UINT elapsedSeconds = static_cast<UINT>(m_fActiveFinancialElapsedSeconds[m_nFinancialCategory]);
		const UINT durationSeconds = m_nActiveFinancialDurationSeconds[m_nFinancialCategory];
		displaySeconds = (elapsedSeconds >= durationSeconds) ? 0 : (durationSeconds - elapsedSeconds);
	}
	renderCenteredText(FormatFinancialDuration(displaySeconds), timerFrame, 0.105f, 0x00000000);

	const XMFLOAT4 moneyFrames[2] =
	{
		GetFinancialMoneyFrameRectangle(0, width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y),
		GetFinancialMoneyFrameRectangle(1, width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y)
	};
	RenderUiImage(commandList, camera, m_FinancialMoneyFrameResource, moneyFrames[0]);
	RenderUiImage(commandList, camera, m_FinancialMoneyFrameResource, moneyFrames[1]);
	RenderUiImage(commandList, camera, m_FinancialRightPointResource,
		GetFinancialRightPointRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));

	const bool firstIncome = (m_nFinancialCategory == 1);
	const bool secondIncome = (m_nFinancialCategory == 0);
	renderCenteredText(FormatFinancialMoney(product.nFirstMoney, firstIncome), moneyFrames[0],
		0.115f, firstIncome ? 0x0000B050 : 0x00FF0000);
	renderCenteredText(FormatFinancialMoney(product.nSecondMoney, secondIncome), moneyFrames[1],
		0.115f, secondIncome ? 0x0000B050 : 0x00FF0000);

	RenderUiImage(commandList, camera, m_FinancialApplicationButtonResource,
		GetFinancialApplicationButtonRectangle(width, height, money, context),
		IsFinancialApplicationButtonDisabled(money) ? 0x00BFBFBF : 0x00FFFFFF);
	RenderFinancialFailLogs(commandList, camera, money, context);
}

void CShopUI::RenderStockMenuPage(ID3D12GraphicsCommandList* commandList, CCamera* camera)
{
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	for (int i = 0; i < 2; ++i)
		RenderUiImage(commandList, camera, m_StockSlotResources[i], GetStockSlotRectangle(i, width, height));
}

void CShopUI::RenderStockTransactionPage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	UINT money, const SHOP_TEXT_RENDER_CONTEXT& context)
{
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	const XMFLOAT4 leftPanel = GetPetContentPanelRectangle(false, width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const XMFLOAT4 rightPanel = GetPetContentPanelRectangle(true, width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);

	RenderUiImage(commandList, camera, m_EmptySquareResources[0], leftPanel);
	RenderUiImage(commandList, camera, m_EmptySquareResources[1], rightPanel);

	const XMFLOAT4 scrollTrack = GetPetScrollTrackRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	RenderUiImage(commandList, camera, m_ScrollBackgroundResource, scrollTrack);
	RenderUiImage(commandList, camera, m_ScrollResource,
		GetStockTransactionScrollThumbRectangle(width, height));

	if (g_pFramework)
	{
		const float rowHeight = (leftPanel.w - leftPanel.y) / 10.0f;
		const float listFontSize = rowHeight * 0.58f;
		for (size_t row = 0; row < 10; ++row)
		{
			const size_t stockIndex = m_nStockTransactionScrollOffset + row;
			if (stockIndex >= m_StockTransactionInfos.size()) break;
			if (stockIndex == m_nSelectedStockTransactionIndex)
			{
				const XMFLOAT4 selection = GetStockTransactionListRowRectangle(row, width, height);
				RenderSolidUiRectangle(commandList, camera, selection.x, selection.y,
					selection.z, selection.w, 0x00BFBFBF, context);
			}
			const std::wstring text = std::to_wstring(stockIndex + 1) + L". "
				+ m_StockTransactionInfos[stockIndex].wstrStockName;
			QueueShopDirectWriteText(text,
				XMFLOAT4(leftPanel.x + 14.0f, leftPanel.y + rowHeight * row,
					scrollTrack.x - 5.0f, leftPanel.y + rowHeight * (row + 1)),
				listFontSize, 0xFF000000, false, true);
		}
	}

	const bool hasSelectedStock = !m_StockTransactionInfos.empty()
		&& m_nSelectedStockTransactionIndex < m_StockTransactionInfos.size();
	const SHOP_STOCK_TRANSACTION_INFO selectedStock = hasSelectedStock
		? m_StockTransactionInfos[m_nSelectedStockTransactionIndex]
		: SHOP_STOCK_TRANSACTION_INFO();
	const std::wstring emptyText = L"-";
	const std::wstring stockName = hasSelectedStock ? selectedStock.wstrStockName : emptyText;
	const std::wstring issuerId = hasSelectedStock ? selectedStock.wstrIssuerId : emptyText;
	const std::wstring saleableQuantity = hasSelectedStock
		? std::to_wstring(selectedStock.nSaleableQuantity) : emptyText;
	const std::wstring myQuantity = hasSelectedStock
		? std::to_wstring(selectedStock.nMyQuantity) : emptyText;
	const std::wstring recentTradeQuantity = hasSelectedStock
		? std::to_wstring(selectedStock.nRecentTradeQuantity) : emptyText;

	const float rightWidth = rightPanel.z - rightPanel.x;
	const float rightHeight = rightPanel.w - rightPanel.y;
	const float imageWidth = rightWidth * 0.94f;
	const float gap = rightHeight * 0.018f;
	const float titleHeight = imageWidth * (140.0f / 1095.0f);
	const float issuerHeight = imageWidth * (148.0f / 1095.0f);
	const float graphHeight = imageWidth * (459.0f / 1092.0f);
	const float descriptionHeight = imageWidth * (310.0f / 1095.0f);
	const float totalHeight = titleHeight + issuerHeight + graphHeight + descriptionHeight + gap * 3.0f;
	const float imageLeft = (rightPanel.x + rightPanel.z - imageWidth) * 0.5f;
	float top = rightPanel.y + (rightHeight - totalHeight) * 0.5f;
	XMFLOAT4 titleRect(imageLeft, top, imageLeft + imageWidth, top + titleHeight);
	top = titleRect.w + gap;
	XMFLOAT4 issuerRect(imageLeft, top, imageLeft + imageWidth, top + issuerHeight);
	top = issuerRect.w + gap;
	XMFLOAT4 graphRect(imageLeft, top, imageLeft + imageWidth, top + graphHeight);
	top = graphRect.w + gap;
	XMFLOAT4 descriptionRect(imageLeft, top, imageLeft + imageWidth, top + descriptionHeight);

	RenderUiImage(commandList, camera, m_StockTransactionTitleResource, titleRect);
	RenderUiImage(commandList, camera, m_StockTransactionIssuerResource, issuerRect);
	RenderUiImage(commandList, camera, m_StockTransactionGraphResource, graphRect);
	RenderUiImage(commandList, camera, m_StockTransactionDescriptionResource, descriptionRect);

	const float seeStockWidth = (graphRect.z - graphRect.x) * 0.55f;
	const float seeStockHeight = seeStockWidth * (209.0f / 678.0f);
	const float seeStockCenterY = graphRect.y + (graphRect.w - graphRect.y) * 0.40f;
	RenderUiImage(commandList, camera, m_SeeStockResource,
		XMFLOAT4((graphRect.x + graphRect.z - seeStockWidth) * 0.5f,
			seeStockCenterY - seeStockHeight * 0.5f,
			(graphRect.x + graphRect.z + seeStockWidth) * 0.5f,
			seeStockCenterY + seeStockHeight * 0.5f));

	if (g_pFramework)
	{
		QueueShopDirectWriteText(stockName, titleRect,
			(titleRect.w - titleRect.y) * 0.45f, 0xFF000000, true, true);
		QueueShopDirectWriteText(issuerId,
			XMFLOAT4(issuerRect.x + (issuerRect.z - issuerRect.x) * 0.31f, issuerRect.y,
				issuerRect.z - 5.0f, issuerRect.w),
			(issuerRect.w - issuerRect.y) * 0.42f, 0xFF000000, true, true);

		const UINT currentPrice = hasSelectedStock ? selectedStock.nCurrentPrice : 0;
		const UINT previousPrice = hasSelectedStock ? selectedStock.nPreviousPrice : 0;
		const bool priceUpOrSame = (currentPrice >= previousPrice);
		const UINT priceTextColor = priceUpOrSame ? 0xFFFF0000 : 0xFF0070C0;
		const float priceTop = graphRect.y + (graphRect.w - graphRect.y) * 0.79f;
		const float priceBottom = graphRect.w - (graphRect.w - graphRect.y) * 0.04f;
		QueueShopDirectWriteText(hasSelectedStock ? FormatStockPrice(currentPrice) : emptyText,
			XMFLOAT4(graphRect.x + (graphRect.z - graphRect.x) * 0.28f, priceTop,
				graphRect.x + (graphRect.z - graphRect.x) * 0.50f, priceBottom),
			(priceBottom - priceTop) * 0.66f, hasSelectedStock ? priceTextColor : 0xFF000000,
			false, true);
		if (hasSelectedStock)
		{
			const float markSize = (priceBottom - priceTop) * 0.55f;
			const float markLeft = graphRect.x + (graphRect.z - graphRect.x) * 0.52f;
			const float markTop = priceTop + (priceBottom - priceTop - markSize) * 0.5f;
			RenderUiImage(commandList, camera,
				priceUpOrSame ? m_StockUpMarkResource : m_StockDownMarkResource,
				XMFLOAT4(markLeft, markTop, markLeft + markSize, markTop + markSize));
			QueueShopDirectWriteText(FormatStockPriceChangeText(currentPrice, previousPrice),
				XMFLOAT4(markLeft + markSize + 2.0f, priceTop, graphRect.z - 3.0f, priceBottom),
				(priceBottom - priceTop) * 0.42f, priceTextColor, false, true);
		}
		else
		{
			QueueShopDirectWriteText(emptyText,
				XMFLOAT4(graphRect.x + (graphRect.z - graphRect.x) * 0.60f, priceTop,
					graphRect.z - 3.0f, priceBottom),
				(priceBottom - priceTop) * 0.54f, 0xFF000000, false, true);
		}

		const float descWidth = descriptionRect.z - descriptionRect.x;
		const float descHeight = descriptionRect.w - descriptionRect.y;
		const float descFont = descHeight * 0.19f;
		const float descLine = descHeight * 0.255f;
		const float labelLeft = descriptionRect.x + descWidth * 0.035f;
		const float valueLeft = descriptionRect.x + descWidth * 0.35f;
		const float labelTop = descriptionRect.y + descHeight * 0.13f;
		const wchar_t* labels[3] = { L"\uD310\uB9E4 \uAC00\uB2A5", L"\uB0B4 \uBCF4\uC720", L"\uCD5C\uADFC \uAC70\uB798" };
		const std::wstring values[3] = { saleableQuantity, myQuantity, recentTradeQuantity };
		for (int i = 0; i < 3; ++i)
		{
			const float y = labelTop + descLine * i;
			QueueShopDirectWriteText(labels[i],
				XMFLOAT4(labelLeft, y, valueLeft, y + descLine),
				descFont, 0xFF000000, false, true);
			const std::wstring valueText = L":" + values[i] + (hasSelectedStock ? L"\uC8FC" : L"");
			QueueShopDirectWriteText(valueText,
				XMFLOAT4(valueLeft, y, descriptionRect.x + descWidth * 0.60f, y + descLine),
				descFont, 0xFF000000, false, true);
		}

		const XMFLOAT4 quantityRect(descriptionRect.x + descWidth * 0.66f,
			descriptionRect.y + descHeight * 0.09f,
			descriptionRect.x + descWidth * 0.88f,
			descriptionRect.y + descHeight * 0.40f);
		const XMFLOAT4 priceRect(descriptionRect.x + descWidth * 0.66f,
			descriptionRect.y + descHeight * 0.55f,
			descriptionRect.x + descWidth * 0.94f,
			descriptionRect.y + descHeight * 0.86f);
		QueueShopDirectWriteText(hasSelectedStock
			? std::to_wstring(m_nStockTransactionOrderQuantity) : emptyText,
			quantityRect, (quantityRect.w - quantityRect.y) * 0.68f, 0xFF000000, true, true);
		if (hasSelectedStock && m_bStockTransactionQuantityInputActive
			&& m_eShopPage == SHOP_PAGE::STOCK_TRANSACTION)
			RenderStockQuantityCursor(commandList, camera, quantityRect,
				m_nStockTransactionOrderQuantity, (quantityRect.w - quantityRect.y) * 0.68f);
		if (hasSelectedStock)
		{
			QueueShopDirectWriteText(L"\uC8FC",
				XMFLOAT4(quantityRect.z, quantityRect.y, descriptionRect.z, quantityRect.w),
				(quantityRect.w - quantityRect.y) * 0.60f, 0xFF000000, true, true);
		}
		const UINT64 totalPrice = static_cast<UINT64>(m_nStockTransactionOrderQuantity)
			* static_cast<UINT64>(hasSelectedStock ? selectedStock.nCurrentPrice : 0);
		QueueShopDirectWriteText(hasSelectedStock
			? ToWideString(FormatPossessionTwoDecimals(
				static_cast<UINT>((totalPrice > UINT_MAX) ? UINT_MAX : totalPrice)))
			: emptyText,
			priceRect, (priceRect.w - priceRect.y) * 0.60f, 0xFF000000, true, true);
	}

	RenderUiImage(commandList, camera, m_StockBuyingResource,
		GetStockTransactionBuyingButtonRectangle(width, height, context),
		IsStockTradeButtonDisabled(0, money) ? 0x00BFBFBF : 0x00FFFFFF);
	RenderUiImage(commandList, camera, m_StockSellingResource,
		GetStockTransactionSellingButtonRectangle(width, height, context),
		IsStockTradeButtonDisabled(1, money) ? 0x00BFBFBF : 0x00FFFFFF);
}

void CShopUI::RenderStockManagementPage(ID3D12GraphicsCommandList* commandList, CCamera* camera)
{
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;

	const float contentLeft = board.x + boardWidth * 0.06f;
	const float contentTop = board.y + boardHeight * 0.16f;
	const float leftColumnWidth = boardWidth * 0.52f;
	const float rightColumnWidth = boardWidth * 0.30f;
	const float columnGap = boardWidth * 0.055f;
	const float rightColumnLeft = contentLeft + leftColumnWidth + columnGap;

	const float stockNameWidth = leftColumnWidth;
	const float stockNameHeight = stockNameWidth * (275.0f / 1341.0f);
	const XMFLOAT4 stockNameRect(contentLeft, contentTop,
		contentLeft + stockNameWidth, contentTop + stockNameHeight);
	RenderUiImage(commandList, camera, m_StockNameResource, stockNameRect);
	if (g_pFramework)
	{
		const XMFLOAT4 inputRect = GetStockNameInputRectangle(width, height);
		const float inputHeight = inputRect.w - inputRect.y;
		const float textPaddingX = inputHeight * 0.35f;
		const float fontSize = GetStockNameInputFontSize(width, height);
		const XMFLOAT4 textRect(inputRect.x + textPaddingX, inputRect.y,
			inputRect.z - textPaddingX, inputRect.w);
		if (!m_wstrStockName.empty())
		{
			QueueShopDirectWriteText(m_wstrStockName, textRect, fontSize,
				0xFF000000, false, true);
		}
		if (m_bStockNameInputActive && m_fStockNameCursorBlinkElapsed < 0.5f)
		{
			const std::wstring leftText = m_wstrStockName.substr(0,
				min(m_nStockNameCursorIndex, m_wstrStockName.size()));
			const float cursorOffset = MeasureStockNameTextWidth(leftText, fontSize,
				textRect.z - textRect.x, textRect.w - textRect.y);
			const float cursorX = min(textRect.x + cursorOffset + 1.0f, textRect.z);
			const float cursorMarginY = inputHeight * 0.22f;
			g_pFramework->QueueDirectWriteSolidRectangle(
				XMFLOAT4(cursorX, inputRect.y + cursorMarginY,
					cursorX + 2.0f, inputRect.w - cursorMarginY),
				0xFF000000);
		}
	}

	const float holdersTop = stockNameRect.w + boardHeight * 0.035f;
	const float holdersHeight = stockNameWidth * (534.0f / 1341.0f);
	const XMFLOAT4 holdersRect(contentLeft, holdersTop,
		contentLeft + stockNameWidth, holdersTop + holdersHeight);
	RenderUiImage(commandList, camera, m_StockHoldersResource, holdersRect);

	const float tableGapX = boardWidth * 0.012f;
	const float tableGapY = boardHeight * 0.012f;
	const float tableWidth = (leftColumnWidth - tableGapX) * 0.5f;
	const float tableHeight = tableWidth * (141.0f / 775.0f);
	const float tableTop = holdersRect.w + boardHeight * 0.03f;
	XMFLOAT4 tableRects[4];
	for (int row = 0; row < 2; ++row)
	{
		for (int col = 0; col < 2; ++col)
		{
			const float left = contentLeft + col * (tableWidth + tableGapX);
			const float top = tableTop + row * (tableHeight + tableGapY);
			const int tableIndex = row * 2 + col;
			tableRects[tableIndex] = XMFLOAT4(left, top, left + tableWidth, top + tableHeight);
			RenderUiImage(commandList, camera, m_StockManagementTableResource, tableRects[tableIndex]);
		}
	}

	const float issuanceWidth = rightColumnWidth * 0.64f;
	const float issuanceHeight = issuanceWidth * (275.0f / 500.0f);
	const float issuanceLeft = rightColumnLeft + (rightColumnWidth - issuanceWidth) * 0.5f;
	const XMFLOAT4 issuanceRect(issuanceLeft, contentTop,
		issuanceLeft + issuanceWidth, contentTop + issuanceHeight);
	RenderUiImage(commandList, camera, m_IssuanceStockResource, issuanceRect,
		(m_bStockIssued || m_bStockIssueButtonPressed) ? 0x00BFBFBF : 0x00FFFFFF);

	const XMFLOAT4 chartRect = GetStockChartRectangle(width, height);
	RenderUiImage(commandList, camera, m_StockChartResource, chartRect);
	RenderUiImage(commandList, camera, m_SeeGraphResource,
		GetStockGraphButtonRectangle(camera->m_d3dViewport.Width, camera->m_d3dViewport.Height),
		m_bStockIssued ? 0x00FFFFFF : 0x00BFBFBF);

	if (g_pFramework)
	{
		const UINT currentPrice = (m_StockManagementInfo.nCurrentPrice > 0)
			? m_StockManagementInfo.nCurrentPrice : 100;
		const UINT previousPrice = (m_StockManagementInfo.nPreviousPrice > 0)
			? m_StockManagementInfo.nPreviousPrice : currentPrice;
		const bool priceUpOrSame = (currentPrice >= previousPrice);
		const UINT priceTextColor = priceUpOrSame ? 0xFFFF0000 : 0xFF0070C0;
		const float priceTop = chartRect.y + (chartRect.w - chartRect.y) * 0.805f;
		const float priceBottom = chartRect.w - (chartRect.w - chartRect.y) * 0.035f;
		const float priceFontSize = boardHeight * 0.041f;
		const float currentPriceLeft = chartRect.x + (chartRect.z - chartRect.x) * 0.17f;
		const float currentPriceRight = chartRect.x + (chartRect.z - chartRect.x) * 0.44f;
		QueueShopDirectWriteText(FormatStockPrice(currentPrice),
			XMFLOAT4(currentPriceLeft, priceTop, currentPriceRight, priceBottom),
			priceFontSize, priceTextColor, false, true);
		const float markSize = (priceBottom - priceTop) * 0.62f;
		const float markLeft = chartRect.x + (chartRect.z - chartRect.x) * 0.48f;
		const float markTop = priceTop + (priceBottom - priceTop - markSize) * 0.5f;
		RenderUiImage(commandList, camera,
			priceUpOrSame ? m_StockUpMarkResource : m_StockDownMarkResource,
			XMFLOAT4(markLeft, markTop, markLeft + markSize, markTop + markSize));
		QueueShopDirectWriteText(FormatStockPriceChangeText(currentPrice, previousPrice),
			XMFLOAT4(markLeft + markSize + 3.0f, priceTop, chartRect.z - 3.0f, priceBottom),
			boardHeight * 0.0245f, priceTextColor, false, true);

		const float holderLineHeight = (holdersRect.w - holdersRect.y) * 0.225f;
		const float holderTextLeft = holdersRect.x + (holdersRect.z - holdersRect.x) * 0.055f;
		const float holderTextRight = holdersRect.z - (holdersRect.z - holdersRect.x) * 0.05f;
		const float holderTextTop = holdersRect.y + (holdersRect.w - holdersRect.y) * 0.285f;
		const float holderQuantityLeft = holdersRect.x + (holdersRect.z - holdersRect.x) * 0.58f;
		const UINT holderColors[3] = { 0xFFFFC000, 0xFF7F7F7F, 0xFFA64D00 };
		for (int i = 0; i < 3; ++i)
		{
			const SHOP_STOCK_HOLDER_INFO& holder = m_StockManagementInfo.TopHolders[i];
			const std::wstring holderName = holder.wstrPlayerId.empty() ? L"-" : holder.wstrPlayerId;
			const std::wstring holderRankText = std::to_wstring(i + 1) + L". " + holderName;
			const std::wstring holderQuantityText = L":" + std::to_wstring(holder.nQuantity) + L"\uC8FC";
			const float holderFontSize = boardHeight * ((i == 0) ? 0.044f : 0.037f);
			QueueShopDirectWriteText(holderRankText,
				XMFLOAT4(holderTextLeft, holderTextTop + holderLineHeight * i,
					holderQuantityLeft, holderTextTop + holderLineHeight * (i + 1)),
				holderFontSize, holderColors[i], false, true);
			QueueShopDirectWriteText(holderQuantityText,
				XMFLOAT4(holderQuantityLeft, holderTextTop + holderLineHeight * i,
					holderTextRight, holderTextTop + holderLineHeight * (i + 1)),
				holderFontSize, holderColors[i], false, true);
		}

		const wchar_t* tableLabels[4] =
		{
			L"\uD310\uB9E4\uB428",
			L"\uBC1C\uD589\uC218\uC775",
			L"\uBBF8\uD310\uB9E4",
			L"\uCD5C\uADFC\uAC70\uB798"
		};
		const UINT saleableQuantity = (m_StockManagementInfo.nSaleableQuantity > 0)
			? m_StockManagementInfo.nSaleableQuantity : 800;
		const std::wstring tableValues[4] =
		{
			FormatStockQuantity(m_StockManagementInfo.nSoldQuantity, saleableQuantity),
			FormatStockRevenue(m_StockManagementInfo.nIssuanceRevenue),
			FormatStockQuantity(m_StockManagementInfo.nUnsoldQuantity, saleableQuantity),
			FormatStockTradeQuantity(m_StockManagementInfo.nRecentTradeQuantity)
		};
		for (int i = 0; i < 4; ++i)
		{
			const XMFLOAT4& table = tableRects[i];
			const float tableWidth = table.z - table.x;
			const float splitX = table.x + tableWidth * 0.38f;
			const float cellPaddingX = tableWidth * 0.025f;
			const XMFLOAT4 labelRect(table.x + cellPaddingX, table.y,
				splitX - cellPaddingX, table.w);
			const XMFLOAT4 valueRect(splitX + cellPaddingX, table.y,
				table.z - cellPaddingX, table.w);
			QueueShopDirectWriteText(tableLabels[i], labelRect,
				boardHeight * 0.030f, 0xFF000000, true, true);
			QueueShopDirectWriteText(tableValues[i], valueRect,
				boardHeight * 0.027f, 0xFF000000, true, true);
		}
	}
	else
	{
		OutputDebugStringW(L"[ShopUI] g_pFramework is null. Cannot queue DirectWrite texts.\n");
	}
}

XMFLOAT4 CShopUI::GetStockChartRectangle(float width, float height) const
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float contentLeft = board.x + boardWidth * 0.06f;
	const float contentTop = board.y + boardHeight * 0.16f;
	const float leftColumnWidth = boardWidth * 0.52f;
	const float rightColumnWidth = boardWidth * 0.30f;
	const float columnGap = boardWidth * 0.055f;
	const float rightColumnLeft = contentLeft + leftColumnWidth + columnGap;
	const float issuanceWidth = rightColumnWidth * 0.64f;
	const float issuanceHeight = issuanceWidth * (275.0f / 500.0f);
	const float chartWidth = rightColumnWidth;
	const float chartHeight = chartWidth * (673.0f / 774.0f);
	const float chartTop = contentTop + issuanceHeight + boardHeight * 0.035f;
	return(XMFLOAT4(rightColumnLeft, chartTop, rightColumnLeft + chartWidth, chartTop + chartHeight));
}

XMFLOAT4 CShopUI::GetStockGraphButtonRectangle(float width, float height) const
{
	const XMFLOAT4 chartRect = GetStockChartRectangle(width, height);
	const float chartWidth = chartRect.z - chartRect.x;
	const float chartHeight = chartRect.w - chartRect.y;
	const float graphAreaTop = chartRect.y + chartHeight * 0.10f;
	const float graphAreaBottom = chartRect.y + chartHeight * 0.70f;
	const float graphAreaCenterX = (chartRect.x + chartRect.z) * 0.5f;
	const float buttonWidth = chartWidth * 0.86f;
	const float buttonHeight = (graphAreaBottom - graphAreaTop) * 0.88f;
	const float buttonLeft = graphAreaCenterX - buttonWidth * 0.5f;
	const float buttonTop = graphAreaTop + (graphAreaBottom - graphAreaTop - buttonHeight) * 0.5f;
	return(XMFLOAT4(buttonLeft, buttonTop, buttonLeft + buttonWidth, buttonTop + buttonHeight));
}

XMFLOAT4 CShopUI::GetStockNameInputRectangle(float width, float height) const
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float contentLeft = board.x + boardWidth * 0.06f;
	const float contentTop = board.y + boardHeight * 0.16f;
	const float leftColumnWidth = boardWidth * 0.52f;
	const float stockNameWidth = leftColumnWidth;
	const float stockNameHeight = stockNameWidth * (275.0f / 1341.0f);
	const float dividerX = contentLeft + stockNameWidth * 0.30f;
	return(XMFLOAT4(dividerX, contentTop, contentLeft + stockNameWidth,
		contentTop + stockNameHeight));
}

XMFLOAT4 CShopUI::GetStockIssuanceButtonRectangle(float width, float height) const
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float contentLeft = board.x + boardWidth * 0.06f;
	const float contentTop = board.y + boardHeight * 0.16f;
	const float leftColumnWidth = boardWidth * 0.52f;
	const float rightColumnWidth = boardWidth * 0.30f;
	const float columnGap = boardWidth * 0.055f;
	const float rightColumnLeft = contentLeft + leftColumnWidth + columnGap;
	const float issuanceWidth = rightColumnWidth * 0.64f;
	const float issuanceHeight = issuanceWidth * (275.0f / 500.0f);
	const float issuanceLeft = rightColumnLeft + (rightColumnWidth - issuanceWidth) * 0.5f;
	return(XMFLOAT4(issuanceLeft, contentTop,
		issuanceLeft + issuanceWidth, contentTop + issuanceHeight));
}

float CShopUI::GetStockNameInputFontSize(float width, float height) const
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	return((board.w - board.y) * 0.048f);
}

float CShopUI::MeasureStockNameTextWidth(const std::wstring& text, float fontSize,
	float availableWidth, float availableHeight) const
{
	if (!g_pFramework || text.empty()) return(0.0f);
	float measuredWidth = 0.0f;
	float measuredHeight = 0.0f;
	g_pFramework->MeasureDirectWriteText(text, fontSize,
		max(availableWidth, 1.0f), max(availableHeight, 1.0f),
		measuredWidth, measuredHeight);
	return(measuredWidth);
}

void CShopUI::MoveStockNameCursorFromClick(float x, const XMFLOAT4& rectangle, float fontSize)
{
	const float inputHeight = rectangle.w - rectangle.y;
	const float textPaddingX = inputHeight * 0.35f;
	const float textLeft = rectangle.x + textPaddingX;
	const float availableWidth = rectangle.z - rectangle.x - textPaddingX * 2.0f;
	const float availableHeight = rectangle.w - rectangle.y;
	if (m_wstrStockName.empty() || x <= textLeft)
	{
		m_nStockNameCursorIndex = 0;
		return;
	}

	float previousBoundary = 0.0f;
	for (size_t index = 0; index < m_wstrStockName.size(); ++index)
	{
		const float nextBoundary = MeasureStockNameTextWidth(
			m_wstrStockName.substr(0, index + 1), fontSize, availableWidth, availableHeight);
		const float middle = textLeft + (previousBoundary + nextBoundary) * 0.5f;
		if (x < middle)
		{
			m_nStockNameCursorIndex = index;
			return;
		}
		previousBoundary = nextBoundary;
	}
	m_nStockNameCursorIndex = m_wstrStockName.size();
}

bool CShopUI::ProcessStockMenuClick(float x, float y, float width, float height)
{
	if (IsPointInRectangle(x, y, GetStockSlotRectangle(0, width, height)))
	{
		g_pFramework->PlayClickSound();
		m_eShopPage = SHOP_PAGE::STOCK_TRANSACTION;
		m_bStockNameInputActive = false;
		m_bStockTransactionQuantityInputActive = false;
		m_bPendingStockTransactionListRequest = true;
		return(true);
	}
	if (IsPointInRectangle(x, y, GetStockSlotRectangle(1, width, height)))
	{
		g_pFramework->PlayClickSound();
		m_eShopPage = m_bStockCreationAvailable ? SHOP_PAGE::STOCK_MANAGEMENT : SHOP_PAGE::STOCK_CANT_PUBLISH;
		m_bStockNameInputActive = false;
		if (m_eShopPage == SHOP_PAGE::STOCK_MANAGEMENT)
			m_bPendingStockManagementInfoRequest = true;
		return(true);
	}
	return(false);
}

void CShopUI::RenderCantCreateStockPage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	const SHOP_TEXT_RENDER_CONTEXT& context)
{
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;

	const float logWidth = boardWidth * 0.72f;
	const float logHeight = logWidth * (300.0f / 2563.0f);
	const float logLeft = (board.x + board.z - logWidth) * 0.5f;
	const float logTop = board.y + boardHeight * 0.35f;
	RenderUiImage(commandList, camera, m_CantCreateStockGenLogResource,
		XMFLOAT4(logLeft, logTop, logLeft + logWidth, logTop + logHeight));

	const float limitWidth = boardWidth * 0.405f;
	const float limitHeight = limitWidth * (259.0f / 1190.0f);
	const float limitTop = board.y + boardHeight * 0.60f;
	const float leftLimitLeft = board.x + boardWidth * 0.05f;
	const float rightLimitLeft = board.z - boardWidth * 0.05f - limitWidth;
	const XMFLOAT4 limitRects[2] =
	{
		XMFLOAT4(leftLimitLeft, limitTop, leftLimitLeft + limitWidth, limitTop + limitHeight),
		XMFLOAT4(rightLimitLeft, limitTop, rightLimitLeft + limitWidth, limitTop + limitHeight)
	};

	for (int category = 0; category < 2; ++category)
	{
		const int progress = min(m_nFinancialProgressCounts[category], 5);
		const UINT tint = (progress >= 5) ? 0x0000B050 : 0x00FF0000;
		RenderUiImage(commandList, camera, m_StockLimitResources[category], limitRects[category], tint);

		const std::string progressText = "( " + std::to_string(progress) + " / 5 )";
		const float scale = 0.105f;
		const float textLeft = limitRects[category].x + (limitRects[category].z - limitRects[category].x) * 0.47f;
		const float visibleHeight = 190.0f * scale;
		const float textTop = limitRects[category].y
			+ ((limitRects[category].w - limitRects[category].y - visibleHeight) * 0.5f)
			+ 129.0f * scale - (limitRects[category].w - limitRects[category].y) * 0.32f;
		RenderTextLine(commandList, camera, progressText, textLeft, textTop, scale, 0x00000000, context);
	}
}

void CShopUI::RenderStockGraphPage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	UINT money, const SHOP_TEXT_RENDER_CONTEXT& context)
{
	if (!camera) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float graphWidth = boardWidth * 0.86f;
	const float graphHeight = graphWidth * (1354.0f / 2553.0f);
	const float graphLeft = (board.x + board.z - graphWidth) * 0.5f;
	const float graphTop = board.y + boardHeight * 0.17f;
	const XMFLOAT4 graphRect(graphLeft, graphTop, graphLeft + graphWidth, graphTop + graphHeight);
	const bool targetGraph = (m_eShopPage == SHOP_PAGE::STOCK_SEE_TARGET_GRAPH);
	const bool hasTargetStock = !m_StockTransactionInfos.empty()
		&& m_nSelectedStockTransactionIndex < m_StockTransactionInfos.size();
	const SHOP_STOCK_TRANSACTION_INFO targetStock = hasTargetStock
		? m_StockTransactionInfos[m_nSelectedStockTransactionIndex]
		: SHOP_STOCK_TRANSACTION_INFO();
	RenderUiImage(commandList, camera, targetGraph ? m_TargetGraphResource : m_MyGraphResource, graphRect);

	if (!g_pFramework) return;

	std::vector<SHOP_STOCK_PRICE_INFO> prices;
	const std::vector<SHOP_STOCK_PRICE_INFO>& sourcePrices =
		targetGraph ? targetStock.RecentPrices : m_StockManagementInfo.RecentPrices;
	for (auto it = sourcePrices.rbegin(); it != sourcePrices.rend() && prices.size() < 10; ++it)
		prices.push_back(*it);
	if (!prices.empty()
		&& prices.size() < 10
		&& prices.front().nPreviousPrice == prices.front().nNewPrice
		&& prices.front().nNewPrice == 100)
	{
		prices.front().nPreviousPrice = 0;
	}

	UINT minimumPrice = 0;
	UINT maximumPrice = 100;
	if (!prices.empty())
	{
		minimumPrice = (prices.size() < 10) ? 0 : UINT_MAX;
		maximumPrice = 0;
		for (const SHOP_STOCK_PRICE_INFO& price : prices)
		{
			if (prices.size() >= 10)
				minimumPrice = min(minimumPrice, min(price.nPreviousPrice, price.nNewPrice));
			maximumPrice = max(maximumPrice, max(price.nPreviousPrice, price.nNewPrice));
		}
		if (minimumPrice == UINT_MAX) minimumPrice = 0;
		if (maximumPrice <= minimumPrice) maximumPrice = minimumPrice + 100;
	}

	const float graphRectWidth = graphRect.z - graphRect.x;
	const float graphRectHeight = graphRect.w - graphRect.y;
	const float plotLeft = graphRect.x + graphRectWidth * (1.59f / 19.60f);
	const float plotRight = plotLeft + graphRectWidth * (12.23f / 19.60f);
	const float infoLeft = plotRight + graphRectWidth * 0.018f;
	const float plotTop = graphRect.y + graphRectHeight * 0.064f;
	const float plotBottom = graphRect.y + graphRectHeight * 0.768f;
	const float timeTop = graphRect.y + graphRectHeight * 0.845f;
	const float labelLeft = graphRect.x + graphRectWidth * 0.012f;
	const float labelRight = plotLeft - graphRectWidth * 0.030f;
	const float plotHeight = plotBottom - plotTop;
	const float priceRange = static_cast<float>(maximumPrice - minimumPrice);
	auto priceToY = [&](UINT price) -> float
	{
		const float ratio = (priceRange > 0.0f)
			? (static_cast<float>(price - minimumPrice) / priceRange) : 0.0f;
		return(plotBottom - ratio * plotHeight);
	};

	const float axisFontSize = boardHeight * 0.025f;
	for (int i = 0; i < 5; ++i)
	{
		const float ratio = static_cast<float>(i) / 4.0f;
		const UINT value = static_cast<UINT>(minimumPrice
			+ static_cast<UINT64>(maximumPrice - minimumPrice) * i / 4);
		const float y = plotBottom - plotHeight * ratio;
		QueueShopDirectWriteText(FormatStockPrice(value),
			XMFLOAT4(labelLeft, y - axisFontSize, labelRight, y + axisFontSize),
			axisFontSize, 0xFFFFFFFF, false, true);
	}

	const float slotWidth = (plotRight - plotLeft) / 10.0f;
	const float barWidth = slotWidth * 0.62f;
	for (size_t i = 0; i < prices.size(); ++i)
	{
		const SHOP_STOCK_PRICE_INFO& price = prices[i];
		const float centerX = plotLeft + slotWidth * (static_cast<float>(i) + 0.5f);
		const float barLeft = centerX - barWidth * 0.5f;
		const float barRight = centerX + barWidth * 0.5f;
		const float previousY = priceToY(price.nPreviousPrice);
		const float newY = priceToY(price.nNewPrice);
		if (price.nPreviousPrice == price.nNewPrice)
		{
			QueueShopDirectWriteText(FormatGraphTimeLabel(price.wstrChangedTime),
				XMFLOAT4(centerX - slotWidth * 0.5f, timeTop,
					centerX + slotWidth * 0.5f, timeTop + boardHeight * 0.04f),
				boardHeight * 0.020f, 0xFFFFFFFF, true, true);
			continue;
		}
		const float top = min(previousY, newY);
		const float bottom = max(previousY, newY);
		const UINT barColor = (price.nNewPrice >= price.nPreviousPrice) ? 0x00FF0000 : 0x000070C0;
		RenderSolidUiRectangle(commandList, camera, barLeft, top,
			barRight, max(bottom, top + 2.0f), barColor, context);
		QueueShopDirectWriteText(FormatGraphTimeLabel(price.wstrChangedTime),
			XMFLOAT4(centerX - slotWidth * 0.5f, timeTop,
				centerX + slotWidth * 0.5f, timeTop + boardHeight * 0.04f),
			boardHeight * 0.020f, 0xFFFFFFFF, true, true);
	}

	UINT highestPrice = maximumPrice;
	UINT lowestPrice = minimumPrice;
	UINT averagePrice = 0;
	if (!prices.empty())
	{
		UINT64 sum = 0;
		for (const SHOP_STOCK_PRICE_INFO& price : prices) sum += price.nNewPrice;
		averagePrice = static_cast<UINT>(sum / prices.size());
	}
	const SHOP_STOCK_PRICE_INFO latestPrice = prices.empty() ? SHOP_STOCK_PRICE_INFO() : prices.back();
	const UINT basePrice = prices.empty() ? 100 : prices.front().nPreviousPrice;
	const UINT latestNewPrice = prices.empty() ? 100 : latestPrice.nNewPrice;
	const UINT latestPreviousPrice = prices.empty() ? 100 : latestPrice.nPreviousPrice;
	const INT64 latestDelta = static_cast<INT64>(latestNewPrice) - static_cast<INT64>(latestPreviousPrice);
	const double latestPercent = (latestPreviousPrice > 0)
		? (static_cast<double>(latestDelta) * 100.0 / static_cast<double>(latestPreviousPrice))
		: ((latestDelta > 0) ? 100.0 : 0.0);
	wchar_t latestDeltaText[80] = {};
	swprintf_s(latestDeltaText, L"%+lld (%+.2f%%)", latestDelta, latestPercent);

	const float infoTop = graphRect.y + graphRectHeight * 0.10f;
	const float infoRight = graphRect.z - graphRectWidth * 0.02f;
	const float infoFont = boardHeight * 0.031f;
	const float infoLine = infoFont * 1.18f;
	auto drawInfo = [&](const std::wstring& text, int line, bool center = false)
	{
		const float y = infoTop + infoLine * line;
		QueueShopDirectWriteText(text,
			XMFLOAT4(infoLeft, y, infoRight, y + infoLine),
			infoFont, 0xFFFFFFFF, center, true);
	};

	drawInfo(L"[주가 요약]", 0, true);
	if (targetGraph)
	{
		const UINT currentPrice = hasTargetStock ? targetStock.nCurrentPrice : latestNewPrice;
		drawInfo(L"현재가:     " + FormatStockPrice(currentPrice), 1);
		drawInfo(L"최고가:     " + FormatStockPrice(highestPrice), 2);
		drawInfo(L"최저가:     " + FormatStockPrice(lowestPrice), 3);
		drawInfo(L"변동률:     " + FormatStockPercentChange(basePrice, currentPrice), 4);
		drawInfo(L"[내 보유]", 7, true);
		const UINT holdingQuantity = hasTargetStock ? targetStock.nMyQuantity : 0;
		const UINT64 evaluatedPrice64 = static_cast<UINT64>(holdingQuantity)
			* static_cast<UINT64>(currentPrice);
		const UINT evaluatedPrice = static_cast<UINT>(
			(evaluatedPrice64 > UINT_MAX) ? UINT_MAX : evaluatedPrice64);
		drawInfo(L"보유주:     " + std::to_wstring(holdingQuantity) + L"주", 8);
		drawInfo(L"평가액:     " + FormatStockPrice(evaluatedPrice), 9);
		drawInfo(L"구매가능:   " + std::to_wstring(hasTargetStock ? targetStock.nSaleableQuantity : 0) + L"주", 10);
		drawInfo(L"최근거래:   " + std::to_wstring(hasTargetStock ? targetStock.nRecentTradeQuantity : 0) + L"주", 11);

		const XMFLOAT4 receiptRect = GetStockTargetReceiptRectangle(width, height);
		RenderUiImage(commandList, camera, m_StockReceiptResource, receiptRect);
		const XMFLOAT4 quantityRect = GetStockTargetQuantityRectangle(width, height);
		const float receiptWidth = receiptRect.z - receiptRect.x;
		const float receiptHeight = receiptRect.w - receiptRect.y;
		const XMFLOAT4 priceRect(receiptRect.x + receiptWidth * 0.075f,
			receiptRect.y + receiptHeight * 0.53f,
			receiptRect.z - receiptWidth * 0.07f,
			receiptRect.y + receiptHeight * 0.93f);
		QueueShopDirectWriteText(std::to_wstring(m_nStockTransactionOrderQuantity),
			quantityRect, (quantityRect.w - quantityRect.y) * 0.82f, 0xFF000000, true, true);
		if (m_bStockTransactionQuantityInputActive)
			RenderStockQuantityCursor(commandList, camera, quantityRect,
				m_nStockTransactionOrderQuantity, (quantityRect.w - quantityRect.y) * 0.82f);
		QueueShopDirectWriteText(L"주",
			XMFLOAT4(quantityRect.z, quantityRect.y, receiptRect.z, quantityRect.w),
			(quantityRect.w - quantityRect.y) * 0.72f, 0xFF000000, true, true);
		const UINT64 totalPrice64 = static_cast<UINT64>(m_nStockTransactionOrderQuantity)
			* static_cast<UINT64>(currentPrice);
		QueueShopDirectWriteText(ToWideString(FormatPossessionTwoDecimals(
			static_cast<UINT>((totalPrice64 > UINT_MAX) ? UINT_MAX : totalPrice64))),
			priceRect, (priceRect.w - priceRect.y) * 0.72f, 0xFF000000, true, true);

		RenderUiImage(commandList, camera, m_StockBuyingResource,
			GetStockTargetBuyingButtonRectangle(width, height, context),
			IsStockTradeButtonDisabled(0, money) ? 0x00BFBFBF : 0x00FFFFFF);
		RenderUiImage(commandList, camera, m_StockSellingResource,
			GetStockTargetSellingButtonRectangle(width, height, context),
			IsStockTradeButtonDisabled(1, money) ? 0x00BFBFBF : 0x00FFFFFF);
	}
	else
	{
		drawInfo(L"최고가:     " + FormatStockPrice(highestPrice), 1);
		drawInfo(L"최저가:     " + FormatStockPrice(lowestPrice), 2);
		drawInfo(L"평균가:     " + FormatStockPrice(averagePrice), 3);
		drawInfo(L"변동률:     " + FormatStockPercentChange(basePrice, latestNewPrice), 4);
		drawInfo(L"[최근 갱신]", 7, true);
		drawInfo(prices.empty() ? L"--:--" : FormatGraphTimeLabel(latestPrice.wstrChangedTime), 8);
		drawInfo(FormatStockPrice(latestPreviousPrice) + L" → " + FormatStockPrice(latestNewPrice), 9);
		drawInfo(latestDeltaText, 10);
		drawInfo(L"매수 " + std::to_wstring(prices.empty() ? 0 : latestPrice.nBoughtQuantity) + L"주", 11);
		drawInfo(L"매도 " + std::to_wstring(prices.empty() ? 0 : latestPrice.nSoldQuantity) + L"주", 12);
	}
}

void CShopUI::RenderPageTitle(ID3D12GraphicsCommandList* commandList, CCamera* camera)
{
	if (!camera) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	const XMFLOAT4 titleRect = GetShopPageTitleRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	RenderUiImage(commandList, camera, m_PageTitleResource, titleRect);

	const wchar_t* titleText = L"";
	switch (m_eShopPage)
	{
	case SHOP_PAGE::SHOP_MENU: titleText = L"\uC0C1\uC810"; break;
	case SHOP_PAGE::PET_CHANGE: titleText = L"\uD3AB \uAD50\uCCB4"; break;
	case SHOP_PAGE::PET_ENHANCE: titleText = L"\uD3AB \uAC15\uD654"; break;
	case SHOP_PAGE::BANK: titleText = L"\uC740\uD589"; break;
	case SHOP_PAGE::STOCK_MENU: titleText = L"\uC8FC\uC2DD"; break;
	case SHOP_PAGE::STOCK_TRANSACTION: titleText = L"\uC8FC\uC2DD \uAD6C\uB9E4"; break;
	case SHOP_PAGE::STOCK_CANT_PUBLISH: titleText = L"\uC8FC\uC2DD \uAD00\uB9AC"; break;
	case SHOP_PAGE::STOCK_MANAGEMENT: titleText = L"\uC8FC\uC2DD \uAD00\uB9AC"; break;
	case SHOP_PAGE::STOCK_SEE_MYGRAPH: titleText = L"\uC8FC\uC2DD \uADF8\uB798\uD504"; break;
	case SHOP_PAGE::STOCK_SEE_TARGET_GRAPH: titleText = L"\uC8FC\uC2DD \uADF8\uB798\uD504"; break;
	}
	if (g_pFramework)
	{
		QueueShopDirectWriteText(titleText, titleRect,
			(titleRect.w - titleRect.y) * 0.48f, 0xFF000000, true, true);
	}
}

