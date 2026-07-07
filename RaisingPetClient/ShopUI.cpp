#include "stdafx.h"
#include "ShopUI.h"
#include "DDSTextureLoader12.h"
#include "d3dx12.h"

namespace
{
struct MONEY_UI_LAYOUT
{
	float fHorizontalPadding = 14.0f;
	float fVerticalPadding = 8.0f;
	float fOutlineThickness = 2.0f;
	float fGlyphScale = 0.12f;
	float fGlyphGap = 1.0f;
	const char* pWidthReferenceText = "100.0k$";
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

const MONEY_UI_LAYOUT gMoneyUiLayout;
const SHOP_UI_LAYOUT gShopUiLayout;

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
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
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

	loadImage(L"Assets/Image/ShopIcon.dds", m_ShopIconResource);
	loadImage(L"Assets/Image/ShopBoard.dds", m_ShopBoardResource);
	loadImage(L"Assets/Image/ShopCloseIcon.dds", m_ShopCloseIconResource);
	loadImage(L"Assets/Image/ShopBackSpaceIcon.dds", m_ShopBackSpaceIconResource);
	loadImage(L"Assets/Image/ShopSlot1.dds", m_ShopSlotResources[0]);
	loadImage(L"Assets/Image/ShopSlot2.dds", m_ShopSlotResources[1]);
	loadImage(L"Assets/Image/ShopSlot3.dds", m_ShopSlotResources[2]);
	loadImage(L"Assets/Image/ShopSlot4.dds", m_ShopSlotResources[3]);
	loadImage(L"Assets/Image/EmptySquare.dds", m_EmptySquareResources[0]);
	loadImage(L"Assets/Image/EmptySquare.dds", m_EmptySquareResources[1]);
	loadImage(L"Assets/Image/PetConfirmationButton.dds", m_PetConfirmationButtonResource);
	loadImage(L"Assets/Image/ScrollBackGround.dds", m_ScrollBackgroundResource);
	loadImage(L"Assets/Image/Scroll.dds", m_ScrollResource);
	loadImage(L"Assets/Image/EmptyFrame.dds", m_EmptyFrameResource);
	loadImage(L"Assets/Image/PetEnhanceButton.dds", m_PetEnhanceButtonResource);
	loadImage(L"Assets/Image/PetEnhancePriceFrame.dds", m_PetEnhancePriceFrameResource);
	loadImage(L"Assets/Image/PetEnhanceLog1.dds", m_PetEnhanceLogResources[0]);
	loadImage(L"Assets/Image/PetEnhanceLog2.dds", m_PetEnhanceLogResources[1]);
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
	UI_IMAGE_RESOURCE* images[] = { &m_ShopIconResource, &m_ShopBoardResource,
		&m_ShopCloseIconResource, &m_ShopBackSpaceIconResource, &m_ShopSlotResources[0],
		&m_ShopSlotResources[1], &m_ShopSlotResources[2], &m_ShopSlotResources[3],
		&m_EmptySquareResources[0], &m_EmptySquareResources[1], &m_PetConfirmationButtonResource,
		&m_ScrollBackgroundResource, &m_ScrollResource, &m_EmptyFrameResource,
		&m_PetEnhanceButtonResource, &m_PetEnhancePriceFrameResource,
		&m_PetEnhanceLogResources[0], &m_PetEnhanceLogResources[1] };
	for (UI_IMAGE_RESOURCE* image : images)
	{
		if (image->pd3dTexture) image->pd3dTexture->Release();
		if (image->pd3dTextureUploadBuffer) image->pd3dTextureUploadBuffer->Release();
		if (image->pd3dSrvDescriptorHeap) image->pd3dSrvDescriptorHeap->Release();
	}
}

void CShopUI::Animate(float elapsedTime)
{
	if (m_bShopActive && m_eShopPage == SHOP_PAGE::SLOT_CONTENT_1 && m_pPreviewPet)
		m_pPreviewPet->AnimateWithoutMovement(elapsedTime);
}

void CShopUI::ReleaseUploadBuffers()
{
	UI_IMAGE_RESOURCE* images[] = { &m_ShopIconResource, &m_ShopBoardResource,
		&m_ShopCloseIconResource, &m_ShopBackSpaceIconResource, &m_ShopSlotResources[0],
		&m_ShopSlotResources[1], &m_ShopSlotResources[2], &m_ShopSlotResources[3],
		&m_EmptySquareResources[0], &m_EmptySquareResources[1], &m_PetConfirmationButtonResource,
		&m_ScrollBackgroundResource, &m_ScrollResource, &m_EmptyFrameResource,
		&m_PetEnhanceButtonResource, &m_PetEnhancePriceFrameResource,
		&m_PetEnhanceLogResources[0], &m_PetEnhanceLogResources[1] };
	for (UI_IMAGE_RESOURCE* image : images)
	{
		if (image->pd3dTextureUploadBuffer)
		{
			image->pd3dTextureUploadBuffer->Release();
			image->pd3dTextureUploadBuffer = NULL;
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
	const std::string debug = "[RenderTextLine] Text: " + text + ", Width: "
		+ std::to_string(textWidth) + "\n";
	OutputDebugStringA(debug.c_str());
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
	for (char ch : FormatPossession(money) + "$")
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
	const std::string text = FormatPossession(money) + "$";
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

void CShopUI::RenderEnhancementPage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	CPet* activePet, UINT money, const SHOP_TEXT_RENDER_CONTEXT& context)
{
	if (!activePet) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	auto nextValue = [](UINT value, int type) -> UINT
	{
		UINT ratePercent = 110;
		if ((type == 0 && value >= 1000) || (type == 1 && value >= 10000))
			ratePercent = 101;
		else if ((type == 0 && value >= 100) || (type == 1 && value >= 1000))
			ratePercent = 105;
		const UINT64 enhanced = (static_cast<UINT64>(value) * ratePercent + 99) / 100;
		return static_cast<UINT>((enhanced > UINT_MAX) ? UINT_MAX : enhanced);
	};
	auto enhancementPrice = [](UINT value, int type) -> UINT
	{
		const UINT64 price = (type == 0)
			? static_cast<UINT64>(value) * 10
			: (static_cast<UINT64>(value) * 4 + 4) / 5;
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
		const UINT values[2] = { currentValues[type], nextValue(currentValues[type], type) };
		for (int frameIndex = 0; frameIndex < 2; ++frameIndex)
		{
			const XMFLOAT4 frame(frameLeft, frameTops[frameIndex], frameLeft + frameWidth,
				frameTops[frameIndex] + frameHeight);
			RenderUiImage(commandList, camera, m_EmptyFrameResource, frame);
			const std::string text = ((type == 1)
				? FormatPossessionTwoDecimals(values[frameIndex]) : FormatPossession(values[frameIndex]))
				+ ((type == 0) ? "$ / S" : "$");
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
		const std::string priceText = FormatPossession(price) + "$";
		const float priceScale = 0.12f;
		const float priceLeft = (priceFrame.x + priceFrame.z - measureText(priceText, priceScale)) * 0.5f;
		const float priceVisibleHeight = 190.0f * priceScale;
		const float priceTop = priceFrame.y + ((priceFrame.w - priceFrame.y - priceVisibleHeight) * 0.5f)
			+ 129.0f * priceScale - (priceFrame.w - priceFrame.y) * 0.38f;
		RenderTextLine(commandList, camera, priceText, priceLeft, priceTop, priceScale,
			(money < price) ? 0x00FF0000 : 0x00000000, context);
	}
}

void CShopUI::Render(ID3D12GraphicsCommandList* commandList, CCamera* camera, UINT money,
	size_t activePetIndex, const std::vector<SHOP_PET_RENDER_RESOURCE>& pets,
	const SHOP_TEXT_RENDER_CONTEXT& context)
{
	if (!camera) return;
	RebuildPetScrollMetricsIfNeeded(pets.size());
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	if (m_bShopActive)
	{
		RenderUiImage(commandList, camera, m_ShopBoardResource,
			GetShopBoardRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
		if (m_eShopPage == SHOP_PAGE::SLOT_MENU)
		{
			for (int i = 0; i < 4; ++i)
				RenderUiImage(commandList, camera, m_ShopSlotResources[i],
					GetShopSlotRectangle(i, width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
		}
		else if (m_eShopPage == SHOP_PAGE::SLOT_CONTENT_1)
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
		else if (m_eShopPage == SHOP_PAGE::SLOT_CONTENT_2)
		{
			CPet* activePet = (activePetIndex < pets.size()) ? pets[activePetIndex].pPet : NULL;
			RenderEnhancementPage(commandList, camera, activePet, money, context);
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
	m_nPressedEnhanceButton = -1;
	ResetSelectedPet(activePetIndex, m_nCachedPetCount);
}

bool CShopUI::ProcessShopUIClick(float x, float y, float width, float height, UINT money,
	size_t petCount, size_t activePetIndex, const SHOP_TEXT_RENDER_CONTEXT& context)
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
			m_eShopPage = SHOP_PAGE::SLOT_MENU;
			m_nSelectedShopSlot = -1;
			ResetSelectedPet(activePetIndex, petCount);
		}
		return(true);
	}
	if (!m_bShopActive) return(false);
	if (IsPointInRectangle(x, y,
		GetShopBackRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y)))
	{
		if (m_eShopPage == SHOP_PAGE::SLOT_MENU) DeactivateShop(width, height, activePetIndex);
		else
		{
			m_eShopPage = SHOP_PAGE::SLOT_MENU;
			m_nSelectedShopSlot = -1;
			m_nPressedEnhanceButton = -1;
			ResetSelectedPet(activePetIndex, petCount);
		}
		return(true);
	}
	if (m_eShopPage == SHOP_PAGE::SLOT_MENU)
	{
		for (int i = 0; i < 4; ++i)
		{
			if (!IsPointInRectangle(x, y, GetShopSlotRectangle(i, width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y))) continue;
			m_eShopPage = static_cast<SHOP_PAGE>(static_cast<int>(SHOP_PAGE::SLOT_CONTENT_1) + i);
			m_nSelectedShopSlot = i;
			if (m_eShopPage == SHOP_PAGE::SLOT_CONTENT_1)
				ResetSelectedPet(activePetIndex, petCount);
			return(true);
		}
	}
	else if (m_eShopPage == SHOP_PAGE::SLOT_CONTENT_1)
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
	else if (m_eShopPage == SHOP_PAGE::SLOT_CONTENT_2)
	{
		const XMFLOAT4 left = GetPetContentPanelRectangle(false, width, height,
			m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
		const XMFLOAT4 right = GetPetContentPanelRectangle(true, width, height,
			m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
		if (IsPointInRectangle(x, y, left) || IsPointInRectangle(x, y, right)) return(true);
	}
	return(false);
}

bool CShopUI::OnProcessingMouseMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
	UINT money, size_t nPetCount, size_t activePetIndex, const SHOP_TEXT_RENDER_CONTEXT& context)
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
		if (m_bShopActive && m_eShopPage == SHOP_PAGE::SLOT_CONTENT_1)
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
		break;
	case WM_LBUTTONDOWN:
		RebuildPetScrollMetricsIfNeeded(nPetCount);
		if (m_bShopActive && m_eShopPage == SHOP_PAGE::SLOT_CONTENT_2)
		{
			for (int type = 0; type < 2; ++type)
			{
				if (!IsPointInRectangle(x, y, GetEnhanceButtonRectangle(type, width, height))) continue;
				m_nPressedEnhanceButton = type;
				SetCapture(hWnd);
				return(true);
			}
		}
		if (m_bShopActive && m_eShopPage == SHOP_PAGE::SLOT_CONTENT_1
			&& m_nMaximumPetScrollOffset > 0
			&& IsPointInRectangle(x, y, GetPetScrollThumbRectangle(width, height)))
		{
			m_bPetScrollDragging = true;
			m_fPetScrollDragLastY = y;
			return(true);
		}
		if (ProcessShopUIClick(x, y, width, height, money, nPetCount, activePetIndex, context)) return(true);
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
		if (m_nPressedEnhanceButton >= 0)
		{
			const int pressedButton = m_nPressedEnhanceButton;
			m_nPressedEnhanceButton = -1;
			if (m_bShopActive && m_eShopPage == SHOP_PAGE::SLOT_CONTENT_2
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
