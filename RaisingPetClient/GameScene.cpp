//-----------------------------------------------------------------------------
// File: GameScene.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Scene.h"
#include "GameScene.h"
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
		{ "Touma", "Assets/Touma/ToumaMesh.obj", L"Assets/Touma/ToumaTexture.dds"},
		{ "Janmang", "Assets/Janmang/JanmangMesh.obj", L"Assets/Janmang/JanmangTexture.dds"},
		{ "Poideu", "Assets/Poideu/PoideuMesh.obj", L"Assets/Poideu/PoideuTexture.dds"},
		{ "Rune", "Assets/Rune/RuneMesh.obj", L"Assets/Rune/RuneTexture.dds"},
		{ "Bbangbugi", "Assets/Bbangbugi/BbangbugiMesh.obj", L"Assets/Bbangbugi/BbangbugiTexture.dds" }
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
		{ 'b', L"Assets/Image/Spellings/b.dds", 453, 521, 166, 129, 295, 322 },
		{ 'a', L"Assets/Image/Spellings/a.dds", 433, 521, 157, 187, 269, 322 },
		{ 'A', L"Assets/Image/Spellings/a2.dds", 472, 521, 150, 139, 321, 318 },
		{ 'B', L"Assets/Image/Spellings/b2.dds", 455, 521, 170, 139, 297, 318 },
		{ 'c', L"Assets/Image/Spellings/c.dds", 419, 521, 159, 187, 261, 322 },
		{ 'C', L"Assets/Image/Spellings/c2.dds", 456, 521, 158, 136, 296, 322 },
		{ 'd', L"Assets/Image/Spellings/d.dds", 453, 521, 159, 129, 287, 322 },
		{ 'D', L"Assets/Image/Spellings/d2.dds", 483, 521, 170, 139, 325, 318 },
		{ 'e', L"Assets/Image/Spellings/e.dds", 435, 521, 159, 187, 278, 322 },
		{ 'E', L"Assets/Image/Spellings/e2.dds", 431, 521, 170, 139, 274, 318 },
		{ 'f', L"Assets/Image/Spellings/f.dds", 390, 521, 154, 126, 244, 318 },
		{ 'F', L"Assets/Image/Spellings/f2.dds", 428, 521, 169, 139, 270, 318 },
		{ 'g', L"Assets/Image/Spellings/g.dds", 453, 521, 159, 187, 287, 378 },
		{ 'G', L"Assets/Image/Spellings/g2.dds", 477, 521, 159, 136, 313, 322 },
		{ 'h', L"Assets/Image/Spellings/h.dds", 448, 521, 166, 129, 284, 318 },
		{ 'H', L"Assets/Image/Spellings/h2.dds", 488, 521, 169, 139, 318, 318 },
		{ 'i', L"Assets/Image/Spellings/i.dds", 366, 521, 163, 130, 203, 319 },
		{ 'I', L"Assets/Image/Spellings/i2.dds", 375, 521, 170, 139, 205, 318 },
		{ 'j', L"Assets/Image/Spellings/j.dds", 367, 521, 130, 130, 204, 378 },
		{ 'J', L"Assets/Image/Spellings/j2.dds", 405, 521, 153, 139, 237, 322 },
		{ 'K', L"Assets/Image/Spellings/k2.dds", 458, 521, 169, 139, 311, 318 },
		{ 'l', L"Assets/Image/Spellings/l.dds", 366, 521, 165, 129, 200, 318 },
		{ 'L', L"Assets/Image/Spellings/l2.dds", 425, 521, 170, 139, 273, 318 },
		{ 'M', L"Assets/Image/Spellings/m2.dds", 537, 521, 170, 139, 368, 318 },
		{ 'n', L"Assets/Image/Spellings/n.dds", 449, 521, 167, 187, 285, 319 },
		{ 'N', L"Assets/Image/Spellings/n2.dds", 495, 521, 170, 139, 326, 318 },
		{ 'o', L"Assets/Image/Spellings/o.dds", 451, 521, 158, 187, 293, 322 },
		{ 'O', L"Assets/Image/Spellings/o2.dds", 490, 521, 158, 136, 332, 322 },
		{ 'p', L"Assets/Image/Spellings/p.dds", 453, 521, 166, 187, 295, 377 },
		{ 'P', L"Assets/Image/Spellings/p2.dds", 451, 521, 170, 139, 295, 318 },
		{ 'q', L"Assets/Image/Spellings/q.dds", 453, 521, 159, 187, 287, 377 },
		{ 'Q', L"Assets/Image/Spellings/q2.dds", 490, 521, 158, 136, 349, 342 },
		{ 'r', L"Assets/Image/Spellings/r.dds", 395, 521, 166, 188, 245, 319 },
		{ 'R', L"Assets/Image/Spellings/r2.dds", 459, 521, 170, 139, 313, 318 },
		{ 's', L"Assets/Image/Spellings/s.dds", 413, 521, 162, 187, 257, 322 },
		{ 'S', L"Assets/Image/Spellings/s2.dds", 438, 521, 161, 136, 280, 322 },
		{ 't', L"Assets/Image/Spellings/t.dds", 393, 521, 154, 152, 239, 321 },
		{ 'T', L"Assets/Image/Spellings/t2.dds", 443, 521, 153, 139, 291, 318 },
		{ 'u', L"Assets/Image/Spellings/u.dds", 449, 521, 164, 190, 283, 322 },
		{ 'U', L"Assets/Image/Spellings/u2.dds", 479, 521, 168, 139, 312, 322 },
		{ 'v', L"Assets/Image/Spellings/v.dds", 431, 521, 150, 190, 282, 319 },
		{ 'V', L"Assets/Image/Spellings/v2.dds", 464, 521, 150, 139, 314, 318 },
		{ 'w', L"Assets/Image/Spellings/w.dds", 495, 521, 151, 190, 344, 319 },
		{ 'W', L"Assets/Image/Spellings/w2.dds", 548, 521, 151, 139, 397, 318 },
		{ 'x', L"Assets/Image/Spellings/x.dds", 431, 521, 150, 190, 281, 319 },
		{ 'X', L"Assets/Image/Spellings/x2.dds", 459, 521, 150, 139, 309, 318 },
		{ 'y', L"Assets/Image/Spellings/y.dds", 431, 521, 150, 190, 283, 378 },
		{ 'Y', L"Assets/Image/Spellings/y2.dds", 448, 521, 149, 139, 299, 318 },
		{ 'z', L"Assets/Image/Spellings/z.dds", 418, 521, 152, 190, 264, 319 },
		{ 'Z', L"Assets/Image/Spellings/z2.dds", 449, 521, 153, 139, 295, 318 },
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
	m_ShopUI.BuildObjects(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, m_vPetResources.size());
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
	m_ShopUI.ReleaseObjects();
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
	m_ShopUI.ReleaseUploadBuffers();
}
SHOP_TEXT_RENDER_CONTEXT CGameScene::GetShopTextRenderContext()
{
	SHOP_TEXT_RENDER_CONTEXT context;
	context.pTextPipelineState = m_pd3dTextPipelineState;
	context.pSolidUiPipelineState = m_pd3dSolidUiPipelineState;
	context.pGlyphResources = &m_vTextGlyphResources;
	return(context);
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
	std::vector<SHOP_PET_RENDER_RESOURCE> shopPets;
	shopPets.reserve(m_vPetResources.size());
	for (PET_RENDER_RESOURCE& petResource : m_vPetResources)
		shopPets.push_back({ petResource.pPet, petResource.pd3dSrvDescriptorHeap });
	m_ShopUI.Render(pd3dCommandList, pCamera, m_nMoney, shopPets, GetShopTextRenderContext());
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
	if (m_ShopUI.IsPointOver(static_cast<float>(xClient), static_cast<float>(yClient),
		pCamera->m_d3dViewport.Width, pCamera->m_d3dViewport.Height))
		return(&m_ShopUiHitObject);

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
	if (m_ShopUI.OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam,
		m_nMoney, m_vPetResources.size(), m_nActivePetIndex, GetShopTextRenderContext()))
		return;

	if (nMessageID == WM_LBUTTONDOWN && m_pPointedPet)
		CollectPetPossession(m_pPointedPet);
}
void CGameScene::Animate(float fElapsedTime)
{
	if (m_nActivePetIndex < m_vPetResources.size())
	{
		CPet* pActivePet = m_vPetResources[m_nActivePetIndex].pPet;
		if (pActivePet)
		{
			pActivePet->Animate(fElapsedTime);
			if (pActivePet->ConsumeAutoCollectRequest())
				CollectPetPossession(pActivePet);
		}
	}
	AnimateCoinEffects(fElapsedTime);
	m_ShopUI.Animate(fElapsedTime);
}

bool CGameScene::DiscountMoney(UINT p)
{
	if (p > m_nMoney) return false;

	m_nMoney -= p;
	return true;
}
