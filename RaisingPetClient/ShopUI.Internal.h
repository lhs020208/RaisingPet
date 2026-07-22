#pragma once

// Internal layout constants and helper functions for ShopUI.cpp.
// This header is included inside the ShopUI anonymous namespace.

struct MONEY_UI_LAYOUT
{
	float fHorizontalPadding = 14.0f;
	float fVerticalPadding = 8.0f;
	float fOutlineThickness = 2.0f;
	float fGlyphScale = 0.12f;
	float fGlyphGap = 1.0f;
	const char* pWidthReferenceText = "100.0k";
};

struct SHOP_UI_LAYOUT
{
	float fIconSize = 64.0f;
	float fIconRightMargin = 30.0f;
	float fIconBottomMargin = 50.0f;
	float fBoardWidthRatio = 0.68f;
	float fBoardMaximumWidth = 720.0f;
	float fBoardMaximumHeightRatio = 0.80f;
	float fCloseWidth = 40.0f;
	float fCloseHeight = 45.0f;
	float fBoardTopPadding = 25.0f;
	float fBoardRightPadding = 20.0f;
	float fBackWidth = 40.0f;
	float fBackHeight = 40.0f;
	float fBackToCloseGap = 8.0f;
	float fSlotLeftRatio = 0.055f;
	float fSlotTopRatio = 0.21f;
	float fSlotWidthRatio = 0.41f;
	float fSlotVerticalGap = 14.0f;
	float fMoneyRightPadding = 24.0f;
	float fMoneyBottomPadding = 18.0f;
	float fContentPanelTopRatio = 0.21f;
	float fContentPanelWidthRatio = 0.41f;
	float fContentPanelHeightRatio = 0.60f;
	float fContentLeftRatio = 0.055f;
	float fContentRightRatio = 0.535f;
	float fConfirmationWidth = 112.0f;
	float fConfirmationHeight = 54.0f;
	float fConfirmationBottomPadding = 20.0f;
	float fConfirmationHorizontalOffset = -24.0f;
	float fScrollWidth = 15.0f;
	float fScrollRightPadding = 10.0f;
	float fScrollVerticalPadding = 20.0f;
};

struct FINANCIAL_PRODUCT_DATA
{
	UINT nFirstMoney = 0;
	UINT nSecondMoney = 0;
	UINT nDurationSeconds = 0;
};

const MONEY_UI_LAYOUT gMoneyUiLayout;
const SHOP_UI_LAYOUT gShopUiLayout;
const FINANCIAL_PRODUCT_DATA gInstallmentSavingsProducts[10] =
{
	{ 150, 240, 600 }, { 500, 850, 1800 }, { 1500, 2700, 3600 },
	{ 5000, 9500, 7200 }, { 15000, 30000, 14400 }, { 50000, 105000, 21600 },
	{ 150000, 330000, 28800 }, { 500000, 1150000, 43200 },
	{ 2000000, 4800000, 64800 }, { 10000000, 25000000, 86400 }
};
const FINANCIAL_PRODUCT_DATA gLoanProducts[10] =
{
	{ 200, 210, 600 }, { 700, 740, 1800 }, { 2000, 2120, 3600 },
	{ 7000, 7450, 7200 }, { 20000, 21400, 14400 }, { 70000, 75000, 21600 },
	{ 200000, 216000, 28800 }, { 700000, 760000, 43200 },
	{ 2500000, 2750000, 64800 }, { 12000000, 13500000, 86400 }
};

std::string FormatPossession(UINT nValue)
{
	if (nValue < 1000) return(std::to_string(nValue));
	static const char suffixes[] = { 'k', 'm', 'b' };
	UINT64 nDivisor = 1000;
	int nSuffixIndex = 0;
	while ((nValue / nDivisor) >= 1000 && nSuffixIndex < 2)
	{
		nDivisor *= 1000;
		++nSuffixIndex;
	}
	const UINT64 nTenths = (static_cast<UINT64>(nValue) * 10) / nDivisor;
	return(std::to_string(nTenths / 10) + "." + std::to_string(nTenths % 10) + suffixes[nSuffixIndex]);
}

std::string FormatPossessionTwoDecimals(UINT nValue)
{
	if (nValue < 1000) return(std::to_string(nValue));
	static const char suffixes[] = { 'k', 'm', 'b' };
	UINT64 divisor = 1000;
	int suffixIndex = 0;
	while ((nValue / divisor) >= 1000 && suffixIndex < 2)
	{
		divisor *= 1000;
		++suffixIndex;
	}
	const UINT64 hundredths = (static_cast<UINT64>(nValue) * 100) / divisor;
	const UINT64 fraction = hundredths % 100;
	return(std::to_string(hundredths / 100) + "."
		+ ((fraction < 10) ? "0" : "") + std::to_string(fraction) + suffixes[suffixIndex]);
}

std::string FormatFinancialMoney(UINT value, bool income)
{
	return(std::string(income ? "+" : "-") + FormatPossessionTwoDecimals(value) + "$");
}

std::wstring ToWideString(const std::string& text)
{
	return(std::wstring(text.begin(), text.end()));
}

std::wstring FormatStockQuantity(UINT value, UINT total)
{
	return(std::to_wstring(value) + L"/" + std::to_wstring(total) + L"\uC8FC");
}

std::wstring FormatStockTradeQuantity(UINT value)
{
	return(std::to_wstring(value) + L"\uC8FC");
}

std::wstring FormatStockRevenue(UINT value)
{
	return(ToWideString(FormatPossessionTwoDecimals(value)));
}

std::wstring FormatStockPrice(UINT value)
{
	return(ToWideString(FormatPossessionTwoDecimals(value)));
}

std::wstring FormatStockPriceChangeText(UINT currentPrice, UINT previousPrice)
{
	const INT64 change = static_cast<INT64>(currentPrice) - static_cast<INT64>(previousPrice);
	const double percent = (previousPrice > 0)
		? (static_cast<double>(change) * 100.0 / static_cast<double>(previousPrice))
		: ((change > 0) ? 100.0 : 0.0);
	wchar_t buffer[64] = {};
	swprintf_s(buffer, L"%+lld(%.2f%%)", change, percent);
	return(std::wstring(buffer));
}

std::wstring FormatStockPercentChange(UINT fromPrice, UINT toPrice)
{
	const INT64 change = static_cast<INT64>(toPrice) - static_cast<INT64>(fromPrice);
	const double percent = (fromPrice > 0)
		? (static_cast<double>(change) * 100.0 / static_cast<double>(fromPrice))
		: ((change > 0) ? 100.0 : 0.0);
	wchar_t buffer[64] = {};
	swprintf_s(buffer, L"%+.2f%%", percent);
	return(std::wstring(buffer));
}

std::wstring FormatGraphTimeLabel(const std::wstring& changedTime)
{
	if (changedTime.size() >= 16) return(changedTime.substr(11, 5));
	if (changedTime.size() >= 5) return(changedTime.substr(changedTime.size() - 5, 5));
	return(changedTime);
}

std::string FormatFinancialDuration(UINT seconds)
{
	const UINT hours = seconds / 3600;
	seconds %= 3600;
	const UINT minutes = seconds / 60;
	seconds %= 60;
	auto twoDigits = [](UINT value) -> std::string
	{
		return((value < 10) ? "0" : "") + std::to_string(value);
	};
	return(twoDigits(hours) + "H " + twoDigits(minutes) + "M " + twoDigits(seconds) + "S");
}

float MeasureShopTextWidth(const std::string& text, float scale, const SHOP_TEXT_RENDER_CONTEXT& context)
{
	if (!context.pGlyphResources) return(0.0f);
	const float gap = (scale * 8.0f > 1.0f) ? scale * 8.0f : 1.0f;
	const float space = scale * 70.0f;
	float result = 0.0f;
	for (char ch : text)
	{
		if (ch == ' ') { result += space; continue; }
		for (const TEXT_GLYPH_RESOURCE& glyph : *context.pGlyphResources)
		{
			if (glyph.ch != ch) continue;
			result += glyph.fPixelWidth * scale + gap;
			break;
		}
	}
	if (!text.empty() && result >= gap) result -= gap;
	return(result);
}

bool IsPointInRectangle(float x, float y, const XMFLOAT4& rectangle)
{
	return(x >= rectangle.x && x <= rectangle.z && y >= rectangle.y && y <= rectangle.w);
}

XMFLOAT4 GetShopIconRectangle(float width, float height)
{
	const float right = width - gShopUiLayout.fIconRightMargin;
	const float bottom = height - gShopUiLayout.fIconBottomMargin;
	return(XMFLOAT4(right - gShopUiLayout.fIconSize, bottom - gShopUiLayout.fIconSize, right, bottom));
}

XMFLOAT4 GetSettingIconRectangle(float width, float height)
{
	const XMFLOAT4 shopIcon = GetShopIconRectangle(width, height);
	const float gap = 12.0f;
	const float iconSize = shopIcon.z - shopIcon.x;
	return(XMFLOAT4(shopIcon.x - gap - iconSize, shopIcon.y,
		shopIcon.x - gap, shopIcon.w));
}

XMFLOAT4 GetShopBoardRectangle(float width, float height, float offsetX = 0.0f, float offsetY = 0.0f)
{
	float boardWidth = width * gShopUiLayout.fBoardWidthRatio;
	if (boardWidth > gShopUiLayout.fBoardMaximumWidth) boardWidth = gShopUiLayout.fBoardMaximumWidth;
	float boardHeight = boardWidth * (2017.0f / 2923.0f);
	const float maximumHeight = height * gShopUiLayout.fBoardMaximumHeightRatio;
	if (boardHeight > maximumHeight)
	{
		boardHeight = maximumHeight;
		boardWidth = boardHeight * (2923.0f / 2017.0f);
	}
	const float left = ((width - boardWidth) * 0.5f) + offsetX;
	const float top = ((height - boardHeight) * 0.5f) + offsetY;
	return(XMFLOAT4(left, top, left + boardWidth, top + boardHeight));
}

XMFLOAT4 GetShopCloseRectangle(float width, float height, float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height, offsetX, offsetY);
	const float right = board.z - gShopUiLayout.fBoardRightPadding;
	const float top = board.y + gShopUiLayout.fBoardTopPadding;
	return(XMFLOAT4(right - gShopUiLayout.fCloseWidth, top, right, top + gShopUiLayout.fCloseHeight));
}

XMFLOAT4 GetShopBackRectangle(float width, float height, float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 close = GetShopCloseRectangle(width, height, offsetX, offsetY);
	const float right = close.x - gShopUiLayout.fBackToCloseGap;
	return(XMFLOAT4(right - gShopUiLayout.fBackWidth, close.y, right, close.y + gShopUiLayout.fBackHeight));
}

XMFLOAT4 GetShopPageTitleRectangle(float width, float height, float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height, offsetX, offsetY);
	const XMFLOAT4 close = GetShopCloseRectangle(width, height, offsetX, offsetY);
	const float titleHeight = close.w - close.y;
	const float titleWidth = titleHeight * (1380.0f / 177.0f);
	const float centerX = (board.x + board.z) * 0.5f;
	return(XMFLOAT4(centerX - titleWidth * 0.5f, close.y,
		centerX + titleWidth * 0.5f, close.y + titleHeight));
}

XMFLOAT4 GetShopSlotRectangle(int index, float width, float height, float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height, offsetX, offsetY);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float slotWidth = boardWidth * gShopUiLayout.fSlotWidthRatio;
	const float slotHeight = slotWidth * (259.0f / 1190.0f);
	const float left = board.x + boardWidth * gShopUiLayout.fSlotLeftRatio;
	const float top = board.y + boardHeight * gShopUiLayout.fSlotTopRatio
		+ index * (slotHeight + gShopUiLayout.fSlotVerticalGap);
	return(XMFLOAT4(left, top, left + slotWidth, top + slotHeight));
}

XMFLOAT4 GetShopNetworkIconRectangle(float width, float height, float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height, offsetX, offsetY);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float iconSize = boardHeight * 0.34f;
	const float centerX = board.x + boardWidth * 0.76f;
	const float centerY = board.y + boardHeight * 0.48f;
	return(XMFLOAT4(centerX - iconSize * 0.5f, centerY - iconSize * 0.5f,
		centerX + iconSize * 0.5f, centerY + iconSize * 0.5f));
}

XMFLOAT4 GetShopNetworkErrorLogRectangle(int slotIndex, float width, float height,
	float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height, offsetX, offsetY);
	const XMFLOAT4 slot = GetShopSlotRectangle(slotIndex, width, height, offsetX, offsetY);
	const float boardWidth = board.z - board.x;
	const float logWidth = boardWidth * 0.58f;
	const float logHeight = logWidth * (133.0f / 1756.0f);
	const float centerX = (slot.x + slot.z) * 0.5f;
	const float top = slot.y - logHeight - 12.0f;
	return(XMFLOAT4(centerX - logWidth * 0.5f, top, centerX + logWidth * 0.5f, top + logHeight));
}

XMFLOAT4 GetShopNetworkIconErrorLogRectangle(float width, float height,
	float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height, offsetX, offsetY);
	const XMFLOAT4 icon = GetShopNetworkIconRectangle(width, height, offsetX, offsetY);
	const float boardWidth = board.z - board.x;
	const float logWidth = boardWidth * 0.58f;
	const float logHeight = logWidth * (133.0f / 1756.0f);
	const float margin = 20.0f;
	float left = ((icon.x + icon.z) * 0.5f) - (logWidth * 0.5f);
	float top = icon.y - logHeight - 12.0f;
	if (left < board.x + margin) left = board.x + margin;
	if (left + logWidth > board.z - margin) left = board.z - margin - logWidth;
	if (top < board.y + margin) top = icon.w + 12.0f;
	return(XMFLOAT4(left, top, left + logWidth, top + logHeight));
}

XMFLOAT4 GetPetContentPanelRectangle(bool rightPanel, float width, float height,
	float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height, offsetX, offsetY);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float leftRatio = rightPanel ? gShopUiLayout.fContentRightRatio : gShopUiLayout.fContentLeftRatio;
	const float left = board.x + boardWidth * leftRatio;
	const float top = board.y + boardHeight * gShopUiLayout.fContentPanelTopRatio;
	return(XMFLOAT4(left, top, left + boardWidth * gShopUiLayout.fContentPanelWidthRatio,
		top + boardHeight * gShopUiLayout.fContentPanelHeightRatio));
}

XMFLOAT4 GetPetConfirmationRectangle(float width, float height, float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 panel = GetPetContentPanelRectangle(true, width, height, offsetX, offsetY);
	const float centerX = ((panel.x + panel.z) * 0.5f) + gShopUiLayout.fConfirmationHorizontalOffset;
	const float top = panel.w + gShopUiLayout.fConfirmationBottomPadding;
	return(XMFLOAT4(centerX - gShopUiLayout.fConfirmationWidth * 0.5f, top,
		centerX + gShopUiLayout.fConfirmationWidth * 0.5f, top + gShopUiLayout.fConfirmationHeight));
}

XMFLOAT4 GetPetScrollTrackRectangle(float width, float height, float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 panel = GetPetContentPanelRectangle(false, width, height, offsetX, offsetY);
	const float right = panel.z - gShopUiLayout.fScrollRightPadding;
	return(XMFLOAT4(right - gShopUiLayout.fScrollWidth,
		panel.y + gShopUiLayout.fScrollVerticalPadding, right,
		panel.w - gShopUiLayout.fScrollVerticalPadding));
}

XMFLOAT4 GetFinancialBankFrameRectangle(float width, float height, float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height, offsetX, offsetY);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	float bankWidth = boardWidth * 0.885f;
	float bankHeight = bankWidth * (1321.0f / 2602.0f);
	const float maximumHeight = boardHeight * 0.68f;
	if (bankHeight > maximumHeight)
	{
		bankHeight = maximumHeight;
		bankWidth = bankHeight * (2602.0f / 1321.0f);
	}
	const float left = (board.x + board.z - bankWidth) * 0.5f;
	const float top = board.y + boardHeight * 0.18f;
	return(XMFLOAT4(left, top, left + bankWidth, top + bankHeight));
}

XMFLOAT4 GetFinancialProductNameRectangle(float width, float height, float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 bank = GetFinancialBankFrameRectangle(width, height, offsetX, offsetY);
	const float bankWidth = bank.z - bank.x;
	const float productWidth = bankWidth * 0.695f;
	const float productHeight = productWidth * (198.0f / 1812.0f);
	const float left = (bank.x + bank.z - productWidth) * 0.5f;
	const float top = bank.y + (bank.w - bank.y) * 0.28f;
	return(XMFLOAT4(left, top, left + productWidth, top + productHeight));
}

XMFLOAT4 GetFinancialTimerFrameRectangle(float width, float height, float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 product = GetFinancialProductNameRectangle(width, height, offsetX, offsetY);
	const XMFLOAT4 bank = GetFinancialBankFrameRectangle(width, height, offsetX, offsetY);
	const float top = bank.y + (bank.w - bank.y) * 0.51f;
	const float timerHeight = (product.z - product.x) * (197.0f / 1812.0f);
	return(XMFLOAT4(product.x, top, product.z, top + timerHeight));
}

XMFLOAT4 GetFinancialMoneyFrameRectangle(int index, float width, float height,
	float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 bank = GetFinancialBankFrameRectangle(width, height, offsetX, offsetY);
	const float bankWidth = bank.z - bank.x;
	const float moneyWidth = bankWidth * 0.35f;
	const float moneyHeight = moneyWidth * (197.0f / 910.0f);
	const float centerX = (bank.x + bank.z) * 0.5f;
	const float gap = bankWidth * 0.105f;
	const float top = bank.y + (bank.w - bank.y) * 0.765f;
	const float left = (index == 0) ? centerX - gap * 0.5f - moneyWidth : centerX + gap * 0.5f;
	return(XMFLOAT4(left, top, left + moneyWidth, top + moneyHeight));
}

XMFLOAT4 GetFinancialRightPointRectangle(float width, float height, float offsetX = 0.0f, float offsetY = 0.0f)
{
	const XMFLOAT4 leftMoney = GetFinancialMoneyFrameRectangle(0, width, height, offsetX, offsetY);
	const XMFLOAT4 rightMoney = GetFinancialMoneyFrameRectangle(1, width, height, offsetX, offsetY);
	const float pointWidth = (rightMoney.x - leftMoney.z) * 0.55f;
	const float pointHeight = pointWidth * (94.0f / 163.0f);
	const float centerX = (leftMoney.z + rightMoney.x) * 0.5f;
	const float centerY = (leftMoney.y + leftMoney.w) * 0.5f;
	return(XMFLOAT4(centerX - pointWidth * 0.5f, centerY - pointHeight * 0.5f,
		centerX + pointWidth * 0.5f, centerY + pointHeight * 0.5f));
}
