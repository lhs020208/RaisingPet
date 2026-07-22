// scroll metrics and interaction rectangles for ShopUI.cpp
// This file is intentionally included by ShopUI.cpp to keep helper functions in one translation unit.

void CShopUI::RebuildPetScrollMetrics(size_t nPetCount)
{
	m_nCachedPetCount = nPetCount;
	m_nMaximumPetScrollOffset = (nPetCount > 10) ? nPetCount - 10 : 0;
	m_fPetScrollThumbRatio = 10.0f / static_cast<float>((nPetCount > 10) ? nPetCount : 10);
	if (m_nPetScrollOffset > m_nMaximumPetScrollOffset)
		m_nPetScrollOffset = m_nMaximumPetScrollOffset;
}

void CShopUI::RebuildPetScrollMetricsIfNeeded(size_t nPetCount)
{
	if (nPetCount != m_nCachedPetCount) RebuildPetScrollMetrics(nPetCount);
}

XMFLOAT4 CShopUI::GetPetScrollThumbRectangle(float width, float height) const
{
	const XMFLOAT4 track = GetPetScrollTrackRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float trackHeight = track.w - track.y;
	const float thumbHeight = trackHeight * m_fPetScrollThumbRatio;
	const float progress = (m_nMaximumPetScrollOffset > 0)
		? static_cast<float>(m_nPetScrollOffset) / static_cast<float>(m_nMaximumPetScrollOffset) : 0.0f;
	const float top = track.y + (trackHeight - thumbHeight) * progress;
	return(XMFLOAT4(track.x, top, track.z, top + thumbHeight));
}

XMFLOAT4 CShopUI::GetPetListRowRectangle(size_t row, float width, float height) const
{
	const XMFLOAT4 panel = GetPetContentPanelRectangle(false, width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const XMFLOAT4 track = GetPetScrollTrackRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float rowHeight = (panel.w - panel.y) / 10.0f;
	const float verticalMargin = rowHeight * 0.08f;
	return(XMFLOAT4(panel.x + 10.0f, panel.y + rowHeight * row + verticalMargin,
		track.x - 10.0f, panel.y + rowHeight * (row + 1) - verticalMargin));
}

XMFLOAT4 CShopUI::GetStockTransactionListRowRectangle(size_t row, float width, float height) const
{
	const XMFLOAT4 panel = GetPetContentPanelRectangle(false, width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const XMFLOAT4 track = GetPetScrollTrackRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float rowHeight = (panel.w - panel.y) / 10.0f;
	const float verticalMargin = rowHeight * 0.08f;
	return(XMFLOAT4(panel.x + 10.0f, panel.y + rowHeight * row + verticalMargin,
		track.x - 10.0f, panel.y + rowHeight * (row + 1) - verticalMargin));
}

XMFLOAT4 CShopUI::GetStockTransactionScrollThumbRectangle(float width, float height) const
{
	const XMFLOAT4 track = GetPetScrollTrackRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float trackHeight = track.w - track.y;
	const float visibleRows = 10.0f;
	const float totalRows = static_cast<float>((m_StockTransactionInfos.size() > 10)
		? m_StockTransactionInfos.size() : 10);
	const float thumbHeight = trackHeight * (visibleRows / totalRows);
	const float progress = (m_nMaximumStockTransactionScrollOffset > 0)
		? static_cast<float>(m_nStockTransactionScrollOffset)
		/ static_cast<float>(m_nMaximumStockTransactionScrollOffset) : 0.0f;
	const float top = track.y + (trackHeight - thumbHeight) * progress;
	return(XMFLOAT4(track.x, top, track.z, top + thumbHeight));
}

XMFLOAT4 CShopUI::GetStockTransactionQuantityRectangle(float width, float height) const
{
	const XMFLOAT4 rightPanel = GetPetContentPanelRectangle(true, width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
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
	const float descriptionTop = rightPanel.y + (rightHeight - totalHeight) * 0.5f
		+ titleHeight + issuerHeight + graphHeight + gap * 3.0f;
	const XMFLOAT4 descriptionRect(imageLeft, descriptionTop,
		imageLeft + imageWidth, descriptionTop + descriptionHeight);
	const float descWidth = descriptionRect.z - descriptionRect.x;
	const float descHeight = descriptionRect.w - descriptionRect.y;
	return(XMFLOAT4(descriptionRect.x + descWidth * 0.66f,
		descriptionRect.y + descHeight * 0.09f,
		descriptionRect.x + descWidth * 0.88f,
		descriptionRect.y + descHeight * 0.40f));
}

XMFLOAT4 CShopUI::GetStockTransactionGraphButtonRectangle(float width, float height) const
{
	const XMFLOAT4 rightPanel = GetPetContentPanelRectangle(true, width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
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
	const float graphTop = rightPanel.y + (rightHeight - totalHeight) * 0.5f
		+ titleHeight + issuerHeight + gap * 2.0f;
	const XMFLOAT4 graphRect(imageLeft, graphTop, imageLeft + imageWidth, graphTop + graphHeight);
	const float seeStockWidth = (graphRect.z - graphRect.x) * 0.55f;
	const float seeStockHeight = seeStockWidth * (209.0f / 678.0f);
	const float seeStockCenterY = graphRect.y + (graphRect.w - graphRect.y) * 0.40f;
	return(XMFLOAT4((graphRect.x + graphRect.z - seeStockWidth) * 0.5f,
		seeStockCenterY - seeStockHeight * 0.5f,
		(graphRect.x + graphRect.z + seeStockWidth) * 0.5f,
		seeStockCenterY + seeStockHeight * 0.5f));
}

XMFLOAT4 CShopUI::GetStockTransactionBuyingButtonRectangle(float width, float height,
	const SHOP_TEXT_RENDER_CONTEXT& context) const
{
	const XMFLOAT4 rightPanel = GetPetContentPanelRectangle(true, width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const XMFLOAT4 moneyRect = GetMoneyUiRectangle(width, height, 0, context);
	const XMFLOAT4 confirm = GetPetConfirmationRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float buttonHeight = confirm.w - confirm.y;
	const float buttonWidth = buttonHeight * (294.0f / 216.0f);
	const float buttonGap = 8.0f;
	const float moneyGap = 22.0f;
	const float totalButtonWidth = buttonWidth * 2.0f + buttonGap;
	const float desiredLeft = moneyRect.x - moneyGap - totalButtonWidth;
	const float minimumLeft = rightPanel.x + (rightPanel.z - rightPanel.x) * 0.07f;
	const float buyingLeft = max(minimumLeft, desiredLeft);
	return(XMFLOAT4(buyingLeft, confirm.y, buyingLeft + buttonWidth, confirm.w));
}

XMFLOAT4 CShopUI::GetStockTransactionSellingButtonRectangle(float width, float height,
	const SHOP_TEXT_RENDER_CONTEXT& context) const
{
	const XMFLOAT4 buying = GetStockTransactionBuyingButtonRectangle(width, height, context);
	const float buttonWidth = buying.z - buying.x;
	const float buttonGap = 8.0f;
	return(XMFLOAT4(buying.z + buttonGap, buying.y,
		buying.z + buttonGap + buttonWidth, buying.w));
}

XMFLOAT4 CShopUI::GetStockTargetReceiptRectangle(float width, float height) const
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float graphWidth = boardWidth * 0.86f;
	const float graphHeight = graphWidth * (1354.0f / 2553.0f);
	const float graphLeft = (board.x + board.z - graphWidth) * 0.5f;
	const float graphTop = board.y + boardHeight * 0.17f;
	const XMFLOAT4 graphRect(graphLeft, graphTop, graphLeft + graphWidth, graphTop + graphHeight);
	const float graphRectWidth = graphRect.z - graphRect.x;
	const float graphRectHeight = graphRect.w - graphRect.y;
	const float plotLeft = graphRect.x + graphRectWidth * (1.59f / 19.60f);
	const float plotRight = plotLeft + graphRectWidth * (12.23f / 19.60f);
	const float infoLeft = plotRight + graphRectWidth * 0.018f;
	const float infoRight = graphRect.z - graphRectWidth * 0.02f;
	const float receiptWidth = infoRight - infoLeft;
	const float receiptHeight = receiptWidth * (430.0f / 1095.0f);
	const float receiptBottom = graphRect.y + graphRectHeight * 0.965f;
	return(XMFLOAT4(infoLeft, receiptBottom - receiptHeight,
		infoLeft + receiptWidth, receiptBottom));
}

XMFLOAT4 CShopUI::GetStockTargetQuantityRectangle(float width, float height) const
{
	const XMFLOAT4 receipt = GetStockTargetReceiptRectangle(width, height);
	const float receiptWidth = receipt.z - receipt.x;
	const float receiptHeight = receipt.w - receipt.y;
	return(XMFLOAT4(receipt.x + receiptWidth * 0.075f,
		receipt.y + receiptHeight * 0.07f,
		receipt.x + receiptWidth * 0.49f,
		receipt.y + receiptHeight * 0.47f));
}

XMFLOAT4 CShopUI::GetStockTargetBuyingButtonRectangle(float width, float height,
	const SHOP_TEXT_RENDER_CONTEXT& context) const
{
	return(GetStockTransactionBuyingButtonRectangle(width, height, context));
}

XMFLOAT4 CShopUI::GetStockTargetSellingButtonRectangle(float width, float height,
	const SHOP_TEXT_RENDER_CONTEXT& context) const
{
	return(GetStockTransactionSellingButtonRectangle(width, height, context));
}

