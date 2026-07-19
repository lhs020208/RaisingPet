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

namespace
{
const char* GetLocalPlayerStatusFilePath()
{
	return("Network\\LocalPlayerStatus.rplps");
}

UINT CalculateLocalPlayerStatusChecksum(const std::vector<unsigned char>& data)
{
	UINT checksum = 2166136261u;
	for (unsigned char byte : data)
	{
		checksum ^= byte;
		checksum *= 16777619u;
	}
	return(checksum);
}

void TransformLocalPlayerStatusPayload(std::vector<unsigned char>& data)
{
	const unsigned char key[] = { 0x52, 0x50, 0x4C, 0x6F, 0x63, 0x61, 0x6C, 0x53, 0x74, 0x61, 0x74 };
	unsigned char rolling = 0xC3;
	for (size_t i = 0; i < data.size(); ++i)
	{
		const unsigned char mixedKey = static_cast<unsigned char>(key[i % _countof(key)]
			+ static_cast<unsigned char>(i * 29) + rolling);
		data[i] ^= mixedKey;
		rolling = static_cast<unsigned char>(rolling * 37u + 23u);
	}
}

std::string WideStringToUtf8(const std::wstring& text)
{
	if (text.empty()) return(std::string());
	const int requiredBytes = WideCharToMultiByte(CP_UTF8, 0, text.c_str(),
		static_cast<int>(text.size()), NULL, 0, NULL, NULL);
	if (requiredBytes <= 0) return(std::string());
	std::string utf8(static_cast<size_t>(requiredBytes), '\0');
	WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
		&utf8[0], requiredBytes, NULL, NULL);
	return(utf8);
}

std::wstring Utf8ToWideString(const std::string& text)
{
	if (text.empty()) return(std::wstring());
	const int requiredCharacters = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		text.data(), static_cast<int>(text.size()), NULL, 0);
	if (requiredCharacters <= 0) return(std::wstring());
	std::wstring wide(static_cast<size_t>(requiredCharacters), L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
		static_cast<int>(text.size()), &wide[0], requiredCharacters);
	return(wide);
}

void AppendUInt(std::vector<unsigned char>& data, UINT value)
{
	for (int i = 0; i < 4; ++i)
		data.push_back(static_cast<unsigned char>((value >> (i * 8)) & 0xFF));
}

bool ReadUInt(const std::vector<unsigned char>& data, size_t& offset, UINT& value)
{
	if (offset + 4 > data.size()) return(false);
	value = static_cast<UINT>(data[offset])
		| (static_cast<UINT>(data[offset + 1]) << 8)
		| (static_cast<UINT>(data[offset + 2]) << 16)
		| (static_cast<UINT>(data[offset + 3]) << 24);
	offset += 4;
	return(true);
}

bool ReadBytes(const std::vector<unsigned char>& data, size_t& offset, UINT length,
	std::string& value)
{
	if (offset + length > data.size()) return(false);
	value.assign(reinterpret_cast<const char*>(data.data() + offset),
		reinterpret_cast<const char*>(data.data() + offset + length));
	offset += length;
	return(true);
}
}

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
		{ "TheCA", "Assets/TheCA/TheCAMesh.bin", L"Assets/TheCA/TheCATexture.dds"},
		{ "Touma", "Assets/Touma/ToumaMesh.bin", L"Assets/Touma/ToumaTexture.dds"},
		{ "Janmang", "Assets/Janmang/JanmangMesh.bin", L"Assets/Janmang/JanmangTexture.dds"},
		{ "Poideu", "Assets/Poideu/PoideuMesh.bin", L"Assets/Poideu/PoideuTexture.dds"},
		{ "Rune", "Assets/Rune/RuneMesh.bin", L"Assets/Rune/RuneTexture.dds"},
		{ "Emma", "Assets/Emma/EmmaMesh.bin", L"Assets/Emma/EmmaTexture.dds"},{ "Ritsu", "Assets/Ritsu/RitsuMesh.bin", L"Assets/Ritsu/RitsuTexture.dds"},
		{ "Rope", "Assets/Rope/RopeMesh.bin", L"Assets/Rope/RopeTexture.dds"},
		{ "Bbangbugi", "Assets/Bbangbugi/BbangbugiMesh.bin", L"Assets/Bbangbugi/BbangbugiTexture.dds" }
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
		petResource.pPet->SetPay(1);
		petResource.pPet->GetNowPossession(0);
		petResource.pPet->GetMaxPossession(20);

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

	LoadOrCreateLocalPlayerStatus();
	ApplyFinancialProgressToShopUI();
	SyncMoneyToServer();

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
	d3dPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
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
		{ '$', L"Assets/Image/Numbers/Dollar.dds", 441, 521, 165, 116, 280, 345 },
		{ '-', L"Assets/Image/Numbers/Hyphen.dds", 400, 521, 166, 237, 235, 262 },
		{ '+', L"Assets/Image/Numbers/Plus.dds", 476, 521, 177, 184, 298, 305 },
		{ '(', L"Assets/Image/Numbers/LeftParenthesis.dds", 386, 521, 167, 139, 235, 359 },
		{ ')', L"Assets/Image/Numbers/RightParenthesis.dds", 386, 521, 151, 139, 219, 359 },
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
	hResult = DirectX::LoadDDSTextureFromFile(pd3dDevice, L"Assets/Image/Effects/Coin.dds",
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
	m_ShopUI.Render(pd3dCommandList, pCamera, m_nMoney, m_nActivePetIndex,
		shopPets, GetShopTextRenderContext(), g_pFramework->GetNetworkManager().IsConnected());
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
	if (m_ShopUI.OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam))
		return;

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
	SaveLocalPlayerStatus();
	SyncMoneyToServer();
}

void CGameScene::LoadOrCreateLocalPlayerStatus()
{
	if (!LoadLocalPlayerStatus())
		SaveLocalPlayerStatus();
}

bool CGameScene::LoadLocalPlayerStatus()
{
	if (m_vPetResources.empty()) return(false);

	std::ifstream input(GetLocalPlayerStatusFilePath(), std::ios::binary);
	if (!input) return(false);

	char magic[8] = {};
	UINT version = 0;
	UINT payloadSize = 0;
	UINT checksum = 0;
	input.read(magic, sizeof(magic));
	input.read(reinterpret_cast<char*>(&version), sizeof(version));
	input.read(reinterpret_cast<char*>(&payloadSize), sizeof(payloadSize));
	input.read(reinterpret_cast<char*>(&checksum), sizeof(checksum));
	if (!input || memcmp(magic, "RPLPST01", sizeof(magic)) != 0 || version != 1 ||
		(payloadSize != 12 && payloadSize != 20 && payloadSize != 28
			&& payloadSize < 36))
		return(false);

	std::vector<unsigned char> payload(payloadSize);
	input.read(reinterpret_cast<char*>(payload.data()), payload.size());
	if (!input || input.peek() != EOF) return(false);

	TransformLocalPlayerStatusPayload(payload);
	if (CalculateLocalPlayerStatusChecksum(payload) != checksum) return(false);

	size_t offset = 0;
	UINT money = 0;
	UINT pay = 0;
	UINT maxPossession = 0;
	UINT savingsMaximumProductIndex = 0;
	UINT loanMaximumProductIndex = 0;
	UINT savingsProgressCount = 0;
	UINT loanProgressCount = 0;
	UINT stockIssued = 0;
	UINT stockNameLength = 0;
	std::string stockNameUtf8;
	UINT activePetNameLength = 0;
	std::string activePetName;
	if (!ReadUInt(payload, offset, money) || !ReadUInt(payload, offset, pay)
		|| !ReadUInt(payload, offset, maxPossession))
		return(false);
	if (payloadSize >= 20 && (!ReadUInt(payload, offset, savingsMaximumProductIndex)
		|| !ReadUInt(payload, offset, loanMaximumProductIndex)))
		return(false);
	if (payloadSize >= 28 && (!ReadUInt(payload, offset, savingsProgressCount)
		|| !ReadUInt(payload, offset, loanProgressCount)))
		return(false);
	if (payloadSize >= 36)
	{
		if (!ReadUInt(payload, offset, stockIssued) || !ReadUInt(payload, offset, stockNameLength))
			return(false);
		if (stockNameLength > 150 || !ReadBytes(payload, offset, stockNameLength, stockNameUtf8))
			return(false);
		if (offset < payload.size())
		{
			if (!ReadUInt(payload, offset, activePetNameLength))
				return(false);
			if (activePetNameLength > 150 || !ReadBytes(payload, offset, activePetNameLength, activePetName))
				return(false);
		}
		if (offset != payload.size()) return(false);
	}
	if (pay == 0) pay = 1;
	if (maxPossession == 0) maxPossession = 1;
	if (savingsMaximumProductIndex > 9) savingsMaximumProductIndex = 9;
	if (loanMaximumProductIndex > 9) loanMaximumProductIndex = 9;
	if (savingsProgressCount > 5) savingsProgressCount = 5;
	if (loanProgressCount > 5) loanProgressCount = 5;

	m_nMoney = money;
	m_nFinancialMaximumProductIndices[0] = savingsMaximumProductIndex;
	m_nFinancialMaximumProductIndices[1] = loanMaximumProductIndex;
	m_nFinancialProgressCounts[0] = savingsProgressCount;
	m_nFinancialProgressCounts[1] = loanProgressCount;
	const std::wstring stockName = Utf8ToWideString(stockNameUtf8);
	m_ShopUI.SetStockIssued(stockIssued != 0, stockName);

	UINT activePetIndex = 0;
	if (!activePetName.empty())
	{
		for (size_t i = 0; i < m_vPetResources.size(); ++i)
		{
			CPet* pet = m_vPetResources[i].pPet;
			if (pet && pet->GetName() == activePetName)
			{
				activePetIndex = static_cast<UINT>(i);
				break;
			}
		}
	}
	m_nActivePetIndex = activePetIndex;
	m_pPointedPet = NULL;

	CPet* activePet = m_vPetResources[m_nActivePetIndex].pPet;
	if (!activePet) return(false);
	activePet->SetPay(pay);
	activePet->GetMaxPossession(maxPossession);
	if (activePet->GetNowPossession() > activePet->GetMaxPossession())
		activePet->GetNowPossession(activePet->GetMaxPossession());

	OutputDebugStringA("[LocalPlayerStatus] Loaded local player status.\n");
	return(true);
}

void CGameScene::SaveLocalPlayerStatus() const
{
	if (m_nActivePetIndex >= m_vPetResources.size()) return;
	CPet* activePet = m_vPetResources[m_nActivePetIndex].pPet;
	if (!activePet) return;

	CreateDirectoryA("Network", NULL);

	std::vector<unsigned char> payload;
	const std::string stockNameUtf8 = WideStringToUtf8(m_ShopUI.GetStockName());
	const std::string activePetName = activePet->GetName();
	payload.reserve(40 + stockNameUtf8.size() + activePetName.size());
	AppendUInt(payload, m_nMoney);
	AppendUInt(payload, activePet->GetPay());
	AppendUInt(payload, activePet->GetMaxPossession());
	AppendUInt(payload, m_nFinancialMaximumProductIndices[0]);
	AppendUInt(payload, m_nFinancialMaximumProductIndices[1]);
	AppendUInt(payload, m_nFinancialProgressCounts[0]);
	AppendUInt(payload, m_nFinancialProgressCounts[1]);
	AppendUInt(payload, m_ShopUI.IsStockIssued() ? 1 : 0);
	AppendUInt(payload, static_cast<UINT>(stockNameUtf8.size()));
	payload.insert(payload.end(), stockNameUtf8.begin(), stockNameUtf8.end());
	AppendUInt(payload, static_cast<UINT>(activePetName.size()));
	payload.insert(payload.end(), activePetName.begin(), activePetName.end());
	const UINT checksum = CalculateLocalPlayerStatusChecksum(payload);
	TransformLocalPlayerStatusPayload(payload);

	const char* path = GetLocalPlayerStatusFilePath();
	const char* tempPath = "Network\\LocalPlayerStatus.rplps.tmp";
	std::ofstream output(tempPath, std::ios::binary | std::ios::trunc);
	if (!output) return;

	const char magic[8] = { 'R', 'P', 'L', 'P', 'S', 'T', '0', '1' };
	const UINT version = 1;
	const UINT payloadSize = static_cast<UINT>(payload.size());
	output.write(magic, sizeof(magic));
	output.write(reinterpret_cast<const char*>(&version), sizeof(version));
	output.write(reinterpret_cast<const char*>(&payloadSize), sizeof(payloadSize));
	output.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
	output.write(reinterpret_cast<const char*>(payload.data()), payload.size());
	output.close();
	if (!output) return;

	MoveFileExA(tempPath, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
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
	const bool bShopMessageProcessed = m_ShopUI.OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam,
		m_nMoney, m_vPetResources.size(), m_nActivePetIndex, GetShopTextRenderContext(),
		g_pFramework->GetNetworkManager().IsConnected());
	size_t nConfirmedPetIndex = 0;
	if (m_ShopUI.ConsumePetConfirmationRequest(nConfirmedPetIndex))
		ChangeActivePet(nConfirmedPetIndex);
	int nEnhancementType = -1;
	if (m_ShopUI.ConsumePetEnhancementRequest(nEnhancementType))
		EnhanceActivePet(nEnhancementType);
	if (m_ShopUI.ConsumeLoginSceneReturnRequest())
	{
		g_pFramework->GetNetworkManager().Disconnect();
		g_pFramework->RequestSceneChange(SCENE_TYPE::LOGIN);
		return;
	}
	int nFinancialCategory = -1;
	int nFinancialProductIndex = -1;
	if (m_ShopUI.ConsumeFinancialProductRequest(nFinancialCategory, nFinancialProductIndex))
	{
		const unsigned int nProductId = static_cast<unsigned int>(nFinancialProductIndex + 1);
		if (nFinancialCategory == 0)
			g_pFramework->GetNetworkManager().SendSavingsJoinRequest(nProductId);
		else if (nFinancialCategory == 1)
			g_pFramework->GetNetworkManager().SendLoanApplyRequest(nProductId);
	}
	std::wstring wstrStockName;
	if (m_ShopUI.ConsumeStockIssueRequest(wstrStockName))
	{
		const std::string stockNameUtf8 = WideStringToUtf8(wstrStockName);
		g_pFramework->GetNetworkManager().SendStockIssueRequest(stockNameUtf8);
	}
	if (m_ShopUI.ConsumeStockManagementInfoRequest())
	{
		g_pFramework->GetNetworkManager().SendStockManagementInfoRequest();
	}
	if (m_ShopUI.ConsumeStockTransactionListRequest())
	{
		g_pFramework->GetNetworkManager().SendStockTransactionListRequest();
	}
	if (bShopMessageProcessed)
		return;

	if (nMessageID == WM_LBUTTONDOWN && m_pPointedPet)
		CollectPetPossession(m_pPointedPet);
}

void CGameScene::ChangeActivePet(size_t petIndex)
{
	if (petIndex >= m_vPetResources.size() || m_nActivePetIndex >= m_vPetResources.size()
		|| petIndex == m_nActivePetIndex) return;
	PET_RENDER_RESOURCE& currentResource = m_vPetResources[m_nActivePetIndex];
	PET_RENDER_RESOURCE& selectedResource = m_vPetResources[petIndex];
	if (!currentResource.pPet || !selectedResource.pPet) return;

	selectedResource.pPet->CopyRuntimeStateFrom(*currentResource.pPet);
	m_nActivePetIndex = static_cast<UINT>(petIndex);
	m_pPointedPet = NULL;
	SaveLocalPlayerStatus();
}

void CGameScene::EnhanceActivePet(int enhancementType)
{
	if (m_nActivePetIndex >= m_vPetResources.size()) return;
	CPet* activePet = m_vPetResources[m_nActivePetIndex].pPet;
	if (!activePet) return;
	auto enhancedValue = [](UINT value, int type) -> UINT
	{
		const UINT maxValue = (type == 0) ? 1000 : 10000;
		if (value >= maxValue) return value;
		UINT64 result = 0;
		if (type == 0)
			result = (value < 100) ? static_cast<UINT64>(value) + 1
				: (static_cast<UINT64>(value) * 101 + 99) / 100;
		else
			result = (value < 1000) ? static_cast<UINT64>(value) + 10
				: (static_cast<UINT64>(value) * 101 + 99) / 100;
		if (result > maxValue) result = maxValue;
		return static_cast<UINT>(result);
	};
	auto enhancementPrice = [](UINT value, int type) -> UINT
	{
		const UINT maxValue = (type == 0) ? 1000 : 10000;
		if (value >= maxValue) return UINT_MAX;
		const UINT64 result = (type == 0)
			? 100 + (static_cast<UINT64>(value) * value * 7 + 9) / 10
			: 50 + (static_cast<UINT64>(value) * 3 + 1) / 2;
		return static_cast<UINT>((result > UINT_MAX) ? UINT_MAX : result);
	};
	if (enhancementType != 0 && enhancementType != 1) return;
	const UINT currentValue = (enhancementType == 0)
		? activePet->GetPay() : activePet->GetMaxPossession();
	const UINT nextValue = enhancedValue(currentValue, enhancementType);
	if (nextValue == currentValue) return;
	const UINT price = enhancementPrice(currentValue, enhancementType);
	if (!DiscountMoney(price)) return;
	if (enhancementType == 0)
		activePet->SetPay(nextValue);
	else if (enhancementType == 1)
		activePet->GetMaxPossession(nextValue);
	SaveLocalPlayerStatus();
}
void CGameScene::Animate(float fElapsedTime)
{
	CLIENT_FINANCIAL_APPLICATION_RESULT financialResult;
	while (g_pFramework->GetNetworkManager().ConsumeFinancialApplicationResult(financialResult))
	{
		if (financialResult.eResult != CLIENT_FINANCIAL_RESULT::SUCCESS) continue;
		ApplyServerMoneyChange(financialResult.nFinalMoney);
		const int nCategory = (financialResult.eCategory == CLIENT_FINANCIAL_CATEGORY::SAVINGS) ? 0 : 1;
		if (financialResult.nProductId >= 1 && financialResult.nProductId <= 10)
		{
			const int nProductIndex = static_cast<int>(financialResult.nProductId - 1);
			AdvanceFinancialProgressIfNeeded(nCategory, nProductIndex);
			m_ShopUI.SetFinancialProductActive(nCategory,
				nProductIndex, financialResult.nDurationSeconds);
		}
	}

	CLIENT_FINANCIAL_ACTIVE_STATUS financialActiveStatus;
	while (g_pFramework->GetNetworkManager().ConsumeFinancialActiveStatus(financialActiveStatus))
	{
		const int nCategory = (financialActiveStatus.eCategory == CLIENT_FINANCIAL_CATEGORY::SAVINGS) ? 0 : 1;
		if (financialActiveStatus.nProductId >= 1 && financialActiveStatus.nProductId <= 10)
			m_ShopUI.SetFinancialProductActive(nCategory,
				static_cast<int>(financialActiveStatus.nProductId - 1),
				financialActiveStatus.nRemainingSeconds);
	}

	CLIENT_FINANCIAL_COMPLETION financialCompletion;
	while (g_pFramework->GetNetworkManager().ConsumeFinancialCompletion(financialCompletion))
	{
		const int nCategory = (financialCompletion.eCategory == CLIENT_FINANCIAL_CATEGORY::SAVINGS) ? 0 : 1;
		if (financialCompletion.nProductId >= 1 && financialCompletion.nProductId <= 10)
			m_ShopUI.ClearFinancialProductActive(nCategory,
				static_cast<int>(financialCompletion.nProductId - 1));
	}

	CLIENT_STOCK_ISSUE_APPLICATION_RESULT stockIssueResult;
	while (g_pFramework->GetNetworkManager().ConsumeStockIssueResult(stockIssueResult))
	{
		if (stockIssueResult.eResult == CLIENT_STOCK_ISSUE_RESULT::SUCCESS)
		{
			ApplyServerMoneyChange(stockIssueResult.nFinalMoney);
			m_ShopUI.SetStockIssued(true);
			SaveLocalPlayerStatus();
		}
		else if (stockIssueResult.eResult == CLIENT_STOCK_ISSUE_RESULT::ALREADY_ISSUED)
		{
			m_ShopUI.SetStockIssued(true);
			SaveLocalPlayerStatus();
		}
		else
		{
			m_ShopUI.NotifyStockIssueFailed();
		}
	}

	CLIENT_STOCK_ISSUE_STATUS stockIssueStatus;
	while (g_pFramework->GetNetworkManager().ConsumeStockIssueStatus(stockIssueStatus))
	{
		m_ShopUI.SetStockIssued(stockIssueStatus.bIssued,
			Utf8ToWideString(stockIssueStatus.strStockNameUtf8));
		SaveLocalPlayerStatus();
	}

	CLIENT_STOCK_MANAGEMENT_INFO stockManagementInfo;
	while (g_pFramework->GetNetworkManager().ConsumeStockManagementInfo(stockManagementInfo))
	{
		SHOP_STOCK_MANAGEMENT_INFO shopStockInfo;
		shopStockInfo.bIssued = stockManagementInfo.bIssued;
		shopStockInfo.wstrStockName = Utf8ToWideString(stockManagementInfo.strStockNameUtf8);
		shopStockInfo.nSoldQuantity = stockManagementInfo.nSoldQuantity;
		shopStockInfo.nUnsoldQuantity = stockManagementInfo.nUnsoldQuantity;
		shopStockInfo.nSaleableQuantity = stockManagementInfo.nSaleableQuantity;
		shopStockInfo.nIssuanceRevenue = stockManagementInfo.nIssuanceRevenue;
		shopStockInfo.nRecentTradeQuantity = stockManagementInfo.nRecentTradeQuantity;
		shopStockInfo.nCurrentPrice = stockManagementInfo.nCurrentPrice;
		shopStockInfo.nPreviousPrice = stockManagementInfo.nPreviousPrice;
		for (int i = 0; i < 3; ++i)
		{
			shopStockInfo.TopHolders[i].wstrPlayerId =
				Utf8ToWideString(stockManagementInfo.TopHolders[i].strPlayerId);
			shopStockInfo.TopHolders[i].nQuantity =
				stockManagementInfo.TopHolders[i].nQuantity;
		}
		for (const CLIENT_STOCK_PRICE_INFO& price : stockManagementInfo.RecentPrices)
		{
			SHOP_STOCK_PRICE_INFO shopPrice;
			shopPrice.nPreviousPrice = price.nPreviousPrice;
			shopPrice.nNewPrice = price.nNewPrice;
			shopPrice.nBoughtQuantity = price.nBoughtQuantity;
			shopPrice.nSoldQuantity = price.nSoldQuantity;
			shopPrice.wstrChangedTime = Utf8ToWideString(price.strChangedTime);
			shopStockInfo.RecentPrices.push_back(shopPrice);
		}
		m_ShopUI.SetStockManagementInfo(shopStockInfo);
	}

	std::vector<CLIENT_STOCK_TRANSACTION_INFO> stockTransactionInfos;
	while (g_pFramework->GetNetworkManager().ConsumeStockTransactionList(stockTransactionInfos))
	{
		std::vector<SHOP_STOCK_TRANSACTION_INFO> shopStockTransactionInfos;
		shopStockTransactionInfos.reserve(stockTransactionInfos.size());
		for (const CLIENT_STOCK_TRANSACTION_INFO& stockInfo : stockTransactionInfos)
		{
			SHOP_STOCK_TRANSACTION_INFO shopStockInfo;
			shopStockInfo.nStockId = stockInfo.nStockId;
			shopStockInfo.wstrStockName = Utf8ToWideString(stockInfo.strStockNameUtf8);
			shopStockInfo.wstrIssuerId = Utf8ToWideString(stockInfo.strIssuerId);
			shopStockInfo.nCurrentPrice = stockInfo.nCurrentPrice;
			shopStockInfo.nPreviousPrice = stockInfo.nPreviousPrice;
			shopStockInfo.nSaleableQuantity = stockInfo.nSaleableQuantity;
			shopStockInfo.nMyQuantity = stockInfo.nMyQuantity;
			shopStockInfo.nRecentTradeQuantity = stockInfo.nRecentTradeQuantity;
			shopStockTransactionInfos.push_back(shopStockInfo);
		}
		m_ShopUI.SetStockTransactionInfos(shopStockTransactionInfos);
	}

	std::int64_t nServerMoneyDelta = 0;
	UINT nServerFinalMoney = 0;
	while (g_pFramework->GetNetworkManager().ConsumeServerMoneyChange(
		nServerMoneyDelta, nServerFinalMoney))
	{
		ApplyServerMoneyChange(nServerFinalMoney);
	}

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
	SaveLocalPlayerStatus();
	SyncMoneyToServer();
	return true;
}

void CGameScene::SetMoney(UINT p)
{
	m_nMoney = p;
	SaveLocalPlayerStatus();
	SyncMoneyToServer();
}

void CGameScene::AddMoney(UINT p)
{
	m_nMoney += p;
	SaveLocalPlayerStatus();
	SyncMoneyToServer();
}

void CGameScene::SyncMoneyToServer() const
{
	g_pFramework->GetNetworkManager().SendMoneyUpdate(m_nMoney);
}

void CGameScene::ApplyServerMoneyChange(UINT nMoney)
{
	m_nMoney = nMoney;
	SaveLocalPlayerStatus();
}

void CGameScene::ApplyFinancialProgressToShopUI()
{
	m_ShopUI.SetFinancialMaximumProductIndex(0,
		static_cast<int>(m_nFinancialMaximumProductIndices[0]));
	m_ShopUI.SetFinancialMaximumProductIndex(1,
		static_cast<int>(m_nFinancialMaximumProductIndices[1]));
	m_ShopUI.SetFinancialProgressCount(0,
		static_cast<int>(m_nFinancialProgressCounts[0]));
	m_ShopUI.SetFinancialProgressCount(1,
		static_cast<int>(m_nFinancialProgressCounts[1]));
}

void CGameScene::AdvanceFinancialProgressIfNeeded(int nCategory, int nProductIndex)
{
	if (nCategory < 0 || nCategory >= 2 || nProductIndex < 0 || nProductIndex >= 10) return;
	if (m_nFinancialProgressCounts[nCategory] < 5)
		++m_nFinancialProgressCounts[nCategory];
	const UINT nCurrentMaximumIndex = m_nFinancialMaximumProductIndices[nCategory];
	if (static_cast<UINT>(nProductIndex) == nCurrentMaximumIndex && nCurrentMaximumIndex < 9)
		m_nFinancialMaximumProductIndices[nCategory] = nCurrentMaximumIndex + 1;
	ApplyFinancialProgressToShopUI();
	SaveLocalPlayerStatus();
}
