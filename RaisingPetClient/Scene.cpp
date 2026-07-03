//-----------------------------------------------------------------------------
// File: Scene.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Scene.h"
#include "GameFramework.h"
#include "DDSTextureLoader12.h"
#include "d3dx12.h"
extern CGameFramework* g_pFramework;

static std::string FormatPossession(UINT nValue)
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
struct MONEY_UI_LAYOUT
{
	float fRightMargin = 30.0f;
	float fBottomMargin = 230.0f;
	float fHorizontalPadding = 14.0f;
	float fVerticalPadding = 8.0f;
	float fOutlineThickness = 2.0f;
	float fGlyphScale = 0.12f;
	float fGlyphGap = 1.0f;
	float fSpaceWidth = 8.0f;
	const char* pWidthReferenceText = "100.0k";
};

static const MONEY_UI_LAYOUT gMoneyUiLayout;
CScene::CScene()
{
}

CScene::~CScene()
{
}

void CScene::BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	
}

ID3D12RootSignature *CScene::CreateGraphicsRootSignature(ID3D12Device *pd3dDevice)
{
	ID3D12RootSignature *pd3dGraphicsRootSignature = NULL;

	D3D12_DESCRIPTOR_RANGE d3dSrvDescriptorRange = {};
	d3dSrvDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	d3dSrvDescriptorRange.NumDescriptors = 1;
	d3dSrvDescriptorRange.BaseShaderRegister = 0;
	d3dSrvDescriptorRange.RegisterSpace = 0;
	d3dSrvDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER pd3dRootParameters[6];
	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[0].Constants.Num32BitValues = 4; //Time, ElapsedTime, xCursor, yCursor
	pd3dRootParameters[0].Constants.ShaderRegister = 0; //Time
	pd3dRootParameters[0].Constants.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[1].Constants.Num32BitValues = 19; //16 + 3
	pd3dRootParameters[1].Constants.ShaderRegister = 1; //World
	pd3dRootParameters[1].Constants.RegisterSpace = 0;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[2].Constants.Num32BitValues = 35; //16 + 16 + 3
	pd3dRootParameters[2].Constants.ShaderRegister = 2; //Camera
	pd3dRootParameters[2].Constants.RegisterSpace = 0;
	pd3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[3].DescriptorTable.pDescriptorRanges = &d3dSrvDescriptorRange;
	pd3dRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[4].Constants.Num32BitValues = 4;
	pd3dRootParameters[4].Constants.ShaderRegister = 3;
	pd3dRootParameters[4].Constants.RegisterSpace = 0;
	pd3dRootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	pd3dRootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[5].Constants.Num32BitValues = 1;
	pd3dRootParameters[5].Constants.ShaderRegister = 4;
	pd3dRootParameters[5].Constants.RegisterSpace = 0;
	pd3dRootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	D3D12_STATIC_SAMPLER_DESC d3dStaticSampler = {};
	d3dStaticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dStaticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dStaticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dStaticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	d3dStaticSampler.MipLODBias = 0.0f;
	d3dStaticSampler.MaxAnisotropy = 1;
	d3dStaticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dStaticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	d3dStaticSampler.MinLOD = 0.0f;
	d3dStaticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	d3dStaticSampler.ShaderRegister = 0;
	d3dStaticSampler.RegisterSpace = 0;
	d3dStaticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	d3dRootSignatureDesc.NumStaticSamplers = 1;
	d3dRootSignatureDesc.pStaticSamplers = &d3dStaticSampler;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	ID3DBlob *pd3dSignatureBlob = NULL;
	ID3DBlob *pd3dErrorBlob = NULL;
	D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void **)&pd3dGraphicsRootSignature);
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	return(pd3dGraphicsRootSignature);
}

void CScene::ReleaseObjects()
{
	
}

void CScene::ReleaseUploadBuffers()
{

}

void CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}
void CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}
CGameObject* CScene::PickObjectPointedByCursor(int xClient, int yClient, CCamera* pCamera)
{
	return(NULL);
}

bool CScene::ProcessInput()
{
	return(false);
}

void CScene::Animate(float fTimeElapsed)
{
}

void CScene::PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList)
{
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
}

void CScene::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
}
void CScene::BuildGraphicsRootSignature(ID3D12Device* pd3dDevice)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
}

//ÅÊÅ© Scene////////////////////////////////////////////////////////////////////////////////////////////////
void CGameScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	struct PET_ASSET_DESC
	{
		const char* pName;
		const char* pMeshFile;
		const wchar_t* pTextureFile;
	};

	const PET_ASSET_DESC petAssets[] =
	{
		{ "TheCA", "Assets/TheCA/TheCAMesh.obj", L"Assets/TheCA/TheCATexture.dds"},
		{ "Touma", "Assets/Touma/ToumaMesh.obj", L"Assets/Touma/ToumaTexture.dds"}
	};

	CPseudoLightingShader* pObjectShader = new CPseudoLightingShader();
	pObjectShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);

	for (const PET_ASSET_DESC& petAsset : petAssets)
	{
		m_vPetResources.emplace_back();
		PET_RENDER_RESOURCE& petResource = m_vPetResources.back();

		CMesh* pPetMesh = new CMesh(pd3dDevice, pd3dCommandList, const_cast<char*>(petAsset.pMeshFile));
		petResource.pPet = new CPet();
		petResource.pPet->SetMesh(pPetMesh);
		petResource.pPet->SetName(petAsset.pName);
		petResource.pPet->SetShader(pObjectShader);
		petResource.pPet->SetPosition(0.0f, -28.0f, 0.0f);
		petResource.pPet->SetPay(10);
		petResource.pPet->GetNowPossession(0);
		petResource.pPet->GetMaxPossession(100);

		std::unique_ptr<uint8_t[]> textureData;
		std::vector<D3D12_SUBRESOURCE_DATA> textureSubresources;
		HRESULT hTextureResult = DirectX::LoadDDSTextureFromFile(pd3dDevice, petAsset.pTextureFile,
			&petResource.pd3dTexture, textureData, textureSubresources);

		if (SUCCEEDED(hTextureResult) && !textureSubresources.empty())
		{
			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(petResource.pd3dTexture, 0,
				static_cast<UINT>(textureSubresources.size()));
			CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
			hTextureResult = pd3dDevice->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
				&uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, __uuidof(ID3D12Resource),
				reinterpret_cast<void**>(&petResource.pd3dTextureUploadBuffer));

			if (SUCCEEDED(hTextureResult))
			{
				UpdateSubresources(pd3dCommandList, petResource.pd3dTexture, petResource.pd3dTextureUploadBuffer,
					0, 0, static_cast<UINT>(textureSubresources.size()), textureSubresources.data());
				CD3DX12_RESOURCE_BARRIER textureBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
					petResource.pd3dTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				pd3dCommandList->ResourceBarrier(1, &textureBarrier);

				D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
				srvHeapDesc.NumDescriptors = 1;
				srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				hTextureResult = pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, __uuidof(ID3D12DescriptorHeap),
					reinterpret_cast<void**>(&petResource.pd3dSrvDescriptorHeap));

				if (SUCCEEDED(hTextureResult))
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
					srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					srvDesc.Format = petResource.pd3dTexture->GetDesc().Format;
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.MipLevels = petResource.pd3dTexture->GetDesc().MipLevels;
					pd3dDevice->CreateShaderResourceView(petResource.pd3dTexture, &srvDesc,
						petResource.pd3dSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
				}
			}
		}
	}

	ID3DBlob* pd3dVertexShaderBlob = NULL;
	ID3DBlob* pd3dPixelShaderBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;

	HRESULT hResult = D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "VSTextGlyph", "vs_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pd3dVertexShaderBlob, &pd3dErrorBlob);
	if (FAILED(hResult))
	{
		if (pd3dErrorBlob) OutputDebugStringA(static_cast<const char*>(pd3dErrorBlob->GetBufferPointer()));
		if (pd3dErrorBlob) pd3dErrorBlob->Release();
		return;
	}
	if (pd3dErrorBlob) { pd3dErrorBlob->Release(); pd3dErrorBlob = NULL; }

	hResult = D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "PSTextGlyph", "ps_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pd3dPixelShaderBlob, &pd3dErrorBlob);
	if (FAILED(hResult))
	{
		if (pd3dErrorBlob) OutputDebugStringA(static_cast<const char*>(pd3dErrorBlob->GetBufferPointer()));
		if (pd3dErrorBlob) pd3dErrorBlob->Release();
		pd3dVertexShaderBlob->Release();
		return;
	}
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineStateDesc = {};
	d3dPipelineStateDesc.pRootSignature = m_pd3dGraphicsRootSignature;
	d3dPipelineStateDesc.VS = { pd3dVertexShaderBlob->GetBufferPointer(), pd3dVertexShaderBlob->GetBufferSize() };
	d3dPipelineStateDesc.PS = { pd3dPixelShaderBlob->GetBufferPointer(), pd3dPixelShaderBlob->GetBufferSize() };
	CShader d3dStateFactory;
	d3dPipelineStateDesc.RasterizerState = d3dStateFactory.CreateRasterizerState();
	d3dPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	d3dPipelineStateDesc.BlendState = d3dStateFactory.CreateBlendState();
	d3dPipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	d3dPipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	d3dPipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	d3dPipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d3dPipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	d3dPipelineStateDesc.DepthStencilState = d3dStateFactory.CreateDepthStencilState();
	d3dPipelineStateDesc.DepthStencilState.DepthEnable = FALSE;
	d3dPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	d3dPipelineStateDesc.SampleMask = UINT_MAX;
	d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	d3dPipelineStateDesc.NumRenderTargets = 1;
	d3dPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dPipelineStateDesc.SampleDesc.Count = 1;

	hResult = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineStateDesc,
		__uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&m_pd3dTextPipelineState));
	pd3dPixelShaderBlob->Release();
	pd3dPixelShaderBlob = NULL;
	if (FAILED(hResult))
	{
		pd3dVertexShaderBlob->Release();
		return;
	}

	hResult = D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "PSSolidUI", "ps_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pd3dPixelShaderBlob, &pd3dErrorBlob);
	if (FAILED(hResult))
	{
		if (pd3dErrorBlob) OutputDebugStringA(static_cast<const char*>(pd3dErrorBlob->GetBufferPointer()));
		if (pd3dErrorBlob) pd3dErrorBlob->Release();
		pd3dVertexShaderBlob->Release();
		return;
	}
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	d3dPipelineStateDesc.PS = { pd3dPixelShaderBlob->GetBufferPointer(), pd3dPixelShaderBlob->GetBufferSize() };
	hResult = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineStateDesc,
		__uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&m_pd3dSolidUiPipelineState));
	pd3dVertexShaderBlob->Release();
	pd3dPixelShaderBlob->Release();
	if (FAILED(hResult)) return;

	struct GLYPH_ASSET_DESC
	{
		char ch;
		const wchar_t* pFileName;
		float fImageWidth;
		float fImageHeight;
		float fLeft;
		float fTop;
		float fRight;
		float fBottom;
	};

	const GLYPH_ASSET_DESC glyphAssets[] =
	{
		{ '0', L"Assets/Image/Numbers/0.dds", 441, 521, 158, 136, 284, 322 },
		{ '1', L"Assets/Image/Numbers/1.dds", 441, 521, 175, 136, 248, 318 },
		{ '2', L"Assets/Image/Numbers/2.dds", 441, 521, 164, 136, 278, 318 },
		{ '3', L"Assets/Image/Numbers/3.dds", 441, 521, 166, 136, 278, 322 },
		{ '4', L"Assets/Image/Numbers/4.dds", 441, 521, 155, 139, 286, 318 },
		{ '5', L"Assets/Image/Numbers/5.dds", 441, 521, 167, 139, 278, 322 },
		{ '6', L"Assets/Image/Numbers/6.dds", 441, 521, 161, 136, 284, 322 },
		{ '7', L"Assets/Image/Numbers/7.dds", 441, 521, 160, 139, 282, 318 },
		{ '8', L"Assets/Image/Numbers/8.dds", 441, 521, 161, 136, 283, 322 },
		{ '9', L"Assets/Image/Numbers/9.dds", 441, 521, 159, 136, 282, 322 },
		{ '.', L"Assets/Image/Numbers/Point.dds", 363, 521, 161, 283, 203, 322 },
		{ '/', L"Assets/Image/Numbers/Slash.dds", 407, 521, 144, 139, 261, 348 },
		{ 'k', L"Assets/Image/Spellings/k.dds", 435, 521, 166, 129, 288, 318 },
		{ 'm', L"Assets/Image/Spellings/m.dds", 527, 521, 167, 187, 363, 319 },
		{ 'b', L"Assets/Image/Spellings/b.dds", 453, 521, 166, 129, 295, 322 }
	};

	for (const GLYPH_ASSET_DESC& glyphAsset : glyphAssets)
	{
		m_vTextGlyphResources.emplace_back();
		TEXT_GLYPH_RESOURCE& glyph = m_vTextGlyphResources.back();
		glyph.ch = glyphAsset.ch;
		glyph.fU0 = glyphAsset.fLeft / glyphAsset.fImageWidth;
		glyph.fV0 = glyphAsset.fTop / glyphAsset.fImageHeight;
		glyph.fU1 = glyphAsset.fRight / glyphAsset.fImageWidth;
		glyph.fV1 = glyphAsset.fBottom / glyphAsset.fImageHeight;
		glyph.fPixelWidth = glyphAsset.fRight - glyphAsset.fLeft;
		glyph.fPixelHeight = glyphAsset.fBottom - glyphAsset.fTop;
		glyph.fTopOffset = glyphAsset.fTop - 129.0f;

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		hResult = DirectX::LoadDDSTextureFromFile(pd3dDevice, glyphAsset.pFileName,
			&glyph.pd3dTexture, ddsData, subresources);
		if (FAILED(hResult) || subresources.empty()) continue;

		const UINT64 nUploadBufferSize = GetRequiredIntermediateSize(glyph.pd3dTexture, 0,
			static_cast<UINT>(subresources.size()));
		CD3DX12_HEAP_PROPERTIES d3dUploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC d3dUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(nUploadBufferSize);
		hResult = pd3dDevice->CreateCommittedResource(&d3dUploadHeapProperties, D3D12_HEAP_FLAG_NONE,
			&d3dUploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
			__uuidof(ID3D12Resource), reinterpret_cast<void**>(&glyph.pd3dTextureUploadBuffer));
		if (FAILED(hResult)) continue;

		UpdateSubresources(pd3dCommandList, glyph.pd3dTexture, glyph.pd3dTextureUploadBuffer,
			0, 0, static_cast<UINT>(subresources.size()), subresources.data());
		CD3DX12_RESOURCE_BARRIER d3dTextureBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			glyph.pd3dTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pd3dCommandList->ResourceBarrier(1, &d3dTextureBarrier);

		D3D12_DESCRIPTOR_HEAP_DESC d3dSrvHeapDesc = {};
		d3dSrvHeapDesc.NumDescriptors = 1;
		d3dSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		d3dSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		hResult = pd3dDevice->CreateDescriptorHeap(&d3dSrvHeapDesc, __uuidof(ID3D12DescriptorHeap),
			reinterpret_cast<void**>(&glyph.pd3dSrvDescriptorHeap));
		if (FAILED(hResult)) continue;

		D3D12_SHADER_RESOURCE_VIEW_DESC d3dSrvDesc = {};
		d3dSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		d3dSrvDesc.Format = glyph.pd3dTexture->GetDesc().Format;
		d3dSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		d3dSrvDesc.Texture2D.MipLevels = glyph.pd3dTexture->GetDesc().MipLevels;
		pd3dDevice->CreateShaderResourceView(glyph.pd3dTexture, &d3dSrvDesc,
			glyph.pd3dSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	}
	pd3dVertexShaderBlob = NULL;
	pd3dPixelShaderBlob = NULL;
	pd3dErrorBlob = NULL;
	hResult = D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "VSCoinSprite", "vs_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pd3dVertexShaderBlob, &pd3dErrorBlob);
	if (FAILED(hResult))
	{
		if (pd3dErrorBlob) OutputDebugStringA(static_cast<const char*>(pd3dErrorBlob->GetBufferPointer()));
		if (pd3dErrorBlob) pd3dErrorBlob->Release();
		return;
	}
	if (pd3dErrorBlob) { pd3dErrorBlob->Release(); pd3dErrorBlob = NULL; }

	hResult = D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "PSCoinSprite", "ps_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pd3dPixelShaderBlob, &pd3dErrorBlob);
	if (FAILED(hResult))
	{
		if (pd3dErrorBlob) OutputDebugStringA(static_cast<const char*>(pd3dErrorBlob->GetBufferPointer()));
		if (pd3dErrorBlob) pd3dErrorBlob->Release();
		pd3dVertexShaderBlob->Release();
		return;
	}
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	d3dPipelineStateDesc.VS = { pd3dVertexShaderBlob->GetBufferPointer(), pd3dVertexShaderBlob->GetBufferSize() };
	d3dPipelineStateDesc.PS = { pd3dPixelShaderBlob->GetBufferPointer(), pd3dPixelShaderBlob->GetBufferSize() };
	hResult = pd3dDevice->CreateGraphicsPipelineState(&d3dPipelineStateDesc,
		__uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&m_pd3dCoinPipelineState));
	pd3dVertexShaderBlob->Release();
	pd3dPixelShaderBlob->Release();
	if (FAILED(hResult)) return;

	std::unique_ptr<uint8_t[]> coinDdsData;
	std::vector<D3D12_SUBRESOURCE_DATA> coinSubresources;
	hResult = DirectX::LoadDDSTextureFromFile(pd3dDevice, L"Assets/Image/Coin.dds",
		&m_pd3dCoinTexture, coinDdsData, coinSubresources);
	if (FAILED(hResult) || coinSubresources.empty()) return;

	const UINT64 nCoinUploadBufferSize = GetRequiredIntermediateSize(m_pd3dCoinTexture, 0,
		static_cast<UINT>(coinSubresources.size()));
	CD3DX12_HEAP_PROPERTIES coinUploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC coinUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(nCoinUploadBufferSize);
	hResult = pd3dDevice->CreateCommittedResource(&coinUploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&coinUploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
		__uuidof(ID3D12Resource), reinterpret_cast<void**>(&m_pd3dCoinTextureUploadBuffer));
	if (FAILED(hResult)) return;

	UpdateSubresources(pd3dCommandList, m_pd3dCoinTexture, m_pd3dCoinTextureUploadBuffer,
		0, 0, static_cast<UINT>(coinSubresources.size()), coinSubresources.data());
	CD3DX12_RESOURCE_BARRIER coinTextureBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pd3dCoinTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pd3dCommandList->ResourceBarrier(1, &coinTextureBarrier);

	D3D12_DESCRIPTOR_HEAP_DESC coinSrvHeapDesc = {};
	coinSrvHeapDesc.NumDescriptors = 1;
	coinSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	coinSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	hResult = pd3dDevice->CreateDescriptorHeap(&coinSrvHeapDesc, __uuidof(ID3D12DescriptorHeap),
		reinterpret_cast<void**>(&m_pd3dCoinSrvDescriptorHeap));
	if (FAILED(hResult)) return;

	D3D12_SHADER_RESOURCE_VIEW_DESC coinSrvDesc = {};
	coinSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	coinSrvDesc.Format = m_pd3dCoinTexture->GetDesc().Format;
	coinSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	coinSrvDesc.Texture2D.MipLevels = m_pd3dCoinTexture->GetDesc().MipLevels;
	pd3dDevice->CreateShaderResourceView(m_pd3dCoinTexture, &coinSrvDesc,
		m_pd3dCoinSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void CGameScene::ReleaseObjects()
{
	for (PET_RENDER_RESOURCE& petResource : m_vPetResources)
	{
		if (petResource.pPet) delete petResource.pPet;
		if (petResource.pd3dTexture) petResource.pd3dTexture->Release();
		if (petResource.pd3dTextureUploadBuffer) petResource.pd3dTextureUploadBuffer->Release();
		if (petResource.pd3dSrvDescriptorHeap) petResource.pd3dSrvDescriptorHeap->Release();
	}
	m_vPetResources.clear();

	if (m_pd3dTextPipelineState) m_pd3dTextPipelineState->Release();
	if (m_pd3dSolidUiPipelineState) m_pd3dSolidUiPipelineState->Release();
	if (m_pd3dCoinPipelineState) m_pd3dCoinPipelineState->Release();
	if (m_pd3dCoinTexture) m_pd3dCoinTexture->Release();
	if (m_pd3dCoinTextureUploadBuffer) m_pd3dCoinTextureUploadBuffer->Release();
	if (m_pd3dCoinSrvDescriptorHeap) m_pd3dCoinSrvDescriptorHeap->Release();
	m_vCoinEffects.clear();
	for (TEXT_GLYPH_RESOURCE& glyph : m_vTextGlyphResources)
	{
		if (glyph.pd3dTexture) glyph.pd3dTexture->Release();
		if (glyph.pd3dTextureUploadBuffer) glyph.pd3dTextureUploadBuffer->Release();
		if (glyph.pd3dSrvDescriptorHeap) glyph.pd3dSrvDescriptorHeap->Release();
	}
	m_vTextGlyphResources.clear();
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();
}
void CGameScene::ReleaseUploadBuffers()
{
	for (PET_RENDER_RESOURCE& petResource : m_vPetResources)
	{
		if (petResource.pPet) petResource.pPet->ReleaseUploadBuffers();
		if (petResource.pd3dTextureUploadBuffer)
		{
			petResource.pd3dTextureUploadBuffer->Release();
			petResource.pd3dTextureUploadBuffer = NULL;
		}
	}

	for (TEXT_GLYPH_RESOURCE& glyph : m_vTextGlyphResources)
	{
		if (glyph.pd3dTextureUploadBuffer)
		{
			glyph.pd3dTextureUploadBuffer->Release();
			glyph.pd3dTextureUploadBuffer = NULL;
		}
	}
	if (m_pd3dCoinTextureUploadBuffer)
	{
		m_pd3dCoinTextureUploadBuffer->Release();
		m_pd3dCoinTextureUploadBuffer = NULL;
	}
}
void CGameScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (!pCamera) return;

	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);
	if (m_nActivePetIndex < m_vPetResources.size())
	{
		PET_RENDER_RESOURCE& petResource = m_vPetResources[m_nActivePetIndex];
		if (petResource.pPet && petResource.pd3dSrvDescriptorHeap)
		{
			ID3D12DescriptorHeap* ppd3dPetDescriptorHeaps[] = { petResource.pd3dSrvDescriptorHeap };
			pd3dCommandList->SetDescriptorHeaps(1, ppd3dPetDescriptorHeaps);
			pd3dCommandList->SetGraphicsRootDescriptorTable(3,
				petResource.pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			petResource.pPet->Render(pd3dCommandList, pCamera);
		}
	}

	if (m_nActivePetIndex < m_vPetResources.size())
		RenderPetPossessionText(pd3dCommandList, pCamera, m_vPetResources[m_nActivePetIndex].pPet);

	RenderCoinEffects(pd3dCommandList, pCamera);
	RenderMoneyUI(pd3dCommandList, pCamera);
}
void CGameScene::RenderPetPossessionText(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, CPet* pPet)
{
	if (!m_pd3dTextPipelineState || !pCamera || !pPet || m_vTextGlyphResources.empty()) return;

	const std::string strText = FormatPossession(pPet->GetNowPossession()) + " / "
		+ FormatPossession(pPet->GetMaxPossession());
	const float fGlyphScale = 0.12f;
	const float fGlyphGap = 1.0f;
	const float fSpaceWidth = 8.0f;

	auto FindGlyph = [this](char ch) -> TEXT_GLYPH_RESOURCE*
	{
		for (TEXT_GLYPH_RESOURCE& glyph : m_vTextGlyphResources)
			if (glyph.ch == ch) return(&glyph);
		return(NULL);
	};

	float fTextWidth = 0.0f;
	for (char ch : strText)
	{
		if (ch == ' ')
			fTextWidth += fSpaceWidth;
		else
		{
			TEXT_GLYPH_RESOURCE* pGlyph = FindGlyph(ch);
			if (pGlyph) fTextWidth += (pGlyph->fPixelWidth * fGlyphScale) + fGlyphGap;
		}
	}
	if (fTextWidth > 0.0f) fTextWidth -= fGlyphGap;

	const BoundingOrientedBox& boundingBox = pPet->m_xmOOBB;
	XMFLOAT3 xmf3Anchor(boundingBox.Center.x,
		boundingBox.Center.y + boundingBox.Extents.y + 1.0f,
		boundingBox.Center.z);
	XMMATRIX xmmtxViewProjection = XMLoadFloat4x4(&pCamera->m_xmf4x4View)
		* XMLoadFloat4x4(&pCamera->m_xmf4x4Projection);
	XMVECTOR xmvAnchor = XMVector3TransformCoord(XMLoadFloat3(&xmf3Anchor), xmmtxViewProjection);
	XMFLOAT3 xmf3Ndc;
	XMStoreFloat3(&xmf3Ndc, xmvAnchor);
	if (xmf3Ndc.z < 0.0f || xmf3Ndc.z > 1.0f) return;

	const float fViewportWidth = pCamera->m_d3dViewport.Width;
	const float fViewportHeight = pCamera->m_d3dViewport.Height;
	if (fViewportWidth <= 0.0f || fViewportHeight <= 0.0f) return;

	const float fAnchorX = (xmf3Ndc.x * 0.5f + 0.5f) * fViewportWidth;
	const float fAnchorY = (0.5f - xmf3Ndc.y * 0.5f) * fViewportHeight;
	float fCursorX = fAnchorX - (fTextWidth * 0.5f);
	const float fTextTop = fAnchorY - 36.0f;

	pd3dCommandList->SetPipelineState(m_pd3dTextPipelineState);
	pd3dCommandList->SetGraphicsRoot32BitConstant(5, 0x00FFC000, 0);
	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (char ch : strText)
	{
		if (ch == ' ')
		{
			fCursorX += fSpaceWidth;
			continue;
		}

		TEXT_GLYPH_RESOURCE* pGlyph = FindGlyph(ch);
		if (!pGlyph || !pGlyph->pd3dSrvDescriptorHeap) continue;

		const float fImageWidth = pGlyph->fPixelWidth / (pGlyph->fU1 - pGlyph->fU0);
		const float fImageHeight = pGlyph->fPixelHeight / (pGlyph->fV1 - pGlyph->fV0);
		const float fCropLeft = pGlyph->fU0 * fImageWidth;
		const float fCropTop = pGlyph->fV0 * fImageHeight;
		const float fGlyphLeft = fCursorX;
		const float fLeft = fGlyphLeft - (fCropLeft * fGlyphScale);
		const float fTop = fTextTop + (pGlyph->fTopOffset * fGlyphScale) - (fCropTop * fGlyphScale);
		const float fRight = fLeft + (fImageWidth * fGlyphScale);
		const float fBottom = fTop + (fImageHeight * fGlyphScale);
		const float textConstants[4] =
		{
			(fLeft / fViewportWidth) * 2.0f - 1.0f,
			1.0f - (fTop / fViewportHeight) * 2.0f,
			(fRight / fViewportWidth) * 2.0f - 1.0f,
			1.0f - (fBottom / fViewportHeight) * 2.0f
		};

		ID3D12DescriptorHeap* ppd3dDescriptorHeaps[] = { pGlyph->pd3dSrvDescriptorHeap };
		pd3dCommandList->SetDescriptorHeaps(1, ppd3dDescriptorHeaps);
		pd3dCommandList->SetGraphicsRootDescriptorTable(3,
			pGlyph->pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		pd3dCommandList->SetGraphicsRoot32BitConstants(4, 4, textConstants, 0);
		pd3dCommandList->DrawInstanced(6, 1, 0, 0);

		fCursorX = fGlyphLeft + (pGlyph->fPixelWidth * fGlyphScale) + fGlyphGap;
	}
}
void CGameScene::RenderSolidUiRectangle(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
	float fLeft, float fTop, float fRight, float fBottom, UINT nColor)
{
	if (!m_pd3dSolidUiPipelineState || !pCamera) return;
	const float fViewportWidth = pCamera->m_d3dViewport.Width;
	const float fViewportHeight = pCamera->m_d3dViewport.Height;
	if (fViewportWidth <= 0.0f || fViewportHeight <= 0.0f) return;

	const float rectangleConstants[4] =
	{
		(fLeft / fViewportWidth) * 2.0f - 1.0f,
		1.0f - (fTop / fViewportHeight) * 2.0f,
		(fRight / fViewportWidth) * 2.0f - 1.0f,
		1.0f - (fBottom / fViewportHeight) * 2.0f
	};

	pd3dCommandList->SetPipelineState(m_pd3dSolidUiPipelineState);
	pd3dCommandList->SetGraphicsRoot32BitConstants(4, 4, rectangleConstants, 0);
	pd3dCommandList->SetGraphicsRoot32BitConstant(5, nColor, 0);
	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pd3dCommandList->DrawInstanced(6, 1, 0, 0);
}

void CGameScene::RenderMoneyUI(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (!m_pd3dTextPipelineState || !pCamera || m_vTextGlyphResources.empty()) return;
	const std::string strText = FormatPossession(m_nMoney);

	auto FindGlyph = [this](char ch) -> TEXT_GLYPH_RESOURCE*
	{
		for (TEXT_GLYPH_RESOURCE& glyph : m_vTextGlyphResources)
			if (glyph.ch == ch) return(&glyph);
		return(NULL);
	};

	float fTextWidth = 0.0f;
	float fMinimumTop = FLT_MAX;
	float fMaximumBottom = -FLT_MAX;
	for (char ch : strText)
	{
		if (ch == ' ')
		{
			fTextWidth += gMoneyUiLayout.fSpaceWidth;
			continue;
		}

		TEXT_GLYPH_RESOURCE* pGlyph = FindGlyph(ch);
		if (!pGlyph) continue;
		fTextWidth += (pGlyph->fPixelWidth * gMoneyUiLayout.fGlyphScale) + gMoneyUiLayout.fGlyphGap;
		fMinimumTop = min(fMinimumTop, pGlyph->fTopOffset * gMoneyUiLayout.fGlyphScale);
		fMaximumBottom = max(fMaximumBottom,
			(pGlyph->fTopOffset + pGlyph->fPixelHeight) * gMoneyUiLayout.fGlyphScale);
	}
	if (fTextWidth > 0.0f) fTextWidth -= gMoneyUiLayout.fGlyphGap;
	if (fMinimumTop == FLT_MAX) return;

	float fFixedTextWidth = 0.0f;
	for (const char* pCharacter = gMoneyUiLayout.pWidthReferenceText; *pCharacter; ++pCharacter)
	{
		TEXT_GLYPH_RESOURCE* pGlyph = FindGlyph(*pCharacter);
		if (pGlyph) fFixedTextWidth += (pGlyph->fPixelWidth * gMoneyUiLayout.fGlyphScale)
			+ gMoneyUiLayout.fGlyphGap;
	}
	if (fFixedTextWidth > 0.0f) fFixedTextWidth -= gMoneyUiLayout.fGlyphGap;

	const float fViewportWidth = pCamera->m_d3dViewport.Width;
	const float fViewportHeight = pCamera->m_d3dViewport.Height;
	const float fPanelWidth = fFixedTextWidth + (gMoneyUiLayout.fHorizontalPadding * 2.0f);
	const float fPanelHeight = (fMaximumBottom - fMinimumTop) + (gMoneyUiLayout.fVerticalPadding * 2.0f);
	const float fPanelRight = fViewportWidth - gMoneyUiLayout.fRightMargin;
	const float fPanelBottom = fViewportHeight - gMoneyUiLayout.fBottomMargin;
	const float fPanelLeft = fPanelRight - fPanelWidth;
	const float fPanelTop = fPanelBottom - fPanelHeight;
	const float fOutline = gMoneyUiLayout.fOutlineThickness;

	RenderSolidUiRectangle(pd3dCommandList, pCamera,
		fPanelLeft - fOutline, fPanelTop - fOutline,
		fPanelRight + fOutline, fPanelBottom + fOutline, 0x00000000);
	RenderSolidUiRectangle(pd3dCommandList, pCamera,
		fPanelLeft, fPanelTop, fPanelRight, fPanelBottom, 0x00FFFFFF);

	float fCursorX = fPanelRight - gMoneyUiLayout.fHorizontalPadding - fTextWidth;
	const float fTextTop = fPanelTop + gMoneyUiLayout.fVerticalPadding - fMinimumTop;
	pd3dCommandList->SetPipelineState(m_pd3dTextPipelineState);
	pd3dCommandList->SetGraphicsRoot32BitConstant(5, 0x00000000, 0);
	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (char ch : strText)
	{
		if (ch == ' ')
		{
			fCursorX += gMoneyUiLayout.fSpaceWidth;
			continue;
		}

		TEXT_GLYPH_RESOURCE* pGlyph = FindGlyph(ch);
		if (!pGlyph || !pGlyph->pd3dSrvDescriptorHeap) continue;

		const float fImageWidth = pGlyph->fPixelWidth / (pGlyph->fU1 - pGlyph->fU0);
		const float fImageHeight = pGlyph->fPixelHeight / (pGlyph->fV1 - pGlyph->fV0);
		const float fCropLeft = pGlyph->fU0 * fImageWidth;
		const float fCropTop = pGlyph->fV0 * fImageHeight;
		const float fGlyphLeft = fCursorX;
		const float fLeft = fGlyphLeft - (fCropLeft * gMoneyUiLayout.fGlyphScale);
		const float fTop = fTextTop + (pGlyph->fTopOffset * gMoneyUiLayout.fGlyphScale)
			- (fCropTop * gMoneyUiLayout.fGlyphScale);
		const float fRight = fLeft + (fImageWidth * gMoneyUiLayout.fGlyphScale);
		const float fBottom = fTop + (fImageHeight * gMoneyUiLayout.fGlyphScale);
		const float textConstants[4] =
		{
			(fLeft / fViewportWidth) * 2.0f - 1.0f,
			1.0f - (fTop / fViewportHeight) * 2.0f,
			(fRight / fViewportWidth) * 2.0f - 1.0f,
			1.0f - (fBottom / fViewportHeight) * 2.0f
		};

		ID3D12DescriptorHeap* ppd3dDescriptorHeaps[] = { pGlyph->pd3dSrvDescriptorHeap };
		pd3dCommandList->SetDescriptorHeaps(1, ppd3dDescriptorHeaps);
		pd3dCommandList->SetGraphicsRootDescriptorTable(3,
			pGlyph->pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		pd3dCommandList->SetGraphicsRoot32BitConstants(4, 4, textConstants, 0);
		pd3dCommandList->DrawInstanced(6, 1, 0, 0);

		fCursorX = fGlyphLeft + (pGlyph->fPixelWidth * gMoneyUiLayout.fGlyphScale)
			+ gMoneyUiLayout.fGlyphGap;
	}
}
void CGameScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	extern CGameFramework* g_pFramework;

	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_SPACE:
			break;
		case VK_ESCAPE:
			::PostQuitMessage(0);
			break;
		default:
			break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_SPACE:
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}
CGameObject* CGameScene::PickObjectPointedByCursor(int xClient, int yClient, CCamera* pCamera)
{
	m_pPointedPet = NULL;
	if (!pCamera || pCamera->m_d3dViewport.Width <= 0.0f || pCamera->m_d3dViewport.Height <= 0.0f)
		return(NULL);

	XMFLOAT3 xmf3PickPosition;
	xmf3PickPosition.x = (((2.0f * xClient) / pCamera->m_d3dViewport.Width) - 1.0f) / pCamera->m_xmf4x4Projection._11;
	xmf3PickPosition.y = -(((2.0f * yClient) / pCamera->m_d3dViewport.Height) - 1.0f) / pCamera->m_xmf4x4Projection._22;
	xmf3PickPosition.z = 1.0f;

	XMVECTOR xmvPickPosition = XMLoadFloat3(&xmf3PickPosition);
	XMMATRIX xmmtxView = XMLoadFloat4x4(&pCamera->m_xmf4x4View);

	if (m_nActivePetIndex >= m_vPetResources.size()) return(NULL);

	CPet* pActivePet = m_vPetResources[m_nActivePetIndex].pPet;
	if (!pActivePet) return(NULL);

	float fHitDistance = FLT_MAX;
	if (pActivePet->PickObjectByRayIntersection(xmvPickPosition, xmmtxView, &fHitDistance) <= 0)
		return(NULL);

	m_pPointedPet = pActivePet;
	return(m_pPointedPet);
}
void CGameScene::SpawnCoinEffects(CPet* pPet, UINT nPossessionBeforeCollection)
{
	if (!pPet || nPossessionBeforeCollection == 0) return;
	const UINT nMaxPossession = pPet->GetMaxPossession();
	if (nMaxPossession == 0) return;

	UINT nCoinCount = static_cast<UINT>((static_cast<UINT64>(nPossessionBeforeCollection) * 10
		+ nMaxPossession - 1) / nMaxPossession);
	if (nCoinCount > 10) nCoinCount = 10;

	std::uniform_real_distribution<float> angleDistribution(0.0f, XM_PI);
	std::uniform_real_distribution<float> speedDistribution(9.0f, 11.0f);
	std::uniform_real_distribution<float> lifetimeDistribution(1.0f, 3.0f);
	const XMFLOAT3 xmf3Origin = pPet->m_xmOOBB.Center;

	for (UINT i = 0; i < nCoinCount; ++i)
	{
		const float fAngle = angleDistribution(m_CoinRandomEngine);
		const float fSpeed = speedDistribution(m_CoinRandomEngine);
		COIN_EFFECT coinEffect;
		coinEffect.xmf3Position = xmf3Origin;
		coinEffect.xmf3Velocity = XMFLOAT3(cosf(fAngle) * fSpeed, sinf(fAngle) * fSpeed, 0.0f);
		coinEffect.fLifetime = lifetimeDistribution(m_CoinRandomEngine);
		m_vCoinEffects.push_back(coinEffect);
	}
}

void CGameScene::CollectPetPossession(CPet* pPet)
{
	if (!pPet) return;
	const UINT nPossessionBeforeCollection = pPet->GetNowPossession();
	std::string strDebugMessage = "[Before Collection] Money: " + std::to_string(m_nMoney)
		+ ", Pet Possession: " + std::to_string(nPossessionBeforeCollection) + "\n";
	OutputDebugStringA(strDebugMessage.c_str());

	SpawnCoinEffects(pPet, nPossessionBeforeCollection);
	m_nMoney += nPossessionBeforeCollection;
	pPet->GetNowPossession(0);

	strDebugMessage = "[After Collection] Money: " + std::to_string(m_nMoney)
		+ ", Pet Possession: " + std::to_string(pPet->GetNowPossession()) + "\n";
	OutputDebugStringA(strDebugMessage.c_str());
}

void CGameScene::AnimateCoinEffects(float fElapsedTime)
{
	if (fElapsedTime <= 0.0f) return;
	const float fGravity = -24.0f;

	for (size_t i = 0; i < m_vCoinEffects.size();)
	{
		COIN_EFFECT& coinEffect = m_vCoinEffects[i];
		coinEffect.fAge += fElapsedTime;
		if (coinEffect.fAge >= coinEffect.fLifetime)
		{
			m_vCoinEffects.erase(m_vCoinEffects.begin() + i);
			continue;
		}

		coinEffect.xmf3Velocity.y += fGravity * fElapsedTime;
		coinEffect.xmf3Position.x += coinEffect.xmf3Velocity.x * fElapsedTime;
		coinEffect.xmf3Position.y += coinEffect.xmf3Velocity.y * fElapsedTime;
		coinEffect.xmf3Position.z += coinEffect.xmf3Velocity.z * fElapsedTime;
		++i;
	}
}

void CGameScene::RenderCoinEffects(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (!m_pd3dCoinPipelineState || !m_pd3dCoinSrvDescriptorHeap || !pCamera || m_vCoinEffects.empty()) return;
	const float fViewportWidth = pCamera->m_d3dViewport.Width;
	const float fViewportHeight = pCamera->m_d3dViewport.Height;
	if (fViewportWidth <= 0.0f || fViewportHeight <= 0.0f) return;

	XMMATRIX xmmtxViewProjection = XMLoadFloat4x4(&pCamera->m_xmf4x4View)
		* XMLoadFloat4x4(&pCamera->m_xmf4x4Projection);
	ID3D12DescriptorHeap* ppd3dDescriptorHeaps[] = { m_pd3dCoinSrvDescriptorHeap };
	pd3dCommandList->SetDescriptorHeaps(1, ppd3dDescriptorHeaps);
	pd3dCommandList->SetGraphicsRootDescriptorTable(3,
		m_pd3dCoinSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	pd3dCommandList->SetPipelineState(m_pd3dCoinPipelineState);
	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const float fCoinWidth = 28.5f;
	const float fCoinHeight = 33.0f;
	for (const COIN_EFFECT& coinEffect : m_vCoinEffects)
	{
		XMVECTOR xmvPosition = XMVector3TransformCoord(XMLoadFloat3(&coinEffect.xmf3Position), xmmtxViewProjection);
		XMFLOAT3 xmf3Ndc;
		XMStoreFloat3(&xmf3Ndc, xmvPosition);
		if (xmf3Ndc.z < 0.0f || xmf3Ndc.z > 1.0f) continue;

		const float fCenterX = (xmf3Ndc.x * 0.5f + 0.5f) * fViewportWidth;
		const float fCenterY = (0.5f - xmf3Ndc.y * 0.5f) * fViewportHeight;
		const float fLeft = fCenterX - (fCoinWidth * 0.5f);
		const float fTop = fCenterY - (fCoinHeight * 0.5f);
		const float fRight = fLeft + fCoinWidth;
		const float fBottom = fTop + fCoinHeight;
		const float spriteConstants[4] =
		{
			(fLeft / fViewportWidth) * 2.0f - 1.0f,
			1.0f - (fTop / fViewportHeight) * 2.0f,
			(fRight / fViewportWidth) * 2.0f - 1.0f,
			1.0f - (fBottom / fViewportHeight) * 2.0f
		};
		const UINT nFrame = static_cast<UINT>(coinEffect.fAge / 0.1f) % 4;
		pd3dCommandList->SetGraphicsRoot32BitConstants(4, 4, spriteConstants, 0);
		pd3dCommandList->SetGraphicsRoot32BitConstant(5, nFrame, 0);
		pd3dCommandList->DrawInstanced(6, 1, 0, 0);
	}
}
void CGameScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
		if (m_pPointedPet)
			CollectPetPossession(m_pPointedPet);
		break;
	case WM_RBUTTONDOWN:
		break;
	}
}

void CGameScene::Animate(float fElapsedTime)
{
	if (m_nActivePetIndex < m_vPetResources.size())
	{
		CPet* pActivePet = m_vPetResources[m_nActivePetIndex].pPet;
		if (pActivePet) pActivePet->Animate(fElapsedTime);
	}
	AnimateCoinEffects(fElapsedTime);
}

bool CGameScene::DiscountMoney(UINT p)
{
	if (p > m_nMoney) return false;

	m_nMoney -= p;
	return true;
}