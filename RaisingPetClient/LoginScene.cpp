#include "stdafx.h"
#include "LoginScene.h"
#include "GameFramework.h"
#include "DDSTextureLoader12.h"
#include "d3dx12.h"

extern CGameFramework* g_pFramework;

namespace
{
bool PointInRectangle(float x, float y, const XMFLOAT4& rectangle)
{
	return(x >= rectangle.x && x <= rectangle.z && y >= rectangle.y && y <= rectangle.w);
}

bool IsLoginInputCharacter(char ch)
{
	return((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
		(ch >= '0' && ch <= '9'));
}

const char* GetAccountInformationFilePath()
{
	return("Network\\AccountInformation.rpacct");
}

UINT CalculateAccountChecksum(const std::vector<unsigned char>& data)
{
	UINT checksum = 2166136261u;
	for (unsigned char byte : data)
	{
		checksum ^= byte;
		checksum *= 16777619u;
	}
	return(checksum);
}

void TransformAccountPayload(std::vector<unsigned char>& data)
{
	const unsigned char key[] = { 0x52, 0x61, 0x69, 0x73, 0x69, 0x6E, 0x67, 0x50, 0x65, 0x74 };
	unsigned char rolling = 0xA7;
	for (size_t i = 0; i < data.size(); ++i)
	{
		const unsigned char mixedKey = static_cast<unsigned char>(key[i % _countof(key)]
			+ static_cast<unsigned char>(i * 31) + rolling);
		data[i] ^= mixedKey;
		rolling = static_cast<unsigned char>(rolling * 33u + 17u);
	}
}

bool IsAccountTextValid(const std::string& text)
{
	if (text.empty() || text.size() > 256) return(false);
	for (char ch : text)
		if (!IsLoginInputCharacter(ch)) return(false);
	return(true);
}

XMFLOAT4 GetBoardRectangle(float width, float height)
{
	float boardWidth = min(width * 0.82f, 900.0f);
	float boardHeight = boardWidth * (2017.0f / 2923.0f);
	if (boardHeight > height * 0.86f)
	{
		boardHeight = height * 0.86f;
		boardWidth = boardHeight * (2923.0f / 2017.0f);
	}
	const float left = (width - boardWidth) * 0.5f;
	const float top = (height - boardHeight) * 0.5f;
	return(XMFLOAT4(left, top, left + boardWidth, top + boardHeight));
}

XMFLOAT4 GetCloseRectangle(float width, float height)
{
	const XMFLOAT4 board = GetBoardRectangle(width, height);
	const float buttonHeight = (board.w - board.y) * 0.09f;
	const float buttonWidth = buttonHeight * (205.0f / 232.0f);
	const float right = board.z - (board.z - board.x) * 0.04f;
	const float top = board.y + (board.w - board.y) * 0.05f;
	return(XMFLOAT4(right - buttonWidth, top, right, top + buttonHeight));
}

XMFLOAT4 GetLoginFrameRectangle(float width, float height)
{
	const XMFLOAT4 board = GetBoardRectangle(width, height);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float left = board.x + boardWidth * 0.035f;
	const float top = board.y + boardHeight * 0.21f;
	const float frameWidth = boardWidth * 0.69f;
	return(XMFLOAT4(left, top, left + frameWidth,
		top + frameWidth * (1208.0f / 1906.0f)));
}

XMFLOAT4 GetLoginActionButtonRectangle(int buttonIndex, float width, float height)
{
	const XMFLOAT4 board = GetBoardRectangle(width, height);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float buttonWidth = boardWidth * 0.17f;
	const float buttonHeight = buttonWidth * (216.0f / 447.0f);
	const float left = board.x + boardWidth * 0.775f;
	const float topRatios[3] = { 0.32f, 0.48f, 0.64f };
	const float top = board.y + boardHeight * topRatios[buttonIndex];
	return(XMFLOAT4(left, top, left + buttonWidth, top + buttonHeight));
}

XMFLOAT4 GetLoginButtonRectangle(bool guest, float width, float height)
{
	return(GetLoginActionButtonRectangle(guest ? 2 : 1, width, height));
}

XMFLOAT4 GetRegisterButtonRectangle(float width, float height)
{
	return(GetLoginActionButtonRectangle(0, width, height));
}

XMFLOAT4 GetLoginErrorLogRectangle(float width, float height, int type)
{
	const XMFLOAT4 board = GetBoardRectangle(width, height);
	const XMFLOAT4 anchorButton = (type == 2 || type == 3)
		? GetRegisterButtonRectangle(width, height)
		: GetLoginButtonRectangle(false, width, height);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float logHeight = (boardWidth * 0.48f) * (134.0f / 1473.0f);
	const float logWidth = (type == 1)
		? logHeight * (1756.0f / 133.0f)
		: (type == 2)
		? logHeight * (1472.0f / 134.0f)
		: (type == 3)
		? logHeight * (1472.0f / 133.0f)
		: logHeight * (1473.0f / 134.0f);
	const float centerX = (anchorButton.x + anchorButton.z) * 0.5f;
	const float top = anchorButton.y - logHeight - boardHeight * 0.035f;
	return(XMFLOAT4(centerX - logWidth * 0.5f, top, centerX + logWidth * 0.5f, top + logHeight));
}

XMFLOAT4 GetLoginLoadingRectangle(float width, float height)
{
	const XMFLOAT4 board = GetBoardRectangle(width, height);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float loadingSize = boardHeight * 0.42f;
	const float centerX = (board.x + board.z) * 0.5f;
	const float centerY = board.y + boardHeight * 0.35f;
	return(XMFLOAT4(centerX - loadingSize * 0.5f, centerY - loadingSize * 0.5f,
		centerX + loadingSize * 0.5f, centerY + loadingSize * 0.5f));
}

XMFLOAT4 GetLoadingTextRectangle(float width, float height)
{
	const XMFLOAT4 board = GetBoardRectangle(width, height);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float textWidth = boardWidth * 0.45f;
	const float textHeight = textWidth * (232.0f / 2895.0f);
	const float centerX = (board.x + board.z) * 0.5f;
	const float centerY = board.y + boardHeight * 0.63f;
	return(XMFLOAT4(centerX - textWidth * 0.5f, centerY - textHeight * 0.5f,
		centerX + textWidth * 0.5f, centerY + textHeight * 0.5f));
}

XMFLOAT4 GetDirectStartButtonRectangle(float width, float height)
{
	const XMFLOAT4 board = GetBoardRectangle(width, height);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float buttonWidth = boardWidth * 0.34f;
	const float buttonHeight = buttonWidth * (196.0f / 993.0f);
	const float centerX = (board.x + board.z) * 0.5f;
	const float top = board.y + boardHeight * 0.81f;
	return(XMFLOAT4(centerX - buttonWidth * 0.5f, top,
		centerX + buttonWidth * 0.5f, top + buttonHeight));
}

XMFLOAT4 GetLabelRectangle(bool password, float width, float height)
{
	const XMFLOAT4 frame = GetLoginFrameRectangle(width, height);
	const float frameWidth = frame.z - frame.x;
	const float frameHeight = frame.w - frame.y;
	const float labelWidth = frameWidth * 0.16f;
	const float labelHeight = labelWidth * ((password ? 274.0f : 237.0f) / (password ? 313.0f : 314.0f));
	const float left = frame.x + frameWidth * 0.055f;
	const float centerY = frame.y + frameHeight * (password ? 0.70f : 0.32f);
	return(XMFLOAT4(left, centerY - labelHeight * 0.5f,
		left + labelWidth, centerY + labelHeight * 0.5f));
}

XMFLOAT4 GetTextFrameRectangle(bool password, float width, float height)
{
	const XMFLOAT4 frame = GetLoginFrameRectangle(width, height);
	const XMFLOAT4 label = GetLabelRectangle(password, width, height);
	const float frameWidth = frame.z - frame.x;
	const float textWidth = frameWidth * 0.64f;
	const float textHeight = textWidth * (156.0f / 1254.0f);
	const float left = frame.x + frameWidth * 0.245f;
	const float centerY = (label.y + label.w) * 0.5f;
	return(XMFLOAT4(left, centerY - textHeight * 0.5f,
		left + textWidth, centerY + textHeight * 0.5f));
}

XMFLOAT4 GetPasswordHideIconRectangle(float width, float height)
{
	const XMFLOAT4 textFrame = GetTextFrameRectangle(true, width, height);
	const float frameHeight = textFrame.w - textFrame.y;
	const float iconWidth = frameHeight * 0.75f;
	const float iconHeight = iconWidth * (70.0f / 118.0f);
	const float left = textFrame.z + frameHeight * 0.18f;
	const float top = textFrame.y - iconHeight - frameHeight * 0.10f;
	return(XMFLOAT4(left, top, left + iconWidth, top + iconHeight));
}

XMFLOAT4 GetPasswordHideCheckBoxRectangle(float width, float height)
{
	const XMFLOAT4 textFrame = GetTextFrameRectangle(true, width, height);
	const float frameHeight = textFrame.w - textFrame.y;
	const float boxHeight = frameHeight * 0.82f;
	const float boxWidth = boxHeight * (105.0f / 107.0f);
	const float left = textFrame.z + frameHeight * 0.18f;
	const float top = textFrame.y + (frameHeight - boxHeight) * 0.5f;
	return(XMFLOAT4(left, top, left + boxWidth, top + boxHeight));
}

struct GLYPH_METRIC
{
	char ch;
	float imageWidth;
	float left;
	float top;
	float right;
	float bottom;
};

const GLYPH_METRIC gGlyphMetrics[] =
{
	{ '0',441,158,136,284,322 }, { '1',441,175,136,248,318 },
	{ '2',441,164,136,278,318 }, { '3',441,166,136,278,322 },
	{ '4',441,155,139,286,318 }, { '5',441,167,139,278,322 },
	{ '6',441,161,136,284,322 }, { '7',441,160,139,282,318 },
	{ '8',441,161,136,283,322 }, { '9',441,159,136,282,322 },
	{ '.',363,161,283,203,322 }, { ',',363,154,287,199,350 },
	{ '/',407,144,139,261,348 }, { '$',441,165,116,280,345 },
	{ '*',410,157,139,253,232 },
	{ 'a',433,157,187,269,322 }, { 'A',472,150,139,321,318 },
	{ 'b',453,166,129,295,322 }, { 'B',455,170,139,297,318 },
	{ 'c',419,159,187,261,322 }, { 'C',456,158,136,296,322 },
	{ 'd',453,159,129,287,322 }, { 'D',483,170,139,325,318 },
	{ 'e',435,159,187,278,322 }, { 'E',431,170,139,274,318 },
	{ 'f',390,154,126,244,318 }, { 'F',428,169,139,270,318 },
	{ 'g',453,159,187,287,378 }, { 'G',477,159,136,313,322 },
	{ 'h',448,166,129,284,318 }, { 'H',488,169,139,318,318 },
	{ 'i',366,163,130,203,319 }, { 'I',375,170,139,205,318 },
	{ 'j',367,130,130,204,378 }, { 'J',405,153,139,237,322 },
	{ 'k',435,166,129,288,318 }, { 'K',458,169,139,311,318 },
	{ 'l',366,165,129,200,318 }, { 'L',425,170,139,273,318 },
	{ 'm',527,167,187,363,319 }, { 'M',537,170,139,368,318 },
	{ 'n',449,167,187,285,319 }, { 'N',495,170,139,326,318 },
	{ 'o',451,158,187,293,322 }, { 'O',490,158,136,332,322 },
	{ 'p',453,166,187,295,377 }, { 'P',451,170,139,295,318 },
	{ 'q',453,159,187,287,377 }, { 'Q',490,158,136,349,342 },
	{ 'r',395,166,188,245,319 }, { 'R',459,170,139,313,318 },
	{ 's',413,162,187,257,322 }, { 'S',438,161,136,280,322 },
	{ 't',393,154,152,239,321 }, { 'T',443,153,139,291,318 },
	{ 'u',449,164,190,283,322 }, { 'U',479,168,139,312,322 },
	{ 'v',431,150,190,282,319 }, { 'V',464,150,139,314,318 },
	{ 'w',495,151,190,344,319 }, { 'W',548,151,139,397,318 },
	{ 'x',431,150,190,281,319 }, { 'X',459,150,139,309,318 },
	{ 'y',431,150,190,283,378 }, { 'Y',448,149,139,299,318 },
	{ 'z',418,152,190,264,319 }, { 'Z',449,153,139,295,318 }
};
}

void CLoginScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	ID3DBlob* vertexShader = NULL;
	ID3DBlob* pixelShader = NULL;
	D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "VSTextGlyph", "vs_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &vertexShader, NULL);
	D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "PSUiImage", "ps_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pixelShader, NULL);
	if (!vertexShader || !pixelShader) return;

	CShader stateFactory;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline = {};
	pipeline.pRootSignature = m_pd3dGraphicsRootSignature;
	pipeline.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	pipeline.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
	pipeline.RasterizerState = stateFactory.CreateRasterizerState();
	pipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	pipeline.BlendState = stateFactory.CreateBlendState();
	pipeline.BlendState.RenderTarget[0].BlendEnable = TRUE;
	pipeline.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	pipeline.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	pipeline.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	pipeline.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	pipeline.DepthStencilState = stateFactory.CreateDepthStencilState();
	pipeline.DepthStencilState.DepthEnable = FALSE;
	pipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	pipeline.SampleMask = UINT_MAX;
	pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipeline.NumRenderTargets = 1;
	pipeline.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
	pipeline.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	pipeline.SampleDesc.Count = 1;
	device->CreateGraphicsPipelineState(&pipeline, __uuidof(ID3D12PipelineState),
		reinterpret_cast<void**>(&m_pd3dUiPipelineState));
	ID3DBlob* rotatingVertexShader = NULL;
	D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "VSLoginLoading", "vs_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &rotatingVertexShader, NULL);
	if (rotatingVertexShader)
	{
		pipeline.VS = { rotatingVertexShader->GetBufferPointer(), rotatingVertexShader->GetBufferSize() };
		device->CreateGraphicsPipelineState(&pipeline, __uuidof(ID3D12PipelineState),
			reinterpret_cast<void**>(&m_pd3dRotatingUiPipelineState));
		pipeline.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
		rotatingVertexShader->Release();
	}
	pixelShader->Release();
	pixelShader = NULL;
	D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "PSTextGlyph", "ps_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pixelShader, NULL);
	if (pixelShader)
	{
		pipeline.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
		device->CreateGraphicsPipelineState(&pipeline, __uuidof(ID3D12PipelineState),
			reinterpret_cast<void**>(&m_pd3dTextPipelineState));
		pixelShader->Release();
	}
	vertexShader->Release();

	auto loadImage = [device, commandList](const wchar_t* fileName, UI_IMAGE_RESOURCE& image)
	{
		std::unique_ptr<uint8_t[]> data;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		if (FAILED(DirectX::LoadDDSTextureFromFile(device, fileName,
			&image.pd3dTexture, data, subresources)) || subresources.empty()) return;
		const UINT64 uploadSize = GetRequiredIntermediateSize(image.pd3dTexture, 0,
			static_cast<UINT>(subresources.size()));
		CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);
		device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, NULL, __uuidof(ID3D12Resource),
			reinterpret_cast<void**>(&image.pd3dTextureUploadBuffer));
		UpdateSubresources(commandList, image.pd3dTexture, image.pd3dTextureUploadBuffer,
			0, 0, static_cast<UINT>(subresources.size()), subresources.data());
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			image.pd3dTexture, D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->ResourceBarrier(1, &barrier);
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap),
			reinterpret_cast<void**>(&image.pd3dSrvDescriptorHeap));
		D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv.Format = image.pd3dTexture->GetDesc().Format;
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = image.pd3dTexture->GetDesc().MipLevels;
		device->CreateShaderResourceView(image.pd3dTexture, &srv,
			image.pd3dSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	};

	loadImage(L"Assets/Image/Shop/ShopBoard.dds", m_ShopBoard);
	loadImage(L"Assets/Image/Shop/ShopCloseIcon.dds", m_CloseIcon);
	loadImage(L"Assets/Image/Login/LoginFrame.dds", m_LoginFrame);
	loadImage(L"Assets/Image/Login/IDLog.dds", m_IdLog);
	loadImage(L"Assets/Image/Login/PasswordLog.dds", m_PasswordLog);
	loadImage(L"Assets/Image/Login/TextFrame.dds", m_TextFrame);
	loadImage(L"Assets/Image/Login/LoginButton.dds", m_LoginButton);
	loadImage(L"Assets/Image/Login/GuestButton.dds", m_GuestButton);
	loadImage(L"Assets/Image/Login/RegisterButton.dds", m_RegisterButton);
	loadImage(L"Assets/Image/Login/TextCursor.dds", m_TextCursor);
	loadImage(L"Assets/Image/Login/LoginErrorLog.dds", m_LoginErrorLog);
	loadImage(L"Assets/Image/Login/LoginErrorLog2.dds", m_LoginErrorLog2);
	loadImage(L"Assets/Image/Login/RegisterSuccessLog.dds", m_RegisterSuccessLog);
	loadImage(L"Assets/Image/Login/RegisterFailLog.dds", m_RegisterFailLog);
	loadImage(L"Assets/Image/Login/LoginLoading.dds", m_LoginLoading);
	loadImage(L"Assets/Image/Login/LoadingText1.dds", m_LoadingTexts[0]);
	loadImage(L"Assets/Image/Login/LoadingText2.dds", m_LoadingTexts[1]);
	loadImage(L"Assets/Image/Login/LoadingText3.dds", m_LoadingTexts[2]);
	loadImage(L"Assets/Image/Login/DirectStartButton.dds", m_DirectStartButton);
	loadImage(L"Assets/Image/Login/LoginBackButton.dds", m_LoginBackButton);
	loadImage(L"Assets/Image/Login/PasswordHideIcon.dds", m_PasswordHideIcon);
	loadImage(L"Assets/Image/Login/PasswordHideCheckBox.dds", m_PasswordHideCheckBox);
	for (const GLYPH_METRIC& metric : gGlyphMetrics)
	{
		m_Glyphs.emplace_back();
		GLYPH_RESOURCE& glyph = m_Glyphs.back();
		glyph.ch = metric.ch;
		glyph.imageWidth = metric.imageWidth;
		glyph.imageHeight = 521.0f;
		glyph.pixelWidth = metric.right - metric.left;
		glyph.pixelHeight = metric.bottom - metric.top;
		glyph.u0 = metric.left / metric.imageWidth;
		glyph.v0 = metric.top / 521.0f;
		glyph.topOffset = metric.top - 129.0f;
		std::wstring fileName;
		if (metric.ch >= '0' && metric.ch <= '9')
			fileName = L"Assets/Image/Numbers/" + std::wstring(1, static_cast<wchar_t>(metric.ch)) + L".dds";
		else if (metric.ch == '.') fileName = L"Assets/Image/Numbers/Point.dds";
		else if (metric.ch == ',') fileName = L"Assets/Image/Numbers/Comma.dds";
		else if (metric.ch == '/') fileName = L"Assets/Image/Numbers/Slash.dds";
		else if (metric.ch == '$') fileName = L"Assets/Image/Numbers/Dollar.dds";
		else if (metric.ch == '*') fileName = L"Assets/Image/Numbers/Star.dds";
		else
		{
			const wchar_t lower = static_cast<wchar_t>(tolower(static_cast<unsigned char>(metric.ch)));
			fileName = L"Assets/Image/Spellings/" + std::wstring(1, lower)
				+ ((metric.ch >= 'A' && metric.ch <= 'Z') ? L"2.dds" : L".dds");
		}
		loadImage(fileName.c_str(), glyph.image);
	}
	LoadSavedAccountInformation();
}

void CLoginScene::ReleaseObjects()
{
	if (m_pd3dUiPipelineState) m_pd3dUiPipelineState->Release();
	m_pd3dUiPipelineState = NULL;
	if (m_pd3dTextPipelineState) m_pd3dTextPipelineState->Release();
	m_pd3dTextPipelineState = NULL;
	if (m_pd3dRotatingUiPipelineState) m_pd3dRotatingUiPipelineState->Release();
	m_pd3dRotatingUiPipelineState = NULL;
	UI_IMAGE_RESOURCE* resources[] = { &m_ShopBoard, &m_CloseIcon, &m_LoginFrame,
		&m_IdLog, &m_PasswordLog, &m_TextFrame, &m_LoginButton, &m_GuestButton,
		&m_RegisterButton, &m_TextCursor, &m_LoginErrorLog, &m_LoginErrorLog2,
		&m_RegisterSuccessLog, &m_RegisterFailLog, &m_LoginLoading, &m_LoadingTexts[0],
		&m_LoadingTexts[1], &m_LoadingTexts[2], &m_DirectStartButton, &m_LoginBackButton,
		&m_PasswordHideIcon, &m_PasswordHideCheckBox };
	for (UI_IMAGE_RESOURCE* resource : resources)
	{
		if (resource->pd3dTexture) resource->pd3dTexture->Release();
		if (resource->pd3dTextureUploadBuffer) resource->pd3dTextureUploadBuffer->Release();
		if (resource->pd3dSrvDescriptorHeap) resource->pd3dSrvDescriptorHeap->Release();
		*resource = UI_IMAGE_RESOURCE();
	}
	for (GLYPH_RESOURCE& glyph : m_Glyphs)
	{
		if (glyph.image.pd3dTexture) glyph.image.pd3dTexture->Release();
		if (glyph.image.pd3dTextureUploadBuffer) glyph.image.pd3dTextureUploadBuffer->Release();
		if (glyph.image.pd3dSrvDescriptorHeap) glyph.image.pd3dSrvDescriptorHeap->Release();
	}
	m_Glyphs.clear();
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();
	m_pd3dGraphicsRootSignature = NULL;
}

void CLoginScene::ReleaseUploadBuffers()
{
	UI_IMAGE_RESOURCE* resources[] = { &m_ShopBoard, &m_CloseIcon, &m_LoginFrame,
		&m_IdLog, &m_PasswordLog, &m_TextFrame, &m_LoginButton, &m_GuestButton,
		&m_RegisterButton, &m_TextCursor, &m_LoginErrorLog, &m_LoginErrorLog2,
		&m_RegisterSuccessLog, &m_RegisterFailLog, &m_LoginLoading, &m_LoadingTexts[0],
		&m_LoadingTexts[1], &m_LoadingTexts[2], &m_DirectStartButton, &m_LoginBackButton,
		&m_PasswordHideIcon, &m_PasswordHideCheckBox };
	for (UI_IMAGE_RESOURCE* resource : resources)
	{
		if (!resource->pd3dTextureUploadBuffer) continue;
		resource->pd3dTextureUploadBuffer->Release();
		resource->pd3dTextureUploadBuffer = NULL;
	}
	for (GLYPH_RESOURCE& glyph : m_Glyphs)
	{
		if (!glyph.image.pd3dTextureUploadBuffer) continue;
		glyph.image.pd3dTextureUploadBuffer->Release();
		glyph.image.pd3dTextureUploadBuffer = NULL;
	}
}

void CLoginScene::RenderUiImage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	UI_IMAGE_RESOURCE& resource, const XMFLOAT4& rectangle, UINT color)
{
	if (!m_pd3dUiPipelineState || !resource.pd3dSrvDescriptorHeap || !camera) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	const float constants[4] = { rectangle.x / width * 2.0f - 1.0f,
		1.0f - rectangle.y / height * 2.0f, rectangle.z / width * 2.0f - 1.0f,
		1.0f - rectangle.w / height * 2.0f };
	ID3D12DescriptorHeap* heaps[] = { resource.pd3dSrvDescriptorHeap };
	commandList->SetDescriptorHeaps(1, heaps);
	commandList->SetGraphicsRootDescriptorTable(3,
		resource.pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRoot32BitConstants(4, 4, constants, 0);
	commandList->SetGraphicsRoot32BitConstant(5, color, 0);
	commandList->SetPipelineState(m_pd3dUiPipelineState);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(6, 1, 0, 0);
}

void CLoginScene::RenderRotatingUiImage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	UI_IMAGE_RESOURCE& resource, const XMFLOAT4& rectangle, UINT color)
{
	if (!m_pd3dRotatingUiPipelineState || !resource.pd3dSrvDescriptorHeap || !camera) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	const float constants[4] = { rectangle.x / width * 2.0f - 1.0f,
		1.0f - rectangle.y / height * 2.0f, rectangle.z / width * 2.0f - 1.0f,
		1.0f - rectangle.w / height * 2.0f };
	ID3D12DescriptorHeap* heaps[] = { resource.pd3dSrvDescriptorHeap };
	commandList->SetDescriptorHeaps(1, heaps);
	commandList->SetGraphicsRootDescriptorTable(3,
		resource.pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRoot32BitConstants(4, 4, constants, 0);
	commandList->SetGraphicsRoot32BitConstant(5, color, 0);
	commandList->SetPipelineState(m_pd3dRotatingUiPipelineState);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(6, 1, 0, 0);
}

CLoginScene::GLYPH_RESOURCE* CLoginScene::FindGlyph(char ch)
{
	for (GLYPH_RESOURCE& glyph : m_Glyphs)
		if (glyph.ch == ch) return(&glyph);
	return(NULL);
}

float CLoginScene::GetGlyphAdvance(char ch, float scale) const
{
	for (const GLYPH_RESOURCE& glyph : m_Glyphs)
		if (glyph.ch == ch) return(glyph.pixelWidth * scale + max(3.0f, scale * 24.0f));
	return(0.0f);
}

float CLoginScene::MeasureText(const std::string& text, size_t characterCount, float scale) const
{
	float width = 0.0f;
	const size_t count = min(characterCount, text.size());
	for (size_t i = 0; i < count; ++i) width += GetGlyphAdvance(text[i], scale);
	return(width);
}

void CLoginScene::RenderTextField(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	int fieldIndex, const XMFLOAT4& rectangle)
{
	if (!m_pd3dTextPipelineState || !camera) return;
	const std::string& actualText = (fieldIndex == 0) ? m_LoginId : m_LoginPassword;
	const std::string hiddenPasswordText(actualText.size(), '*');
	const std::string& text = (fieldIndex == 1 && !m_bPasswordVisible) ? hiddenPasswordText : actualText;
	const float frameHeight = rectangle.w - rectangle.y;
	const float scale = frameHeight / 330.0f;
	const float left = rectangle.x + frameHeight * 0.30f;
	const float top = rectangle.y + (frameHeight - 190.0f * scale) * 0.5f
		+ 129.0f * scale - frameHeight * 0.40f;
	const float viewportWidth = camera->m_d3dViewport.Width;
	const float viewportHeight = camera->m_d3dViewport.Height;
	float cursorX = left;

	commandList->SetPipelineState(m_pd3dTextPipelineState);
	commandList->SetGraphicsRoot32BitConstant(5, 0x00000000, 0);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	for (char ch : text)
	{
		GLYPH_RESOURCE* glyph = FindGlyph(ch);
		if (!glyph || !glyph->image.pd3dSrvDescriptorHeap) continue;
		const float imageLeft = cursorX - glyph->u0 * glyph->imageWidth * scale;
		const float imageTop = top + glyph->topOffset * scale - glyph->v0 * glyph->imageHeight * scale;
		const float constants[4] = {
			imageLeft / viewportWidth * 2.0f - 1.0f,
			1.0f - imageTop / viewportHeight * 2.0f,
			(imageLeft + glyph->imageWidth * scale) / viewportWidth * 2.0f - 1.0f,
			1.0f - (imageTop + glyph->imageHeight * scale) / viewportHeight * 2.0f
		};
		ID3D12DescriptorHeap* heaps[] = { glyph->image.pd3dSrvDescriptorHeap };
		commandList->SetDescriptorHeaps(1, heaps);
		commandList->SetGraphicsRootDescriptorTable(3,
			glyph->image.pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		commandList->SetGraphicsRoot32BitConstants(4, 4, constants, 0);
		commandList->DrawInstanced(6, 1, 0, 0);
		cursorX += GetGlyphAdvance(ch, scale);
	}

	if (m_nActiveTextField == fieldIndex && m_fCursorBlinkElapsed < 0.5f)
	{
		const float caretX = left + MeasureText(text, m_CursorIndices[fieldIndex], scale) - 3.0f;
		const float caretHeight = frameHeight * 0.62f;
		const float caretWidth = max(2.0f, caretHeight * (7.0f / 112.0f));
		const float caretTop = (rectangle.y + rectangle.w - caretHeight) * 0.5f;
		RenderUiImage(commandList, camera, m_TextCursor,
			XMFLOAT4(caretX, caretTop, caretX + caretWidth, caretTop + caretHeight));
	}
}

void CLoginScene::ResetCursorBlink()
{
	m_fCursorBlinkElapsed = 0.0f;
}

void CLoginScene::MoveCursorFromClick(int fieldIndex, float x, const XMFLOAT4& rectangle)
{
	const std::string& actualText = (fieldIndex == 0) ? m_LoginId : m_LoginPassword;
	const std::string hiddenPasswordText(actualText.size(), '*');
	const std::string& text = (fieldIndex == 1 && !m_bPasswordVisible) ? hiddenPasswordText : actualText;
	const float frameHeight = rectangle.w - rectangle.y;
	const float scale = frameHeight / 330.0f;
	const float textLeft = rectangle.x + frameHeight * 0.30f;
	if (text.empty() || x <= textLeft)
	{
		m_CursorIndices[fieldIndex] = 0;
		return;
	}
	const float textRight = textLeft + MeasureText(text, text.size(), scale);
	if (x >= textRight)
	{
		m_CursorIndices[fieldIndex] = text.size();
		return;
	}
	float boundary = textLeft;
	float nearestDistance = fabsf(x - boundary);
	size_t nearestIndex = 0;
	for (size_t i = 0; i < text.size(); ++i)
	{
		boundary += GetGlyphAdvance(text[i], scale);
		const float distance = fabsf(x - boundary);
		if (distance < nearestDistance)
		{
			nearestDistance = distance;
			nearestIndex = i + 1;
		}
	}
	m_CursorIndices[fieldIndex] = nearestIndex;
}

void CLoginScene::SpawnLoginErrorLog(float viewportWidth, float viewportHeight, int type)
{
	LOGIN_ERROR_LOG log;
	log.rectangle = GetLoginErrorLogRectangle(viewportWidth, viewportHeight, type);
	log.elapsedTime = 0.0f;
	log.type = type;
	m_LoginErrorLogs.push_back(log);
}

void CLoginScene::EnterLoadingPage()
{
	m_eLoginPage = LOGIN_PAGE::LOADING;
	m_nActiveTextField = -1;
	m_LoginErrorLogs.clear();
	m_fLoadingElapsedTime = 0.0f;
}

void CLoginScene::ReturnToInputPageWithLoginFailure(float viewportWidth, float viewportHeight)
{
	m_eLoginPage = LOGIN_PAGE::INPUT;
	m_nActiveTextField = -1;
	m_fLoadingElapsedTime = 0.0f;
	m_LoginErrorLogs.clear();
	SpawnLoginErrorLog(viewportWidth, viewportHeight, 1);
}

void CLoginScene::EnterRegisterPage()
{
	m_eLoginPage = LOGIN_PAGE::REGISTER;
	m_nActiveTextField = -1;
	m_LoginErrorLogs.clear();
	m_fLoadingElapsedTime = 0.0f;
}

void CLoginScene::ReturnToInputPageWithRegisterResult(float viewportWidth, float viewportHeight,
	REGISTER_RESULT result)
{
	m_eLoginPage = LOGIN_PAGE::INPUT;
	m_nActiveTextField = -1;
	m_fLoadingElapsedTime = 0.0f;
	m_LoginErrorLogs.clear();
	SpawnLoginErrorLog(viewportWidth, viewportHeight,
		(result == REGISTER_RESULT::SUCCESS) ? 2 : 3);
}

void CLoginScene::LoadSavedAccountInformation()
{
	std::ifstream input(GetAccountInformationFilePath(), std::ios::binary);
	if (!input) return;

	char magic[8] = {};
	UINT version = 0;
	UINT idLength = 0;
	UINT passwordLength = 0;
	UINT checksum = 0;
	input.read(magic, sizeof(magic));
	input.read(reinterpret_cast<char*>(&version), sizeof(version));
	input.read(reinterpret_cast<char*>(&idLength), sizeof(idLength));
	input.read(reinterpret_cast<char*>(&passwordLength), sizeof(passwordLength));
	input.read(reinterpret_cast<char*>(&checksum), sizeof(checksum));
	const char expectedMagic[8] = { 'R', 'P', 'A', 'C', 'C', 'T', '0', '1' };
	if (!input || memcmp(magic, expectedMagic, sizeof(expectedMagic)) != 0 || version != 1
		|| idLength == 0 || passwordLength == 0 || idLength > 256 || passwordLength > 256)
		return;

	const UINT payloadSize = idLength + passwordLength;
	std::vector<unsigned char> payload(payloadSize);
	input.read(reinterpret_cast<char*>(payload.data()), payload.size());
	if (!input || input.peek() != EOF) return;

	TransformAccountPayload(payload);
	if (CalculateAccountChecksum(payload) != checksum) return;

	std::string loadedId(reinterpret_cast<char*>(payload.data()), idLength);
	std::string loadedPassword(reinterpret_cast<char*>(payload.data() + idLength), passwordLength);
	if (!IsAccountTextValid(loadedId) || !IsAccountTextValid(loadedPassword)) return;

	m_LoginId = loadedId;
	m_LoginPassword = loadedPassword;
	m_CursorIndices[0] = m_LoginId.size();
	m_CursorIndices[1] = m_LoginPassword.size();
}

void CLoginScene::SaveAccountInformation() const
{
	if (!IsAccountTextValid(m_LoginId) || !IsAccountTextValid(m_LoginPassword)) return;
	CreateDirectoryA("Network", NULL);

	std::vector<unsigned char> payload;
	payload.reserve(m_LoginId.size() + m_LoginPassword.size());
	payload.insert(payload.end(), m_LoginId.begin(), m_LoginId.end());
	payload.insert(payload.end(), m_LoginPassword.begin(), m_LoginPassword.end());
	const UINT checksum = CalculateAccountChecksum(payload);
	TransformAccountPayload(payload);

	std::ofstream output(GetAccountInformationFilePath(), std::ios::binary | std::ios::trunc);
	if (!output) return;

	const char magic[8] = { 'R', 'P', 'A', 'C', 'C', 'T', '0', '1' };
	const UINT version = 1;
	const UINT idLength = static_cast<UINT>(m_LoginId.size());
	const UINT passwordLength = static_cast<UINT>(m_LoginPassword.size());
	output.write(magic, sizeof(magic));
	output.write(reinterpret_cast<const char*>(&version), sizeof(version));
	output.write(reinterpret_cast<const char*>(&idLength), sizeof(idLength));
	output.write(reinterpret_cast<const char*>(&passwordLength), sizeof(passwordLength));
	output.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
	output.write(reinterpret_cast<const char*>(payload.data()), payload.size());
}

void CLoginScene::RenderInputPage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	float width, float height)
{
	RenderUiImage(commandList, camera, m_LoginFrame, GetLoginFrameRectangle(width, height));
	RenderUiImage(commandList, camera, m_IdLog, GetLabelRectangle(false, width, height));
	RenderUiImage(commandList, camera, m_PasswordLog, GetLabelRectangle(true, width, height));
	RenderUiImage(commandList, camera, m_TextFrame, GetTextFrameRectangle(false, width, height));
	RenderUiImage(commandList, camera, m_TextFrame, GetTextFrameRectangle(true, width, height));
	RenderTextField(commandList, camera, 0, GetTextFrameRectangle(false, width, height));
	RenderTextField(commandList, camera, 1, GetTextFrameRectangle(true, width, height));
	RenderUiImage(commandList, camera, m_PasswordHideIcon, GetPasswordHideIconRectangle(width, height));
	RenderUiImage(commandList, camera, m_PasswordHideCheckBox,
		GetPasswordHideCheckBoxRectangle(width, height),
		m_bPasswordVisible ? 0x00BFBFBF : 0x00FFFFFF);
	RenderUiImage(commandList, camera, m_RegisterButton, GetRegisterButtonRectangle(width, height));
	RenderUiImage(commandList, camera, m_LoginButton, GetLoginButtonRectangle(false, width, height));
	RenderUiImage(commandList, camera, m_GuestButton, GetLoginButtonRectangle(true, width, height));
	RenderUiImage(commandList, camera, m_CloseIcon, GetCloseRectangle(width, height));
	for (const LOGIN_ERROR_LOG& log : m_LoginErrorLogs)
	{
		const float alphaRatio = max(0.0f, 1.0f - log.elapsedTime);
		const UINT alpha = max(1u, static_cast<UINT>(alphaRatio * 255.0f + 0.5f));
		const float moveY = -20.0f * log.elapsedTime;
		UI_IMAGE_RESOURCE& errorResource = (log.type == 1) ? m_LoginErrorLog2
			: (log.type == 2) ? m_RegisterSuccessLog
			: (log.type == 3) ? m_RegisterFailLog
			: m_LoginErrorLog;
		RenderUiImage(commandList, camera, errorResource,
			XMFLOAT4(log.rectangle.x, log.rectangle.y + moveY,
				log.rectangle.z, log.rectangle.w + moveY),
			(alpha << 24) | 0x00FFFFFF);
	}
}

void CLoginScene::RenderLoadingPage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	float width, float height)
{
	RenderRotatingUiImage(commandList, camera, m_LoginLoading, GetLoginLoadingRectangle(width, height));
	const int textIndex = static_cast<int>(m_fLoadingElapsedTime / 0.333f) % 3;
	RenderUiImage(commandList, camera, m_LoadingTexts[textIndex], GetLoadingTextRectangle(width, height));
	RenderUiImage(commandList, camera, m_DirectStartButton, GetDirectStartButtonRectangle(width, height));
	RenderUiImage(commandList, camera, m_CloseIcon, GetCloseRectangle(width, height));
}

void CLoginScene::RenderRegisterPage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	float width, float height)
{
	RenderRotatingUiImage(commandList, camera, m_LoginLoading, GetLoginLoadingRectangle(width, height));
	const int textIndex = static_cast<int>(m_fLoadingElapsedTime / 0.333f) % 3;
	RenderUiImage(commandList, camera, m_LoadingTexts[textIndex], GetLoadingTextRectangle(width, height));
	RenderUiImage(commandList, camera, m_LoginBackButton, GetDirectStartButtonRectangle(width, height));
	RenderUiImage(commandList, camera, m_CloseIcon, GetCloseRectangle(width, height));
}

void CLoginScene::Render(ID3D12GraphicsCommandList* commandList, CCamera* camera)
{
	if (!camera) return;
	camera->SetViewportsAndScissorRects(commandList);
	camera->UpdateShaderVariables(commandList);
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	m_fLastViewportWidth = width;
	m_fLastViewportHeight = height;
	RenderUiImage(commandList, camera, m_ShopBoard, GetBoardRectangle(width, height));
	if (m_eLoginPage == LOGIN_PAGE::REGISTER)
		RenderRegisterPage(commandList, camera, width, height);
	else if (m_eLoginPage == LOGIN_PAGE::LOADING)
		RenderLoadingPage(commandList, camera, width, height);
	else
		RenderInputPage(commandList, camera, width, height);
}

void CLoginScene::OnProcessingMouseMessage(HWND hWnd, UINT message, WPARAM, LPARAM lParam)
{
	if (message != WM_LBUTTONDOWN) return;
	RECT client;
	GetClientRect(hWnd, &client);
	const float width = static_cast<float>(client.right);
	const float height = static_cast<float>(client.bottom);
	const float x = static_cast<float>(static_cast<short>(LOWORD(lParam)));
	const float y = static_cast<float>(static_cast<short>(HIWORD(lParam)));
	if (PointInRectangle(x, y, GetCloseRectangle(width, height)))
	{
		PostMessage(hWnd, WM_CLOSE, 0, 0);
		return;
	}
	if (m_eLoginPage == LOGIN_PAGE::LOADING)
	{
		if (PointInRectangle(x, y, GetDirectStartButtonRectangle(width, height)))
		{
			g_pFramework->GetNetworkManager().Disconnect();
			g_pFramework->RequestSceneChange(SCENE_TYPE::GAME);
		}
		return;
	}
	if (m_eLoginPage == LOGIN_PAGE::REGISTER)
	{
		if (PointInRectangle(x, y, GetDirectStartButtonRectangle(width, height)))
		{
			g_pFramework->GetNetworkManager().Disconnect();
			m_eLoginPage = LOGIN_PAGE::INPUT;
			m_fLoadingElapsedTime = 0.0f;
		}
		return;
	}
	if (PointInRectangle(x, y, GetPasswordHideCheckBoxRectangle(width, height)))
	{
		m_bPasswordVisible = !m_bPasswordVisible;
		return;
	}
	for (int fieldIndex = 0; fieldIndex < 2; ++fieldIndex)
	{
		const XMFLOAT4 textFrame = GetTextFrameRectangle(fieldIndex == 1, width, height);
		if (!PointInRectangle(x, y, textFrame)) continue;
		m_nActiveTextField = fieldIndex;
		MoveCursorFromClick(fieldIndex, x, textFrame);
		ResetCursorBlink();
		return;
	}
	m_nActiveTextField = -1;
	if (PointInRectangle(x, y, GetRegisterButtonRectangle(width, height)))
	{
		if (m_LoginId.empty() || m_LoginPassword.empty() ||
			!g_pFramework->GetNetworkManager().StartRegister(m_LoginId, m_LoginPassword))
		{
			SpawnLoginErrorLog(width, height);
			return;
		}
		EnterRegisterPage();
	}
	else if (PointInRectangle(x, y, GetLoginButtonRectangle(false, width, height)))
	{
		if (m_LoginId.empty() || m_LoginPassword.empty() ||
			!g_pFramework->GetNetworkManager().StartLogin(m_LoginId, m_LoginPassword))
		{
			SpawnLoginErrorLog(width, height);
			return;
		}
		EnterLoadingPage();
	}
	else if (PointInRectangle(x, y, GetLoginButtonRectangle(true, width, height)))
	{
		g_pFramework->GetNetworkManager().Disconnect();
		g_pFramework->RequestSceneChange(SCENE_TYPE::GAME);
	}
}

void CLoginScene::OnProcessingKeyboardMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM)
{
	if (m_eLoginPage == LOGIN_PAGE::LOADING)
	{
		if (message == WM_KEYDOWN && wParam == VK_F9)
		{
			RECT client;
			GetClientRect(hWnd, &client);
			ReturnToInputPageWithLoginFailure(static_cast<float>(client.right),
				static_cast<float>(client.bottom));
		}
		return;
	}
	if (m_eLoginPage == LOGIN_PAGE::REGISTER)
	{
		if (message == WM_KEYDOWN && (wParam == VK_F8 || wParam == VK_F9))
		{
			RECT client;
			GetClientRect(hWnd, &client);
			ReturnToInputPageWithRegisterResult(static_cast<float>(client.right),
				static_cast<float>(client.bottom),
				(wParam == VK_F8) ? REGISTER_RESULT::SUCCESS : REGISTER_RESULT::FAILURE);
		}
		return;
	}
	if (m_eLoginPage != LOGIN_PAGE::INPUT) return;
	if (m_nActiveTextField < 0) return;
	std::string& text = (m_nActiveTextField == 0) ? m_LoginId : m_LoginPassword;
	size_t& cursorIndex = m_CursorIndices[m_nActiveTextField];
	cursorIndex = min(cursorIndex, text.size());

	if (message == WM_CHAR)
	{
		const char ch = static_cast<char>(wParam);
		if (ch == '\b')
		{
			if (cursorIndex == 0) return;
			text.erase(cursorIndex - 1, 1);
			--cursorIndex;
			ResetCursorBlink();
			return;
		}
		if (!IsLoginInputCharacter(ch) || !FindGlyph(ch)) return;
		RECT client;
		GetClientRect(hWnd, &client);
		const XMFLOAT4 frame = GetTextFrameRectangle(m_nActiveTextField == 1,
			static_cast<float>(client.right), static_cast<float>(client.bottom));
		const float scale = (frame.w - frame.y) / 330.0f;
		std::string candidate = text;
		candidate.insert(candidate.begin() + cursorIndex, ch);
		const float availableWidth = (frame.z - frame.x) - (frame.w - frame.y) * 0.60f;
		const std::string displayCandidate = (m_nActiveTextField == 1 && !m_bPasswordVisible)
			? std::string(candidate.size(), '*') : candidate;
		if (MeasureText(displayCandidate, displayCandidate.size(), scale) > availableWidth) return;
		text.swap(candidate);
		++cursorIndex;
		ResetCursorBlink();
		return;
	}

	if (message != WM_KEYDOWN) return;
	switch (wParam)
	{
	case VK_LEFT:
		if (cursorIndex > 0) --cursorIndex;
		break;
	case VK_RIGHT:
		if (cursorIndex < text.size()) ++cursorIndex;
		break;
	case VK_HOME:
		cursorIndex = 0;
		break;
	case VK_END:
		cursorIndex = text.size();
		break;
	case VK_DELETE:
		if (cursorIndex < text.size()) text.erase(cursorIndex, 1);
		else return;
		break;
	case VK_TAB:
		m_nActiveTextField = 1 - m_nActiveTextField;
		m_CursorIndices[m_nActiveTextField] = (m_nActiveTextField == 0)
			? m_LoginId.size() : m_LoginPassword.size();
		break;
	default:
		return;
	}
	ResetCursorBlink();
}

void CLoginScene::Animate(float elapsedTime)
{
	if (m_eLoginPage == LOGIN_PAGE::LOADING || m_eLoginPage == LOGIN_PAGE::REGISTER)
		m_fLoadingElapsedTime += elapsedTime;

	CLIENT_AUTH_REQUEST request = CLIENT_AUTH_REQUEST::NONE;
	CLIENT_AUTH_RESULT result = CLIENT_AUTH_RESULT::SERVER_ERROR;
	if (g_pFramework->GetNetworkManager().ConsumeAuthResult(request, result))
	{
		if (request == CLIENT_AUTH_REQUEST::LOGIN)
		{
			if (result == CLIENT_AUTH_RESULT::SUCCESS)
			{
				SaveAccountInformation();
				g_pFramework->RequestSceneChange(SCENE_TYPE::GAME);
			}
			else
			{
				g_pFramework->GetNetworkManager().Disconnect();
				ReturnToInputPageWithLoginFailure(
					m_fLastViewportWidth, m_fLastViewportHeight);
			}
		}
		else if (request == CLIENT_AUTH_REQUEST::REGISTER)
		{
			g_pFramework->GetNetworkManager().Disconnect();
			ReturnToInputPageWithRegisterResult(
				m_fLastViewportWidth, m_fLastViewportHeight,
				(result == CLIENT_AUTH_RESULT::SUCCESS)
				? REGISTER_RESULT::SUCCESS : REGISTER_RESULT::FAILURE);
		}
	}

	for (LOGIN_ERROR_LOG& log : m_LoginErrorLogs)
		log.elapsedTime += elapsedTime;
	m_LoginErrorLogs.erase(std::remove_if(m_LoginErrorLogs.begin(), m_LoginErrorLogs.end(),
		[](const LOGIN_ERROR_LOG& log) { return(log.elapsedTime >= 1.0f); }),
		m_LoginErrorLogs.end());

	if (m_nActiveTextField >= 0)
	{
		m_fCursorBlinkElapsed += elapsedTime;
		while (m_fCursorBlinkElapsed >= 1.0f) m_fCursorBlinkElapsed -= 1.0f;
	}
}

CGameObject* CLoginScene::PickObjectPointedByCursor(int x, int y, CCamera* camera)
{
	if (!camera) return(NULL);
	return PointInRectangle(static_cast<float>(x), static_cast<float>(y),
		GetBoardRectangle(camera->m_d3dViewport.Width, camera->m_d3dViewport.Height))
		? &m_UiHitObject : NULL;
}
