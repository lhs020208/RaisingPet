//-----------------------------------------------------------------------------
// File: Scene.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Scene.h"
#include "GameFramework.h"
#include "DDSTextureLoader12.h"
#include "d3dx12.h"
extern CGameFramework* g_pFramework;

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

	D3D12_ROOT_PARAMETER pd3dRootParameters[4];
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
	m_vPetResources.emplace_back();
	PET_RENDER_RESOURCE& petResource = m_vPetResources.back();
	CPseudoLightingShader* pObjectShader = new CPseudoLightingShader();
	pObjectShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);

	char pstrTheCAMeshFile0[] = "Assets/TheCA/TheCA.obj";
	char pstrTheCAMeshFile1[] = "RaisingPetClient/Assets/TheCA/TheCA.obj";
	char pstrTheCAMeshFile2[] = "../Assets/TheCA/TheCA.obj";
	char pstrTheCAMeshFile3[] = "../../Assets/TheCA/TheCA.obj";
	char* pstrTheCAMeshFile = pstrTheCAMeshFile0;
	if (GetFileAttributesA(pstrTheCAMeshFile) == INVALID_FILE_ATTRIBUTES) pstrTheCAMeshFile = pstrTheCAMeshFile1;
	if (GetFileAttributesA(pstrTheCAMeshFile) == INVALID_FILE_ATTRIBUTES) pstrTheCAMeshFile = pstrTheCAMeshFile2;
	if (GetFileAttributesA(pstrTheCAMeshFile) == INVALID_FILE_ATTRIBUTES) pstrTheCAMeshFile = pstrTheCAMeshFile3;
	CMesh* pTheCAMesh = new CMesh(pd3dDevice, pd3dCommandList, pstrTheCAMeshFile);
	petResource.pPet = new CPet();
	petResource.pPet->SetMesh(pTheCAMesh);
	petResource.pPet->SetShader(pObjectShader);
	petResource.pPet->SetPosition(0.0f, -25.0f, 0.0f);
	petResource.pPet->SetColor(XMFLOAT3(1.0f, 1.0f, 1.0f));

		{
		std::unique_ptr<uint8_t[]> textureData;
		std::vector<D3D12_SUBRESOURCE_DATA> textureSubresources;
		HRESULT hTextureResult = DirectX::LoadDDSTextureFromFile(pd3dDevice,
			L"Assets/TheCA/CubePaint.dds", &petResource.pd3dTexture, textureData, textureSubresources);
		if (FAILED(hTextureResult))
			hTextureResult = DirectX::LoadDDSTextureFromFile(pd3dDevice,
				L"RaisingPetClient/Assets/TheCA/CubePaint.dds", &petResource.pd3dTexture, textureData, textureSubresources);
		if (FAILED(hTextureResult))
			hTextureResult = DirectX::LoadDDSTextureFromFile(pd3dDevice,
				L"../Assets/TheCA/CubePaint.dds", &petResource.pd3dTexture, textureData, textureSubresources);
		if (FAILED(hTextureResult))
			hTextureResult = DirectX::LoadDDSTextureFromFile(pd3dDevice,
				L"../../Assets/TheCA/CubePaint.dds", &petResource.pd3dTexture, textureData, textureSubresources);

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
// Create the full-screen texture pipeline.
	ID3DBlob* pd3dVertexShaderBlob = NULL;
	ID3DBlob* pd3dPixelShaderBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;

	HRESULT hResult = D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "VSFullscreenTexture", "vs_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pd3dVertexShaderBlob, &pd3dErrorBlob);
	if (FAILED(hResult))
	{
		if (pd3dErrorBlob) OutputDebugStringA(static_cast<const char*>(pd3dErrorBlob->GetBufferPointer()));
		if (pd3dErrorBlob) pd3dErrorBlob->Release();
		return;
	}
	if (pd3dErrorBlob) { pd3dErrorBlob->Release(); pd3dErrorBlob = NULL; }

	hResult = D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "PSFullscreenTexture", "ps_5_1",
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
		__uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&m_pd3dFullscreenPipelineState));
	pd3dVertexShaderBlob->Release();
	pd3dPixelShaderBlob->Release();
	if (FAILED(hResult)) return;

	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	hResult = DirectX::LoadDDSTextureFromFile(pd3dDevice, L"Assets/Image/Test.dds",
		&m_pd3dFullscreenTexture, ddsData, subresources);
	if (FAILED(hResult))
	{
		hResult = DirectX::LoadDDSTextureFromFile(pd3dDevice, L"RaisingPetClient/Assets/Image/Test.dds",
			&m_pd3dFullscreenTexture, ddsData, subresources);
	}
	if (FAILED(hResult))
	{
		hResult = DirectX::LoadDDSTextureFromFile(pd3dDevice, L"../../Assets/Image/Test.dds",
			&m_pd3dFullscreenTexture, ddsData, subresources);
	}
	if (FAILED(hResult) || subresources.empty()) return;


	const UINT64 nUploadBufferSize = GetRequiredIntermediateSize(m_pd3dFullscreenTexture, 0,
		static_cast<UINT>(subresources.size()));
	CD3DX12_HEAP_PROPERTIES d3dUploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC d3dUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(nUploadBufferSize);
	hResult = pd3dDevice->CreateCommittedResource(&d3dUploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&d3dUploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
		__uuidof(ID3D12Resource), reinterpret_cast<void**>(&m_pd3dFullscreenTextureUploadBuffer));
	if (FAILED(hResult)) return;

	UpdateSubresources(pd3dCommandList, m_pd3dFullscreenTexture, m_pd3dFullscreenTextureUploadBuffer,
		0, 0, static_cast<UINT>(subresources.size()), subresources.data());
	CD3DX12_RESOURCE_BARRIER d3dTextureBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pd3dFullscreenTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pd3dCommandList->ResourceBarrier(1, &d3dTextureBarrier);

	D3D12_DESCRIPTOR_HEAP_DESC d3dSrvHeapDesc = {};
	d3dSrvHeapDesc.NumDescriptors = 1;
	d3dSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d3dSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	hResult = pd3dDevice->CreateDescriptorHeap(&d3dSrvHeapDesc, __uuidof(ID3D12DescriptorHeap),
		reinterpret_cast<void**>(&m_pd3dFullscreenSrvDescriptorHeap));
	if (FAILED(hResult)) return;

	D3D12_SHADER_RESOURCE_VIEW_DESC d3dSrvDesc = {};
	d3dSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	d3dSrvDesc.Format = m_pd3dFullscreenTexture->GetDesc().Format;
	d3dSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	d3dSrvDesc.Texture2D.MipLevels = m_pd3dFullscreenTexture->GetDesc().MipLevels;
	pd3dDevice->CreateShaderResourceView(m_pd3dFullscreenTexture, &d3dSrvDesc,
		m_pd3dFullscreenSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
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

	if (m_pd3dFullscreenPipelineState) m_pd3dFullscreenPipelineState->Release();
	if (m_pd3dFullscreenTexture) m_pd3dFullscreenTexture->Release();
	if (m_pd3dFullscreenTextureUploadBuffer) m_pd3dFullscreenTextureUploadBuffer->Release();
	if (m_pd3dFullscreenSrvDescriptorHeap) m_pd3dFullscreenSrvDescriptorHeap->Release();
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

	if (m_pd3dFullscreenTextureUploadBuffer)
	{
		m_pd3dFullscreenTextureUploadBuffer->Release();
		m_pd3dFullscreenTextureUploadBuffer = NULL;
	}
}
void CGameScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (!pCamera) return;

	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);
	for (PET_RENDER_RESOURCE& petResource : m_vPetResources)
	{
		if (!petResource.pPet || !petResource.pd3dSrvDescriptorHeap) continue;

		ID3D12DescriptorHeap* ppd3dPetDescriptorHeaps[] = { petResource.pd3dSrvDescriptorHeap };
		pd3dCommandList->SetDescriptorHeaps(1, ppd3dPetDescriptorHeaps);
		pd3dCommandList->SetGraphicsRootDescriptorTable(3,
			petResource.pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		petResource.pPet->Render(pd3dCommandList, pCamera);
	}

	if (m_pd3dFullscreenPipelineState && m_pd3dFullscreenSrvDescriptorHeap)
	{
		/*ID3D12DescriptorHeap* ppd3dDescriptorHeaps[] = { m_pd3dFullscreenSrvDescriptorHeap };
		pd3dCommandList->SetDescriptorHeaps(1, ppd3dDescriptorHeaps);
		pd3dCommandList->SetGraphicsRootDescriptorTable(3,
			m_pd3dFullscreenSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		pd3dCommandList->SetPipelineState(m_pd3dFullscreenPipelineState);
		pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pd3dCommandList->DrawInstanced(3, 1, 0, 0);*/
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

	XMFLOAT3 xmf3PickPosition;
	xmf3PickPosition.x = (((2.0f * xClient) / (float)pCamera->m_d3dViewport.Width) - 1) / pCamera->m_xmf4x4Projection._11;
	xmf3PickPosition.y = -(((2.0f * yClient) / (float)pCamera->m_d3dViewport.Height) - 1) / pCamera->m_xmf4x4Projection._22;
	xmf3PickPosition.z = 1.0f;

	XMVECTOR xmvPickPosition = XMLoadFloat3(&xmf3PickPosition);
	XMMATRIX xmmtxView = XMLoadFloat4x4(&pCamera->m_xmf4x4View);

	float fNearestHitDistance = FLT_MAX;
	CGameObject* pNearestObject = NULL;
	/*for (int i = 0; i < m_nTanks; i++) {
		if (m_pTank[i])
		{
			int hit = m_pTank[i]->PickObjectByRayIntersection(xmvPickPosition, xmmtxView, &fNearestHitDistance);
			if (hit > 0)
			{
				pNearestObject = m_pTank[i];
			}
		}
	}*/
	return(pNearestObject);

}
void CGameScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_RBUTTONDOWN:
	{
		
	}
	}
}

void CGameScene::Animate(float fElapsedTime)
{
	for (PET_RENDER_RESOURCE& petResource : m_vPetResources)
	{
		if (petResource.pPet) petResource.pPet->Animate(fElapsedTime);
	}
}