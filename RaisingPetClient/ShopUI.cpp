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
}

void CShopUI::ReleaseObjects()
{
	if (m_pd3dUiImagePipelineState) m_pd3dUiImagePipelineState->Release();
	UI_IMAGE_RESOURCE* images[] = { &m_ShopIconResource, &m_ShopBoardResource,
		&m_ShopCloseIconResource, &m_ShopBackSpaceIconResource, &m_ShopSlotResources[0],
		&m_ShopSlotResources[1], &m_ShopSlotResources[2], &m_ShopSlotResources[3],
		&m_EmptySquareResources[0], &m_EmptySquareResources[1], &m_PetConfirmationButtonResource,
		&m_ScrollBackgroundResource, &m_ScrollResource };
	for (UI_IMAGE_RESOURCE* image : images)
	{
		if (image->pd3dTexture) image->pd3dTexture->Release();
		if (image->pd3dTextureUploadBuffer) image->pd3dTextureUploadBuffer->Release();
		if (image->pd3dSrvDescriptorHeap) image->pd3dSrvDescriptorHeap->Release();
	}
}

void CShopUI::ReleaseUploadBuffers()
{
	UI_IMAGE_RESOURCE* images[] = { &m_ShopIconResource, &m_ShopBoardResource,
		&m_ShopCloseIconResource, &m_ShopBackSpaceIconResource, &m_ShopSlotResources[0],
		&m_ShopSlotResources[1], &m_ShopSlotResources[2], &m_ShopSlotResources[3],
		&m_EmptySquareResources[0], &m_EmptySquareResources[1], &m_PetConfirmationButtonResource,
		&m_ScrollBackgroundResource, &m_ScrollResource };
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
	UI_IMAGE_RESOURCE& image, const XMFLOAT4& rectangle)
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

void CShopUI::Render(ID3D12GraphicsCommandList* commandList, CCamera* camera, UINT money,
	const std::vector<CPet*>& pets, const SHOP_TEXT_RENDER_CONTEXT& context)
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
				if (petIndex >= pets.size() || !pets[petIndex]) continue;
				if (petIndex == m_nSelectedPetIndex)
				{
					const XMFLOAT4 selection = GetPetListRowRectangle(row, width, height);
					RenderSolidUiRectangle(commandList, camera, selection.x, selection.y,
						selection.z, selection.w, 0x00BFBFBF, context);
				}
				const std::string name = std::to_string(petIndex + 1) + ". " + pets[petIndex]->GetName();
				RenderTextLine(commandList, camera, name, leftPanel.x + 18.0f,
					leftPanel.y + rowHeight * row + rowHeight * 0.15f, glyphScale, 0x00000000, context);
			}
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
		for (size_t row = 0; row < 10; ++row)
		{
			const size_t petIndex = m_nPetScrollOffset + row;
			if (petIndex >= petCount) break;
			if (!IsPointInRectangle(x, y, GetPetListRowRectangle(row, width, height))) continue;
			m_nSelectedPetIndex = petIndex;
			return(true);
		}
		if (IsPointInRectangle(x, y, left) || IsPointInRectangle(x, y, right)
			|| IsPointInRectangle(x, y, confirm)) return(true);
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
		if (m_bPetScrollDragging) { m_bPetScrollDragging = false; return(true); }
		if (m_bShopBoardDragging) { m_bShopBoardDragging = false; return(true); }
		break;
	}
	return(false);
}
