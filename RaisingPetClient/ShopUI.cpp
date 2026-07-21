#include "stdafx.h"
#include "ShopUI.h"
#include "GameFramework.h"
#include "DDSTextureLoader12.h"
#include "d3dx12.h"

extern CGameFramework* g_pFramework;

namespace
{
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
}

void CShopUI::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	ID3D12RootSignature* rootSignature, size_t nInitialPetCount)
{
	RebuildPetScrollMetrics(nInitialPetCount);
	ID3DBlob* vertexShader = NULL;
	ID3DBlob* pixelShader = NULL;
	ID3DBlob* errorBlob = NULL;
	HRESULT result = D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "VSTextGlyph", "vs_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &vertexShader, &errorBlob);
	if (FAILED(result)) return;
	if (errorBlob) { errorBlob->Release(); errorBlob = NULL; }
	result = D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "PSUiImage", "ps_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pixelShader, &errorBlob);
	if (FAILED(result)) { vertexShader->Release(); return; }
	if (errorBlob) errorBlob->Release();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
	pipelineDesc.pRootSignature = rootSignature;
	pipelineDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	pipelineDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
	CShader stateFactory;
	pipelineDesc.RasterizerState = stateFactory.CreateRasterizerState();
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	pipelineDesc.BlendState = stateFactory.CreateBlendState();
	pipelineDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	pipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	pipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	pipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	pipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	pipelineDesc.DepthStencilState = stateFactory.CreateDepthStencilState();
	pipelineDesc.DepthStencilState.DepthEnable = FALSE;
	pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	pipelineDesc.SampleMask = UINT_MAX;
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.NumRenderTargets = 1;
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
	pipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	pipelineDesc.SampleDesc.Count = 1;
	result = device->CreateGraphicsPipelineState(&pipelineDesc, __uuidof(ID3D12PipelineState),
		reinterpret_cast<void**>(&m_pd3dUiImagePipelineState));
	vertexShader->Release();
	pixelShader->Release();
	if (FAILED(result)) return;

	auto loadImage = [device, commandList](const wchar_t* fileName, UI_IMAGE_RESOURCE& image) -> bool
	{
		std::unique_ptr<uint8_t[]> data;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		HRESULT hr = DirectX::LoadDDSTextureFromFile(device, fileName, &image.pd3dTexture, data, subresources);
		if (FAILED(hr) || subresources.empty()) return(false);
		const UINT64 uploadSize = GetRequiredIntermediateSize(image.pd3dTexture, 0,
			static_cast<UINT>(subresources.size()));
		CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);
		hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, NULL, __uuidof(ID3D12Resource),
			reinterpret_cast<void**>(&image.pd3dTextureUploadBuffer));
		if (FAILED(hr)) return(false);
		UpdateSubresources(commandList, image.pd3dTexture, image.pd3dTextureUploadBuffer,
			0, 0, static_cast<UINT>(subresources.size()), subresources.data());
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(image.pd3dTexture,
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->ResourceBarrier(1, &barrier);
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		hr = device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap),
			reinterpret_cast<void**>(&image.pd3dSrvDescriptorHeap));
		if (FAILED(hr)) return(false);
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = image.pd3dTexture->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = image.pd3dTexture->GetDesc().MipLevels;
		device->CreateShaderResourceView(image.pd3dTexture, &srvDesc,
			image.pd3dSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		return(true);
	};

	loadImage(L"Assets/Image/Shop/ShopIcon.dds", m_ShopIconResource);
	loadImage(L"Assets/Image/Shop/ShopBoard.dds", m_ShopBoardResource);
	loadImage(L"Assets/Image/Shop/PageTitle.dds", m_PageTitleResource);
	loadImage(L"Assets/Image/Shop/ShopCloseIcon.dds", m_ShopCloseIconResource);
	loadImage(L"Assets/Image/Shop/ShopBackSpaceIcon.dds", m_ShopBackSpaceIconResource);
	loadImage(L"Assets/Image/Shop/ShopSlot1.dds", m_ShopSlotResources[0]);
	loadImage(L"Assets/Image/Shop/ShopSlot2.dds", m_ShopSlotResources[1]);
	loadImage(L"Assets/Image/Shop/ShopSlot3.dds", m_ShopSlotResources[2]);
	loadImage(L"Assets/Image/Shop/ShopSlot4.dds", m_ShopSlotResources[3]);
	loadImage(L"Assets/Image/Shop/InternetOnIcon.dds", m_InternetOnIconResource);
	loadImage(L"Assets/Image/Shop/InternetOffIcon.dds", m_InternetOffIconResource);
	loadImage(L"Assets/Image/Shop/NetworkErrorLog.dds", m_NetworkErrorLogResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockSlot1.dds", m_StockSlotResources[0]);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockSlot2.dds", m_StockSlotResources[1]);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/Limit1.dds", m_StockLimitResources[0]);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/Limit2.dds", m_StockLimitResources[1]);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/CantCreateStockGenLog.dds", m_CantCreateStockGenLogResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockName.dds", m_StockNameResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockHolders.dds", m_StockHoldersResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockManagementTable.dds", m_StockManagementTableResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockChart.dds", m_StockChartResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/MyGraph.dds", m_MyGraphResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/TargetGraph.dds", m_TargetGraphResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/SeeGraph.dds", m_SeeGraphResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockUpMark.dds", m_StockUpMarkResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockDownMark.dds", m_StockDownMarkResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/IssuanceStock.dds", m_IssuanceStockResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/IssuanceStockErrorLog.dds", m_IssuanceStockErrorLogResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockTitle.dds", m_StockTransactionTitleResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockIssuer.dds", m_StockTransactionIssuerResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockGraph.dds", m_StockTransactionGraphResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockDescription.dds", m_StockTransactionDescriptionResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/SeeStock.dds", m_SeeStockResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockBuying.dds", m_StockBuyingResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockSelling.dds", m_StockSellingResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockBuyingFailLog.dds", m_StockBuyingFailLogResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockSellingFailLog.dds", m_StockSellingFailLogResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockReceipt.dds", m_StockReceiptResource);
	loadImage(L"Assets/Image/Login/TextCursor.dds", m_TextCursorResource);
	loadImage(L"Assets/Image/Common/EmptySquare.dds", m_EmptySquareResources[0]);
	loadImage(L"Assets/Image/Common/EmptySquare.dds", m_EmptySquareResources[1]);
	loadImage(L"Assets/Image/Shop/PetConfirmationButton.dds", m_PetConfirmationButtonResource);
	loadImage(L"Assets/Image/Shop/ScrollBackGround.dds", m_ScrollBackgroundResource);
	loadImage(L"Assets/Image/Shop/Scroll.dds", m_ScrollResource);
	loadImage(L"Assets/Image/Common/EmptyFrame.dds", m_EmptyFrameResource);
	loadImage(L"Assets/Image/Shop/PetEnhanceButton.dds", m_PetEnhanceButtonResource);
	loadImage(L"Assets/Image/Shop/PetEnhancePriceFrame.dds", m_PetEnhancePriceFrameResource);
	loadImage(L"Assets/Image/Shop/PetEnhanceLog1.dds", m_PetEnhanceLogResources[0]);
	loadImage(L"Assets/Image/Shop/PetEnhanceLog2.dds", m_PetEnhanceLogResources[1]);
	loadImage(L"Assets/Image/Shop/Financial/BankFrame.dds", m_BankFrameResource);
	loadImage(L"Assets/Image/Shop/Financial/InstallmentSavingsButton.dds", m_FinancialCategoryButtonResources[0]);
	loadImage(L"Assets/Image/Shop/Financial/LoansButton.dds", m_FinancialCategoryButtonResources[1]);
	for (int i = 0; i < 10; ++i)
	{
		const std::wstring number = std::to_wstring(i + 1);
		loadImage((L"Assets/Image/Shop/Financial/IS" + number + L".dds").c_str(),
			m_FinancialProductNameResources[0][i]);
		loadImage((L"Assets/Image/Shop/Financial/Loans" + number + L".dds").c_str(),
			m_FinancialProductNameResources[1][i]);
	}
	loadImage(L"Assets/Image/Shop/Financial/LeftButton.dds", m_FinancialLeftButtonResource);
	loadImage(L"Assets/Image/Shop/Financial/RightButton.dds", m_FinancialRightButtonResource);
	loadImage(L"Assets/Image/Shop/Financial/TimerFrame.dds", m_FinancialTimerFrameResource);
	loadImage(L"Assets/Image/Shop/Financial/MoneyFrame.dds", m_FinancialMoneyFrameResource);
	loadImage(L"Assets/Image/Shop/Financial/RightPoint.dds", m_FinancialRightPointResource);
	loadImage(L"Assets/Image/Shop/Financial/ApplicationButton.dds", m_FinancialApplicationButtonResource);
	loadImage(L"Assets/Image/Shop/Financial/ISFailLog.dds", m_FinancialFailLogResources[0]);
	loadImage(L"Assets/Image/Shop/Financial/LoansFailLog.dds", m_FinancialFailLogResources[1]);
}

void CShopUI::ReleaseObjects()
{
	if (m_pPreviewPet)
	{
		delete m_pPreviewPet;
		m_pPreviewPet = NULL;
		m_nPreviewPetIndex = static_cast<size_t>(-1);
	}
	if (m_pd3dUiImagePipelineState) m_pd3dUiImagePipelineState->Release();
	UI_IMAGE_RESOURCE* images[] = { &m_ShopIconResource, &m_ShopBoardResource, &m_PageTitleResource,
		&m_ShopCloseIconResource, &m_ShopBackSpaceIconResource, &m_ShopSlotResources[0],
		&m_ShopSlotResources[1], &m_ShopSlotResources[2], &m_ShopSlotResources[3],
		&m_EmptySquareResources[0], &m_EmptySquareResources[1], &m_PetConfirmationButtonResource,
		&m_ScrollBackgroundResource, &m_ScrollResource, &m_EmptyFrameResource,
		&m_PetEnhanceButtonResource, &m_PetEnhancePriceFrameResource,
		&m_PetEnhanceLogResources[0], &m_PetEnhanceLogResources[1], &m_BankFrameResource,
		&m_FinancialCategoryButtonResources[0], &m_FinancialCategoryButtonResources[1],
		&m_FinancialLeftButtonResource, &m_FinancialRightButtonResource,
		&m_FinancialTimerFrameResource, &m_FinancialMoneyFrameResource,
		&m_FinancialRightPointResource, &m_FinancialApplicationButtonResource,
		&m_FinancialFailLogResources[0], &m_FinancialFailLogResources[1],
		&m_InternetOnIconResource,
		&m_InternetOffIconResource, &m_NetworkErrorLogResource,
		&m_StockSlotResources[0], &m_StockSlotResources[1],
		&m_StockLimitResources[0], &m_StockLimitResources[1],
		&m_CantCreateStockGenLogResource, &m_StockNameResource,
		&m_StockHoldersResource, &m_StockManagementTableResource,
		&m_StockChartResource, &m_MyGraphResource, &m_TargetGraphResource, &m_SeeGraphResource, &m_StockUpMarkResource,
		&m_StockDownMarkResource, &m_IssuanceStockResource, &m_IssuanceStockErrorLogResource,
		&m_StockTransactionTitleResource, &m_StockTransactionIssuerResource,
		&m_StockTransactionGraphResource, &m_StockTransactionDescriptionResource,
		&m_SeeStockResource, &m_StockBuyingResource, &m_StockSellingResource,
		&m_StockBuyingFailLogResource, &m_StockSellingFailLogResource,
		&m_StockReceiptResource, &m_TextCursorResource };
	for (UI_IMAGE_RESOURCE* image : images)
	{
		if (image->pd3dTexture) image->pd3dTexture->Release();
		if (image->pd3dTextureUploadBuffer) image->pd3dTextureUploadBuffer->Release();
		if (image->pd3dSrvDescriptorHeap) image->pd3dSrvDescriptorHeap->Release();
	}
	for (int category = 0; category < 2; ++category)
	{
		for (int index = 0; index < 10; ++index)
		{
			UI_IMAGE_RESOURCE& image = m_FinancialProductNameResources[category][index];
			if (image.pd3dTexture) image.pd3dTexture->Release();
			if (image.pd3dTextureUploadBuffer) image.pd3dTextureUploadBuffer->Release();
			if (image.pd3dSrvDescriptorHeap) image.pd3dSrvDescriptorHeap->Release();
		}
	}
}

void CShopUI::Animate(float elapsedTime)
{
	if (m_bShopActive && m_eShopPage == SHOP_PAGE::PET_CHANGE && m_pPreviewPet)
		m_pPreviewPet->AnimateWithoutMovement(elapsedTime);
	if (m_bStockNameInputActive)
	{
		m_fStockNameCursorBlinkElapsed += elapsedTime;
		while (m_fStockNameCursorBlinkElapsed >= 1.0f)
			m_fStockNameCursorBlinkElapsed -= 1.0f;
	}
	if (m_bStockTransactionQuantityInputActive)
	{
		m_fStockTransactionQuantityCursorBlinkElapsed += elapsedTime;
		while (m_fStockTransactionQuantityCursorBlinkElapsed >= 1.0f)
			m_fStockTransactionQuantityCursorBlinkElapsed -= 1.0f;
	}
	for (int category = 0; category < 2; ++category)
	{
		if (!m_bFinancialProductActive[category]) continue;
		m_fActiveFinancialElapsedSeconds[category] += elapsedTime;
		if (m_fActiveFinancialElapsedSeconds[category] >
			static_cast<float>(m_nActiveFinancialDurationSeconds[category]))
			m_fActiveFinancialElapsedSeconds[category] =
			static_cast<float>(m_nActiveFinancialDurationSeconds[category]);
	}
	for (SHOP_NETWORK_ERROR_LOG& log : m_NetworkErrorLogs)
		log.fElapsedTime += elapsedTime;
	m_NetworkErrorLogs.erase(std::remove_if(m_NetworkErrorLogs.begin(), m_NetworkErrorLogs.end(),
		[](const SHOP_NETWORK_ERROR_LOG& log) { return(log.fElapsedTime >= 1.0f); }),
		m_NetworkErrorLogs.end());
	for (int i = 0; i < 2; ++i)
	{
		for (SHOP_NETWORK_ERROR_LOG& log : m_FinancialFailLogs[i])
			log.fElapsedTime += elapsedTime;
		m_FinancialFailLogs[i].erase(std::remove_if(m_FinancialFailLogs[i].begin(),
			m_FinancialFailLogs[i].end(),
			[](const SHOP_NETWORK_ERROR_LOG& log) { return(log.fElapsedTime >= 1.0f); }),
			m_FinancialFailLogs[i].end());
	}
	for (SHOP_NETWORK_ERROR_LOG& log : m_StockIssueErrorLogs)
		log.fElapsedTime += elapsedTime;
	m_StockIssueErrorLogs.erase(std::remove_if(m_StockIssueErrorLogs.begin(), m_StockIssueErrorLogs.end(),
		[](const SHOP_NETWORK_ERROR_LOG& log) { return(log.fElapsedTime >= 1.0f); }),
		m_StockIssueErrorLogs.end());
	for (int i = 0; i < 2; ++i)
	{
		for (SHOP_NETWORK_ERROR_LOG& log : m_StockTradeFailLogs[i])
			log.fElapsedTime += elapsedTime;
		m_StockTradeFailLogs[i].erase(std::remove_if(m_StockTradeFailLogs[i].begin(),
			m_StockTradeFailLogs[i].end(),
			[](const SHOP_NETWORK_ERROR_LOG& log) { return(log.fElapsedTime >= 1.0f); }),
			m_StockTradeFailLogs[i].end());
	}
}

void CShopUI::ReleaseUploadBuffers()
{
	UI_IMAGE_RESOURCE* images[] = { &m_ShopIconResource, &m_ShopBoardResource, &m_PageTitleResource,
		&m_ShopCloseIconResource, &m_ShopBackSpaceIconResource, &m_ShopSlotResources[0],
		&m_ShopSlotResources[1], &m_ShopSlotResources[2], &m_ShopSlotResources[3],
		&m_EmptySquareResources[0], &m_EmptySquareResources[1], &m_PetConfirmationButtonResource,
		&m_ScrollBackgroundResource, &m_ScrollResource, &m_EmptyFrameResource,
		&m_PetEnhanceButtonResource, &m_PetEnhancePriceFrameResource,
		&m_PetEnhanceLogResources[0], &m_PetEnhanceLogResources[1], &m_BankFrameResource,
		&m_FinancialCategoryButtonResources[0], &m_FinancialCategoryButtonResources[1],
		&m_FinancialLeftButtonResource, &m_FinancialRightButtonResource,
		&m_FinancialTimerFrameResource, &m_FinancialMoneyFrameResource,
		&m_FinancialRightPointResource, &m_FinancialApplicationButtonResource,
		&m_FinancialFailLogResources[0], &m_FinancialFailLogResources[1],
		&m_InternetOnIconResource,
		&m_InternetOffIconResource, &m_NetworkErrorLogResource,
		&m_StockSlotResources[0], &m_StockSlotResources[1],
		&m_StockLimitResources[0], &m_StockLimitResources[1],
		&m_CantCreateStockGenLogResource, &m_StockNameResource,
		&m_StockHoldersResource, &m_StockManagementTableResource,
		&m_StockChartResource, &m_MyGraphResource, &m_TargetGraphResource, &m_SeeGraphResource, &m_StockUpMarkResource,
		&m_StockDownMarkResource, &m_IssuanceStockResource, &m_IssuanceStockErrorLogResource,
		&m_StockTransactionTitleResource, &m_StockTransactionIssuerResource,
		&m_StockTransactionGraphResource, &m_StockTransactionDescriptionResource,
		&m_SeeStockResource, &m_StockBuyingResource, &m_StockSellingResource,
		&m_StockBuyingFailLogResource, &m_StockSellingFailLogResource,
		&m_StockReceiptResource, &m_TextCursorResource };
	for (UI_IMAGE_RESOURCE* image : images)
	{
		if (image->pd3dTextureUploadBuffer)
		{
			image->pd3dTextureUploadBuffer->Release();
			image->pd3dTextureUploadBuffer = NULL;
		}
	}
	for (int category = 0; category < 2; ++category)
	{
		for (int index = 0; index < 10; ++index)
		{
			UI_IMAGE_RESOURCE& image = m_FinancialProductNameResources[category][index];
			if (image.pd3dTextureUploadBuffer)
			{
				image.pd3dTextureUploadBuffer->Release();
				image.pd3dTextureUploadBuffer = NULL;
			}
		}
	}
}

void CShopUI::RenderUiImage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	UI_IMAGE_RESOURCE& image, const XMFLOAT4& rectangle, UINT tintColor)
{
	if (!m_pd3dUiImagePipelineState || !camera || !image.pd3dSrvDescriptorHeap) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	const float constants[4] = { rectangle.x / width * 2.0f - 1.0f,
		1.0f - rectangle.y / height * 2.0f, rectangle.z / width * 2.0f - 1.0f,
		1.0f - rectangle.w / height * 2.0f };
	ID3D12DescriptorHeap* heaps[] = { image.pd3dSrvDescriptorHeap };
	commandList->SetDescriptorHeaps(1, heaps);
	commandList->SetGraphicsRootDescriptorTable(3,
		image.pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRoot32BitConstants(4, 4, constants, 0);
	commandList->SetGraphicsRoot32BitConstant(5, tintColor, 0);
	commandList->SetPipelineState(m_pd3dUiImagePipelineState);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(6, 1, 0, 0);
}

void CShopUI::RenderTextLine(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	const std::string& text, float left, float top, float scale, UINT color,
	const SHOP_TEXT_RENDER_CONTEXT& context)
{
	if (!context.pTextPipelineState || !context.pGlyphResources || !camera) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	float cursorX = left;
	const float glyphGap = (scale * 8.0f > 1.0f) ? scale * 8.0f : 1.0f;
	const float spaceWidth = scale * 70.0f;
	commandList->SetPipelineState(context.pTextPipelineState);
	commandList->SetGraphicsRoot32BitConstant(5, color, 0);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	for (char ch : text)
	{
		if (ch == ' ') { cursorX += spaceWidth; continue; }
		TEXT_GLYPH_RESOURCE* selected = NULL;
		for (TEXT_GLYPH_RESOURCE& glyph : *context.pGlyphResources)
			if (glyph.ch == ch) { selected = &glyph; break; }
		if (!selected || !selected->pd3dSrvDescriptorHeap) continue;
		const float imageWidth = selected->fPixelWidth / (selected->fU1 - selected->fU0);
		const float imageHeight = selected->fPixelHeight / (selected->fV1 - selected->fV0);
		const float rectLeft = cursorX - selected->fU0 * imageWidth * scale;
		const float rectTop = top + selected->fTopOffset * scale - selected->fV0 * imageHeight * scale;
		const float rectRight = rectLeft + imageWidth * scale;
		const float rectBottom = rectTop + imageHeight * scale;
		const float constants[4] = { rectLeft / width * 2.0f - 1.0f,
			1.0f - rectTop / height * 2.0f, rectRight / width * 2.0f - 1.0f,
			1.0f - rectBottom / height * 2.0f };
		ID3D12DescriptorHeap* heaps[] = { selected->pd3dSrvDescriptorHeap };
		commandList->SetDescriptorHeaps(1, heaps);
		commandList->SetGraphicsRootDescriptorTable(3,
			selected->pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		commandList->SetGraphicsRoot32BitConstants(4, 4, constants, 0);
		commandList->DrawInstanced(6, 1, 0, 0);
		cursorX += selected->fPixelWidth * scale + glyphGap;
	}
	float textWidth = cursorX - left;
	if (!text.empty() && text.back() != ' ' && textWidth >= glyphGap) textWidth -= glyphGap;
}

void CShopUI::RenderSolidUiRectangle(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	float left, float top, float right, float bottom, UINT color,
	const SHOP_TEXT_RENDER_CONTEXT& context)
{
	if (!context.pSolidUiPipelineState || !camera) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	const float constants[4] = { left / width * 2.0f - 1.0f, 1.0f - top / height * 2.0f,
		right / width * 2.0f - 1.0f, 1.0f - bottom / height * 2.0f };
	commandList->SetPipelineState(context.pSolidUiPipelineState);
	commandList->SetGraphicsRoot32BitConstants(4, 4, constants, 0);
	commandList->SetGraphicsRoot32BitConstant(5, color, 0);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(6, 1, 0, 0);
}

void CShopUI::RenderStockQuantityCursor(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	const XMFLOAT4& rectangle, UINT quantity, float fontSize)
{
	if (!g_pFramework || !camera || m_fStockTransactionQuantityCursorBlinkElapsed >= 0.5f)
		return;
	const std::wstring text = std::to_wstring(quantity);
	float measuredWidth = 0.0f;
	float measuredHeight = 0.0f;
	g_pFramework->MeasureDirectWriteText(text, fontSize,
		max(rectangle.z - rectangle.x, 1.0f), max(rectangle.w - rectangle.y, 1.0f),
		measuredWidth, measuredHeight);
	const float textLeft = (rectangle.x + rectangle.z - measuredWidth) * 0.5f;
	const float cursorHeight = (rectangle.w - rectangle.y) * 0.72f;
	const float cursorWidth = max(2.0f, cursorHeight * 0.13f);
	const float cursorX = min(textLeft + measuredWidth + 2.0f, rectangle.z - cursorWidth);
	const float cursorTop = (rectangle.y + rectangle.w - cursorHeight) * 0.5f;
	RenderUiImage(commandList, camera, m_TextCursorResource,
		XMFLOAT4(cursorX, cursorTop, cursorX + cursorWidth, cursorTop + cursorHeight));
}

XMFLOAT4 CShopUI::GetMoneyUiRectangle(float width, float height, UINT money,
	const SHOP_TEXT_RENDER_CONTEXT& context) const
{
	if (!context.pGlyphResources) return(XMFLOAT4());
	auto findGlyph = [&context](char ch) -> const TEXT_GLYPH_RESOURCE*
	{
		for (const TEXT_GLYPH_RESOURCE& glyph : *context.pGlyphResources)
			if (glyph.ch == ch) return(&glyph);
		return(NULL);
	};
	float fixedWidth = 0.0f;
	for (const char* p = gMoneyUiLayout.pWidthReferenceText; *p; ++p)
	{
		const TEXT_GLYPH_RESOURCE* glyph = findGlyph(*p);
		if (glyph) fixedWidth += glyph->fPixelWidth * gMoneyUiLayout.fGlyphScale + gMoneyUiLayout.fGlyphGap;
	}
	if (fixedWidth > 0.0f) fixedWidth -= gMoneyUiLayout.fGlyphGap;
	float minimumTop = FLT_MAX;
	float maximumBottom = -FLT_MAX;
	for (char ch : FormatPossession(money))
	{
		const TEXT_GLYPH_RESOURCE* glyph = findGlyph(ch);
		if (!glyph) continue;
		minimumTop = min(minimumTop, glyph->fTopOffset * gMoneyUiLayout.fGlyphScale);
		maximumBottom = max(maximumBottom,
			(glyph->fTopOffset + glyph->fPixelHeight) * gMoneyUiLayout.fGlyphScale);
	}
	if (minimumTop == FLT_MAX) return(XMFLOAT4());
	const float panelWidth = fixedWidth + gMoneyUiLayout.fHorizontalPadding * 2.0f;
	const float panelHeight = maximumBottom - minimumTop + gMoneyUiLayout.fVerticalPadding * 2.0f;
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float right = board.z - gShopUiLayout.fMoneyRightPadding;
	const float bottom = board.w - gShopUiLayout.fMoneyBottomPadding;
	return(XMFLOAT4(right - panelWidth, bottom - panelHeight, right, bottom));
}

void CShopUI::RenderMoneyUI(ID3D12GraphicsCommandList* commandList, CCamera* camera, UINT money,
	const SHOP_TEXT_RENDER_CONTEXT& context)
{
	if (!context.pGlyphResources || !camera) return;
	const std::string text = FormatPossession(money);
	float textWidth = 0.0f;
	float minimumTop = FLT_MAX;
	for (char ch : text)
	{
		for (const TEXT_GLYPH_RESOURCE& glyph : *context.pGlyphResources)
		{
			if (glyph.ch != ch) continue;
			textWidth += glyph.fPixelWidth * gMoneyUiLayout.fGlyphScale + gMoneyUiLayout.fGlyphGap;
			minimumTop = min(minimumTop, glyph.fTopOffset * gMoneyUiLayout.fGlyphScale);
			break;
		}
	}
	if (textWidth > 0.0f) textWidth -= gMoneyUiLayout.fGlyphGap;
	const XMFLOAT4 panel = GetMoneyUiRectangle(camera->m_d3dViewport.Width,
		camera->m_d3dViewport.Height, money, context);
	const float outline = gMoneyUiLayout.fOutlineThickness;
	RenderSolidUiRectangle(commandList, camera, panel.x - outline, panel.y - outline,
		panel.z + outline, panel.w + outline, 0x00000000, context);
	RenderSolidUiRectangle(commandList, camera, panel.x, panel.y, panel.z, panel.w, 0x00FFFFFF, context);
	const float left = panel.z - gMoneyUiLayout.fHorizontalPadding - textWidth;
	const float top = panel.y + gMoneyUiLayout.fVerticalPadding - minimumTop;
	RenderTextLine(commandList, camera, text, left, top, gMoneyUiLayout.fGlyphScale, 0x00000000, context);
}

void CShopUI::EnsurePreviewPet(const std::vector<SHOP_PET_RENDER_RESOURCE>& pets)
{
	if (m_nSelectedPetIndex >= pets.size() || !pets[m_nSelectedPetIndex].pPet)
	{
		if (m_pPreviewPet) delete m_pPreviewPet;
		m_pPreviewPet = NULL;
		m_nPreviewPetIndex = static_cast<size_t>(-1);
		return;
	}
	if (m_pPreviewPet && m_nPreviewPetIndex == m_nSelectedPetIndex) return;

	if (m_pPreviewPet) delete m_pPreviewPet;
	CPet* sourcePet = pets[m_nSelectedPetIndex].pPet;
	m_pPreviewPet = new CPet();
	m_pPreviewPet->SetMesh(sourcePet->m_pMesh);
	m_pPreviewPet->SetShader(sourcePet->m_pShader);
	m_pPreviewPet->SetName(sourcePet->GetName());
	m_pPreviewPet->SetPay(sourcePet->GetPay());
	m_pPreviewPet->GetMaxPossession(sourcePet->GetMaxPossession());
	m_pPreviewPet->GetNowPossession(0);
	m_pPreviewPet->SetPosition(0.0f, 0.0f, 0.0f);
	m_nPreviewPetIndex = m_nSelectedPetIndex;
}

void CShopUI::RenderPreviewPet(ID3D12GraphicsCommandList* commandList, CCamera* mainCamera,
	const XMFLOAT4& panel, const std::vector<SHOP_PET_RENDER_RESOURCE>& pets)
{
	EnsurePreviewPet(pets);
	if (!m_pPreviewPet || m_nPreviewPetIndex >= pets.size()
		|| !pets[m_nPreviewPetIndex].pd3dSrvDescriptorHeap) return;

	const LONG inset = 3;
	const LONG left = static_cast<LONG>(panel.x) + inset;
	const LONG top = static_cast<LONG>(panel.y) + inset;
	const LONG right = static_cast<LONG>(panel.z) - inset;
	const LONG bottom = static_cast<LONG>(panel.w) - inset;
	const int viewportWidth = right - left;
	const int viewportHeight = bottom - top;
	if (viewportWidth <= 0 || viewportHeight <= 0) return;

	m_PreviewPetCamera.SetViewport(left, top, viewportWidth, viewportHeight, 0.0f, 0.2f);
	m_PreviewPetCamera.SetScissorRect(left, top, right, bottom);

	const BoundingOrientedBox& bounds = m_pPreviewPet->m_pMesh->m_xmOOBB;
	XMFLOAT3 target = bounds.Center;
	XMStoreFloat3(&target, XMVector3TransformCoord(XMLoadFloat3(&bounds.Center),
		XMLoadFloat4x4(&m_pPreviewPet->m_xmf4x4World)));
	const float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
	const float fieldOfView = 45.0f;
	const float tanHalfVerticalFov = tanf(XMConvertToRadians(fieldOfView * 0.5f));
	const float tanHalfHorizontalFov = tanHalfVerticalFov * aspectRatio;
	const float horizontalRadius = sqrtf(bounds.Extents.x * bounds.Extents.x
		+ bounds.Extents.z * bounds.Extents.z);
	const float verticalDistance = bounds.Extents.y / (tanHalfVerticalFov * 0.70f);
	const float horizontalDistance = horizontalRadius / (tanHalfHorizontalFov * 0.70f);
	float cameraDistance = ((verticalDistance > horizontalDistance)
		? verticalDistance : horizontalDistance) + horizontalRadius;
	if (cameraDistance < 1.0f) cameraDistance = 10.0f;
	const XMFLOAT3 cameraPosition(target.x, target.y, target.z - cameraDistance);
	m_PreviewPetCamera.GenerateViewMatrix(cameraPosition, target, XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_PreviewPetCamera.GenerateProjectionMatrix(0.1f, 1000.0f,
		aspectRatio, fieldOfView);
	m_PreviewPetCamera.SetViewportsAndScissorRects(commandList);
	m_PreviewPetCamera.UpdateShaderVariables(commandList);

	ID3D12DescriptorHeap* descriptorHeaps[] = { pets[m_nPreviewPetIndex].pd3dSrvDescriptorHeap };
	commandList->SetDescriptorHeaps(1, descriptorHeaps);
	commandList->SetGraphicsRootDescriptorTable(3,
		pets[m_nPreviewPetIndex].pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	m_pPreviewPet->Render(commandList, &m_PreviewPetCamera);

	mainCamera->SetViewportsAndScissorRects(commandList);
	mainCamera->UpdateShaderVariables(commandList);
}

XMFLOAT4 CShopUI::GetEnhanceButtonRectangle(int type, float width, float height) const
{
	const XMFLOAT4 panel = GetPetContentPanelRectangle(type != 0, width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float panelWidth = panel.z - panel.x;
	const float panelHeight = panel.w - panel.y;
	const float buttonWidth = panelWidth * 0.40f;
	const float buttonHeight = buttonWidth * (216.0f / 447.0f);
	const float left = panel.x + panelWidth * 0.06f;
	const float top = panel.y + panelHeight * 0.76f;
	return(XMFLOAT4(left, top, left + buttonWidth, top + buttonHeight));
}

XMFLOAT4 CShopUI::GetEnhancePriceRectangle(int type, float width, float height) const
{
	const XMFLOAT4 button = GetEnhanceButtonRectangle(type, width, height);
	const float buttonHeight = button.w - button.y;
	const float priceHeight = buttonHeight * 0.70f;
	const float priceWidth = buttonHeight * (493.0f / 213.0f);
	const float gap = (button.z - button.x) * 0.05f;
	const float top = (button.y + button.w - priceHeight) * 0.5f;
	return(XMFLOAT4(button.z + gap, top, button.z + gap + priceWidth, top + priceHeight));
}

XMFLOAT4 CShopUI::GetFinancialCategoryButtonRectangle(int category, float width, float height) const
{
	const XMFLOAT4 bank = GetFinancialBankFrameRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float bankWidth = bank.z - bank.x;
	const float buttonWidth = bankWidth * 0.168f;
	const float buttonHeight = buttonWidth * (197.0f / 439.0f);
	const float gap = 0.0f;
	const float totalWidth = buttonWidth * 2.0f + gap;
	const float left = (bank.x + bank.z - totalWidth) * 0.5f + category * (buttonWidth + gap);
	const float top = bank.y + (bank.w - bank.y) * 0.05f;
	return(XMFLOAT4(left, top, left + buttonWidth, top + buttonHeight));
}

XMFLOAT4 CShopUI::GetFinancialLeftButtonRectangle(float width, float height) const
{
	const XMFLOAT4 bank = GetFinancialBankFrameRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const XMFLOAT4 product = GetFinancialProductNameRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float buttonHeight = (product.w - product.y) * 1.35f;
	const float buttonWidth = buttonHeight * (201.0f / 234.0f);
	const float left = bank.x + (bank.z - bank.x) * 0.035f;
	const float centerY = (product.y + product.w) * 0.5f;
	return(XMFLOAT4(left, centerY - buttonHeight * 0.5f, left + buttonWidth,
		centerY + buttonHeight * 0.5f));
}

XMFLOAT4 CShopUI::GetFinancialRightButtonRectangle(float width, float height) const
{
	const XMFLOAT4 bank = GetFinancialBankFrameRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const XMFLOAT4 leftButton = GetFinancialLeftButtonRectangle(width, height);
	const float buttonWidth = leftButton.z - leftButton.x;
	const float right = bank.z - (bank.z - bank.x) * 0.035f;
	return(XMFLOAT4(right - buttonWidth, leftButton.y, right, leftButton.w));
}

XMFLOAT4 CShopUI::GetFinancialApplicationButtonRectangle(float width, float height,
	UINT money, const SHOP_TEXT_RENDER_CONTEXT& context) const
{
	const XMFLOAT4 moneyRect = GetMoneyUiRectangle(width, height, money, context);
	const float buttonHeight = moneyRect.w - moneyRect.y;
	const float buttonWidth = buttonHeight * (447.0f / 217.0f);
	const float gap = 8.0f;
	return(XMFLOAT4(moneyRect.x - gap - buttonWidth, moneyRect.y,
		moneyRect.x - gap, moneyRect.w));
}

XMFLOAT4 CShopUI::GetStockSlotRectangle(int index, float width, float height) const
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float slotWidth = boardWidth * 0.46f;
	const float slotHeight = slotWidth * (260.0f / 1190.0f);
	const float left = board.x + boardWidth * 0.065f;
	const float top = board.y + boardHeight * 0.28f + index * (slotHeight + boardHeight * 0.14f);
	return(XMFLOAT4(left, top, left + slotWidth, top + slotHeight));
}

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
			g_pFramework->QueueDirectWriteText(text,
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
		g_pFramework->QueueDirectWriteText(stockName, titleRect,
			(titleRect.w - titleRect.y) * 0.45f, 0xFF000000, true, true);
		g_pFramework->QueueDirectWriteText(issuerId,
			XMFLOAT4(issuerRect.x + (issuerRect.z - issuerRect.x) * 0.31f, issuerRect.y,
				issuerRect.z - 5.0f, issuerRect.w),
			(issuerRect.w - issuerRect.y) * 0.42f, 0xFF000000, true, true);

		const UINT currentPrice = hasSelectedStock ? selectedStock.nCurrentPrice : 0;
		const UINT previousPrice = hasSelectedStock ? selectedStock.nPreviousPrice : 0;
		const bool priceUpOrSame = (currentPrice >= previousPrice);
		const UINT priceTextColor = priceUpOrSame ? 0xFFFF0000 : 0xFF0070C0;
		const float priceTop = graphRect.y + (graphRect.w - graphRect.y) * 0.79f;
		const float priceBottom = graphRect.w - (graphRect.w - graphRect.y) * 0.04f;
		g_pFramework->QueueDirectWriteText(hasSelectedStock ? FormatStockPrice(currentPrice) : emptyText,
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
			g_pFramework->QueueDirectWriteText(FormatStockPriceChangeText(currentPrice, previousPrice),
				XMFLOAT4(markLeft + markSize + 2.0f, priceTop, graphRect.z - 3.0f, priceBottom),
				(priceBottom - priceTop) * 0.42f, priceTextColor, false, true);
		}
		else
		{
			g_pFramework->QueueDirectWriteText(emptyText,
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
			g_pFramework->QueueDirectWriteText(labels[i],
				XMFLOAT4(labelLeft, y, valueLeft, y + descLine),
				descFont, 0xFF000000, false, true);
			const std::wstring valueText = L":" + values[i] + (hasSelectedStock ? L"\uC8FC" : L"");
			g_pFramework->QueueDirectWriteText(valueText,
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
		g_pFramework->QueueDirectWriteText(hasSelectedStock
			? std::to_wstring(m_nStockTransactionOrderQuantity) : emptyText,
			quantityRect, (quantityRect.w - quantityRect.y) * 0.68f, 0xFF000000, true, true);
		if (hasSelectedStock && m_bStockTransactionQuantityInputActive
			&& m_eShopPage == SHOP_PAGE::STOCK_TRANSACTION)
			RenderStockQuantityCursor(commandList, camera, quantityRect,
				m_nStockTransactionOrderQuantity, (quantityRect.w - quantityRect.y) * 0.68f);
		if (hasSelectedStock)
		{
			g_pFramework->QueueDirectWriteText(L"\uC8FC",
				XMFLOAT4(quantityRect.z, quantityRect.y, descriptionRect.z, quantityRect.w),
				(quantityRect.w - quantityRect.y) * 0.60f, 0xFF000000, true, true);
		}
		const UINT64 totalPrice = static_cast<UINT64>(m_nStockTransactionOrderQuantity)
			* static_cast<UINT64>(hasSelectedStock ? selectedStock.nCurrentPrice : 0);
		g_pFramework->QueueDirectWriteText(hasSelectedStock
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
			g_pFramework->QueueDirectWriteText(m_wstrStockName, textRect, fontSize,
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
		g_pFramework->QueueDirectWriteText(FormatStockPrice(currentPrice),
			XMFLOAT4(currentPriceLeft, priceTop, currentPriceRight, priceBottom),
			priceFontSize, priceTextColor, false, true);
		const float markSize = (priceBottom - priceTop) * 0.62f;
		const float markLeft = chartRect.x + (chartRect.z - chartRect.x) * 0.48f;
		const float markTop = priceTop + (priceBottom - priceTop - markSize) * 0.5f;
		RenderUiImage(commandList, camera,
			priceUpOrSame ? m_StockUpMarkResource : m_StockDownMarkResource,
			XMFLOAT4(markLeft, markTop, markLeft + markSize, markTop + markSize));
		g_pFramework->QueueDirectWriteText(FormatStockPriceChangeText(currentPrice, previousPrice),
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
			g_pFramework->QueueDirectWriteText(holderRankText,
				XMFLOAT4(holderTextLeft, holderTextTop + holderLineHeight * i,
					holderQuantityLeft, holderTextTop + holderLineHeight * (i + 1)),
				holderFontSize, holderColors[i], false, true);
			g_pFramework->QueueDirectWriteText(holderQuantityText,
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
			g_pFramework->QueueDirectWriteText(tableLabels[i], labelRect,
				boardHeight * 0.030f, 0xFF000000, true, true);
			g_pFramework->QueueDirectWriteText(tableValues[i], valueRect,
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
		m_eShopPage = SHOP_PAGE::STOCK_TRANSACTION;
		m_bStockNameInputActive = false;
		m_bStockTransactionQuantityInputActive = false;
		m_bPendingStockTransactionListRequest = true;
		return(true);
	}
	if (IsPointInRectangle(x, y, GetStockSlotRectangle(1, width, height)))
	{
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
		g_pFramework->QueueDirectWriteText(FormatStockPrice(value),
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
			g_pFramework->QueueDirectWriteText(FormatGraphTimeLabel(price.wstrChangedTime),
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
		g_pFramework->QueueDirectWriteText(FormatGraphTimeLabel(price.wstrChangedTime),
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
		g_pFramework->QueueDirectWriteText(text,
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
		g_pFramework->QueueDirectWriteText(std::to_wstring(m_nStockTransactionOrderQuantity),
			quantityRect, (quantityRect.w - quantityRect.y) * 0.82f, 0xFF000000, true, true);
		if (m_bStockTransactionQuantityInputActive)
			RenderStockQuantityCursor(commandList, camera, quantityRect,
				m_nStockTransactionOrderQuantity, (quantityRect.w - quantityRect.y) * 0.82f);
		g_pFramework->QueueDirectWriteText(L"주",
			XMFLOAT4(quantityRect.z, quantityRect.y, receiptRect.z, quantityRect.w),
			(quantityRect.w - quantityRect.y) * 0.72f, 0xFF000000, true, true);
		const UINT64 totalPrice64 = static_cast<UINT64>(m_nStockTransactionOrderQuantity)
			* static_cast<UINT64>(currentPrice);
		g_pFramework->QueueDirectWriteText(ToWideString(FormatPossessionTwoDecimals(
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
		g_pFramework->QueueDirectWriteText(titleText, titleRect,
			(titleRect.w - titleRect.y) * 0.48f, 0xFF000000, true, true);
	}
}

void CShopUI::Render(ID3D12GraphicsCommandList* commandList, CCamera* camera, UINT money,
	size_t activePetIndex, const std::vector<SHOP_PET_RENDER_RESOURCE>& pets,
	const SHOP_TEXT_RENDER_CONTEXT& context, bool networkConnected)
{
	if (!camera) return;
	RebuildPetScrollMetricsIfNeeded(pets.size());
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	if (m_bShopActive)
	{
		RenderUiImage(commandList, camera, m_ShopBoardResource,
			GetShopBoardRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
		RenderPageTitle(commandList, camera);
		if (m_eShopPage == SHOP_PAGE::SHOP_MENU)
		{
			for (int i = 0; i < 4; ++i)
			{
				const UINT slotTint = (!networkConnected && (i == 2 || i == 3))
					? 0x00BFBFBF : 0x00FFFFFF;
				RenderUiImage(commandList, camera, m_ShopSlotResources[i],
					GetShopSlotRectangle(i, width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y),
					slotTint);
			}
			RenderUiImage(commandList, camera,
				networkConnected ? m_InternetOnIconResource : m_InternetOffIconResource,
				GetShopNetworkIconRectangle(width, height,
					m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
			for (const SHOP_NETWORK_ERROR_LOG& log : m_NetworkErrorLogs)
			{
				const float alphaRatio = max(0.0f, 1.0f - log.fElapsedTime);
				const UINT alpha = max(1u, static_cast<UINT>(alphaRatio * 255.0f + 0.5f));
				const float moveY = -20.0f * log.fElapsedTime;
				RenderUiImage(commandList, camera, m_NetworkErrorLogResource,
					XMFLOAT4(log.rectangle.x, log.rectangle.y + moveY,
						log.rectangle.z, log.rectangle.w + moveY),
					(alpha << 24) | 0x00FFFFFF);
			}
		}
		else if (m_eShopPage == SHOP_PAGE::PET_CHANGE)
		{
			const XMFLOAT4 leftPanel = GetPetContentPanelRectangle(false, width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
			const XMFLOAT4 rightPanel = GetPetContentPanelRectangle(true, width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
			RenderUiImage(commandList, camera, m_EmptySquareResources[0], leftPanel);
			RenderUiImage(commandList, camera, m_EmptySquareResources[1], rightPanel);
			RenderPreviewPet(commandList, camera, rightPanel, pets);
			RenderUiImage(commandList, camera, m_PetConfirmationButtonResource,
				GetPetConfirmationRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
			const XMFLOAT4 scrollTrack = GetPetScrollTrackRectangle(width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
			RenderUiImage(commandList, camera, m_ScrollBackgroundResource, scrollTrack);
			RenderUiImage(commandList, camera, m_ScrollResource,
				GetPetScrollThumbRectangle(width, height));
			const float rowHeight = (leftPanel.w - leftPanel.y) / 10.0f;
			const float glyphScale = rowHeight / 320.0f;
			for (size_t row = 0; row < 10; ++row)
			{
				const size_t petIndex = m_nPetScrollOffset + row;
				if (petIndex >= pets.size() || !pets[petIndex].pPet) continue;
				if (petIndex == m_nSelectedPetIndex)
				{
					const XMFLOAT4 selection = GetPetListRowRectangle(row, width, height);
					RenderSolidUiRectangle(commandList, camera, selection.x, selection.y,
						selection.z, selection.w, 0x00BFBFBF, context);
				}
				const std::string name = std::to_string(petIndex + 1) + ". " + pets[petIndex].pPet->GetName();
				RenderTextLine(commandList, camera, name, leftPanel.x + 18.0f,
					leftPanel.y + rowHeight * row + rowHeight * 0.15f, glyphScale, 0x00000000, context);
			}
		}
		else if (m_eShopPage == SHOP_PAGE::PET_ENHANCE)
		{
			CPet* activePet = (activePetIndex < pets.size()) ? pets[activePetIndex].pPet : NULL;
			RenderEnhancementPage(commandList, camera, activePet, money, context);
		}
		else if (m_eShopPage == SHOP_PAGE::BANK)
		{
			RenderFinancialPage(commandList, camera, money, context);
		}
		else if (m_eShopPage == SHOP_PAGE::STOCK_MENU)
		{
			RenderStockMenuPage(commandList, camera);
		}
		else if (m_eShopPage == SHOP_PAGE::STOCK_TRANSACTION)
		{
			RenderStockTransactionPage(commandList, camera, money, context);
			RenderStockTradeFailLogs(commandList, camera, money, context);
		}
		else if (m_eShopPage == SHOP_PAGE::STOCK_MANAGEMENT)
		{
			RenderStockManagementPage(commandList, camera);
			RenderStockIssueErrorLogs(commandList, camera);
		}
		else if (m_eShopPage == SHOP_PAGE::STOCK_CANT_PUBLISH)
		{
			RenderCantCreateStockPage(commandList, camera, context);
		}
		else if (m_eShopPage == SHOP_PAGE::STOCK_SEE_MYGRAPH
			|| m_eShopPage == SHOP_PAGE::STOCK_SEE_TARGET_GRAPH)
		{
			RenderStockGraphPage(commandList, camera, money, context);
			if (m_eShopPage == SHOP_PAGE::STOCK_SEE_TARGET_GRAPH)
				RenderStockTradeFailLogs(commandList, camera, money, context);
		}
		RenderMoneyUI(commandList, camera, money, context);
		RenderUiImage(commandList, camera, m_ShopBackSpaceIconResource,
			GetShopBackRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
		RenderUiImage(commandList, camera, m_ShopCloseIconResource,
			GetShopCloseRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
	}
	RenderUiImage(commandList, camera, m_ShopIconResource, GetShopIconRectangle(width, height));
}

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
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float boardHeight = board.w - board.y;
	const XMFLOAT4 moneyRect = GetMoneyUiRectangle(width, height, 0, context);
	const float buttonHeight = boardHeight * 0.105f;
	const float buttonWidth = buttonHeight * (294.0f / 216.0f);
	const float buttonGap = 8.0f;
	const float moneyGap = 22.0f;
	const float totalButtonWidth = buttonWidth * 2.0f + buttonGap;
	const float buyingLeft = moneyRect.x - moneyGap - totalButtonWidth;
	const float buttonTop = board.w - boardHeight * 0.13f;
	return(XMFLOAT4(buyingLeft, buttonTop,
		buyingLeft + buttonWidth, buttonTop + buttonHeight));
}

XMFLOAT4 CShopUI::GetStockTargetSellingButtonRectangle(float width, float height,
	const SHOP_TEXT_RENDER_CONTEXT& context) const
{
	const XMFLOAT4 buying = GetStockTargetBuyingButtonRectangle(width, height, context);
	const float buttonWidth = buying.z - buying.x;
	const float buttonGap = 8.0f;
	return(XMFLOAT4(buying.z + buttonGap, buying.y,
		buying.z + buttonGap + buttonWidth, buying.w));
}

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
	if (!m_bShopActive) return(false);
	return(IsPointInRectangle(x, y,
		GetShopBoardRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y)));
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

bool CShopUI::ProcessFinancialClick(float x, float y, float width, float height,
	UINT money, const SHOP_TEXT_RENDER_CONTEXT& context)
{
	for (int category = 0; category < 2; ++category)
	{
		if (!IsPointInRectangle(x, y, GetFinancialCategoryButtonRectangle(category, width, height))) continue;
		m_nFinancialCategory = category;
		return(true);
	}

	if (IsPointInRectangle(x, y, GetFinancialLeftButtonRectangle(width, height)))
	{
		if (m_bFinancialProductActive[m_nFinancialCategory]) return(true);
		int& index = m_nFinancialProductIndices[m_nFinancialCategory];
		if (index > 0) --index;
		return(true);
	}
	if (IsPointInRectangle(x, y, GetFinancialRightButtonRectangle(width, height)))
	{
		if (m_bFinancialProductActive[m_nFinancialCategory]) return(true);
		int& index = m_nFinancialProductIndices[m_nFinancialCategory];
		if (index < 9 && index < m_nFinancialMaximumProductIndices[m_nFinancialCategory]) ++index;
		return(true);
	}
	if (IsPointInRectangle(x, y, GetFinancialApplicationButtonRectangle(width, height, money, context)))
	{
		if (!IsFinancialApplicationButtonDisabled(money))
		{
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

bool CShopUI::ProcessShopUIClick(float x, float y, float width, float height, UINT money,
	size_t petCount, size_t activePetIndex, const SHOP_TEXT_RENDER_CONTEXT& context,
	bool networkConnected)
{
	const bool iconClicked = IsPointInRectangle(x, y, GetShopIconRectangle(width, height));
	const bool closeClicked = m_bShopActive && IsPointInRectangle(x, y,
		GetShopCloseRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
	if (iconClicked || closeClicked)
	{
		if (m_bShopActive) DeactivateShop(width, height, activePetIndex);
		else
		{
			if (m_bResetShopPositionOnNextOpen)
			{
				m_xmf2ShopBoardOffset = XMFLOAT2(0.0f, 0.0f);
				m_bResetShopPositionOnNextOpen = false;
			}
			m_bShopActive = true;
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
				m_nPendingConfirmedPetIndex = m_nSelectedPetIndex;
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
				QueueStockTradeRequest(0);
			return(true);
		}
		if (!m_StockTransactionInfos.empty()
			&& IsPointInRectangle(x, y,
				GetStockTransactionSellingButtonRectangle(width, height, context)))
		{
			if (!IsStockTradeButtonDisabled(1, money))
				QueueStockTradeRequest(1);
			return(true);
		}
		if (!m_StockTransactionInfos.empty()
			&& IsPointInRectangle(x, y, GetStockTransactionGraphButtonRectangle(width, height)))
		{
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
				QueueStockTradeRequest(0);
			return(true);
		}
		if (!m_StockTransactionInfos.empty()
			&& IsPointInRectangle(x, y,
				GetStockTargetSellingButtonRectangle(width, height, context)))
		{
			if (!IsStockTradeButtonDisabled(1, money))
				QueueStockTradeRequest(1);
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
		RebuildPetScrollMetricsIfNeeded(nPetCount);
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
			const XMFLOAT4 board = GetShopBoardRectangle(width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
			const XMFLOAT4 moneyRect = GetMoneyUiRectangle(width, height, money, context);
			if (IsPointInRectangle(x, y, board) && !IsPointInRectangle(x, y, moneyRect))
			{
				m_bShopBoardDragging = true;
				m_xmf2ShopDragLastCursor = XMFLOAT2(x, y);
				return(true);
			}
		}
		break;
	case WM_MOUSEMOVE:
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
		break;
	case WM_LBUTTONUP:
		if (m_bStockIssueButtonPressed)
		{
			m_bStockIssueButtonPressed = false;
			if (m_bShopActive && m_eShopPage == SHOP_PAGE::STOCK_MANAGEMENT
				&& IsPointInRectangle(x, y, GetStockIssuanceButtonRectangle(width, height))
				&& !m_wstrStockName.empty())
			{
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
				m_nPendingEnhancementType = pressedButton;
			if (GetCapture() == hWnd) ReleaseCapture();
			return(true);
		}
		if (m_bPetScrollDragging) { m_bPetScrollDragging = false; return(true); }
		if (m_bShopBoardDragging) { m_bShopBoardDragging = false; return(true); }
		break;
	}
	return(false);
}
