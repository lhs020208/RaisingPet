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
	const float left = board.x + boardWidth * 0.055f;
	const float top = board.y + boardHeight * 0.21f;
	const float frameWidth = boardWidth * 0.66f;
	return(XMFLOAT4(left, top, left + frameWidth,
		top + frameWidth * (1208.0f / 1906.0f)));
}

XMFLOAT4 GetLoginButtonRectangle(bool guest, float width, float height)
{
	const XMFLOAT4 board = GetBoardRectangle(width, height);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float buttonWidth = boardWidth * 0.17f;
	const float buttonHeight = buttonWidth * ((guest ? 217.0f : 216.0f) / 447.0f);
	const float left = board.x + boardWidth * 0.775f;
	const float top = board.y + boardHeight * (guest ? 0.64f : 0.39f);
	return(XMFLOAT4(left, top, left + buttonWidth, top + buttonHeight));
}

XMFLOAT4 GetLabelRectangle(bool password, float width, float height)
{
	const XMFLOAT4 frame = GetLoginFrameRectangle(width, height);
	const float frameWidth = frame.z - frame.x;
	const float frameHeight = frame.w - frame.y;
	const float labelWidth = frameWidth * 0.16f;
	const float labelHeight = labelWidth * ((password ? 274.0f : 237.0f) / (password ? 313.0f : 314.0f));
	const float left = frame.x + frameWidth * 0.065f;
	const float centerY = frame.y + frameHeight * (password ? 0.70f : 0.32f);
	return(XMFLOAT4(left, centerY - labelHeight * 0.5f,
		left + labelWidth, centerY + labelHeight * 0.5f));
}

XMFLOAT4 GetTextFrameRectangle(bool password, float width, float height)
{
	const XMFLOAT4 frame = GetLoginFrameRectangle(width, height);
	const XMFLOAT4 label = GetLabelRectangle(password, width, height);
	const float frameWidth = frame.z - frame.x;
	const float textWidth = frameWidth * 0.66f;
	const float textHeight = textWidth * (156.0f / 1254.0f);
	const float left = frame.x + frameWidth * 0.27f;
	const float centerY = (label.y + label.w) * 0.5f;
	return(XMFLOAT4(left, centerY - textHeight * 0.5f,
		left + textWidth, centerY + textHeight * 0.5f));
}
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
	pipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipeline.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	pipeline.SampleDesc.Count = 1;
	device->CreateGraphicsPipelineState(&pipeline, __uuidof(ID3D12PipelineState),
		reinterpret_cast<void**>(&m_pd3dUiPipelineState));
	vertexShader->Release();
	pixelShader->Release();

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

	loadImage(L"Assets/Image/ShopBoard.dds", m_ShopBoard);
	loadImage(L"Assets/Image/ShopCloseIcon.dds", m_CloseIcon);
	loadImage(L"Assets/Image/LoginFrame.dds", m_LoginFrame);
	loadImage(L"Assets/Image/IDLog.dds", m_IdLog);
	loadImage(L"Assets/Image/PasswordLog.dds", m_PasswordLog);
	loadImage(L"Assets/Image/TextFrame.dds", m_TextFrame);
	loadImage(L"Assets/Image/LoginButton.dds", m_LoginButton);
	loadImage(L"Assets/Image/GuestButton.dds", m_GuestButton);
}

void CLoginScene::ReleaseObjects()
{
	if (m_pd3dUiPipelineState) m_pd3dUiPipelineState->Release();
	m_pd3dUiPipelineState = NULL;
	UI_IMAGE_RESOURCE* resources[] = { &m_ShopBoard, &m_CloseIcon, &m_LoginFrame,
		&m_IdLog, &m_PasswordLog, &m_TextFrame, &m_LoginButton, &m_GuestButton };
	for (UI_IMAGE_RESOURCE* resource : resources)
	{
		if (resource->pd3dTexture) resource->pd3dTexture->Release();
		if (resource->pd3dTextureUploadBuffer) resource->pd3dTextureUploadBuffer->Release();
		if (resource->pd3dSrvDescriptorHeap) resource->pd3dSrvDescriptorHeap->Release();
		*resource = UI_IMAGE_RESOURCE();
	}
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();
	m_pd3dGraphicsRootSignature = NULL;
}

void CLoginScene::ReleaseUploadBuffers()
{
	UI_IMAGE_RESOURCE* resources[] = { &m_ShopBoard, &m_CloseIcon, &m_LoginFrame,
		&m_IdLog, &m_PasswordLog, &m_TextFrame, &m_LoginButton, &m_GuestButton };
	for (UI_IMAGE_RESOURCE* resource : resources)
	{
		if (!resource->pd3dTextureUploadBuffer) continue;
		resource->pd3dTextureUploadBuffer->Release();
		resource->pd3dTextureUploadBuffer = NULL;
	}
}

void CLoginScene::RenderUiImage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	UI_IMAGE_RESOURCE& resource, const XMFLOAT4& rectangle)
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
	commandList->SetGraphicsRoot32BitConstant(5, 0x00FFFFFF, 0);
	commandList->SetPipelineState(m_pd3dUiPipelineState);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(6, 1, 0, 0);
}

void CLoginScene::Render(ID3D12GraphicsCommandList* commandList, CCamera* camera)
{
	if (!camera) return;
	camera->SetViewportsAndScissorRects(commandList);
	camera->UpdateShaderVariables(commandList);
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	RenderUiImage(commandList, camera, m_ShopBoard, GetBoardRectangle(width, height));
	RenderUiImage(commandList, camera, m_LoginFrame, GetLoginFrameRectangle(width, height));
	RenderUiImage(commandList, camera, m_IdLog, GetLabelRectangle(false, width, height));
	RenderUiImage(commandList, camera, m_PasswordLog, GetLabelRectangle(true, width, height));
	RenderUiImage(commandList, camera, m_TextFrame, GetTextFrameRectangle(false, width, height));
	RenderUiImage(commandList, camera, m_TextFrame, GetTextFrameRectangle(true, width, height));
	RenderUiImage(commandList, camera, m_LoginButton, GetLoginButtonRectangle(false, width, height));
	RenderUiImage(commandList, camera, m_GuestButton, GetLoginButtonRectangle(true, width, height));
	RenderUiImage(commandList, camera, m_CloseIcon, GetCloseRectangle(width, height));
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
		PostMessage(hWnd, WM_CLOSE, 0, 0);
	else if (PointInRectangle(x, y, GetLoginButtonRectangle(false, width, height))
		|| PointInRectangle(x, y, GetLoginButtonRectangle(true, width, height)))
		g_pFramework->RequestSceneChange(SCENE_TYPE::GAME);
}

CGameObject* CLoginScene::PickObjectPointedByCursor(int x, int y, CCamera* camera)
{
	if (!camera) return(NULL);
	return PointInRectangle(static_cast<float>(x), static_cast<float>(y),
		GetBoardRectangle(camera->m_d3dViewport.Width, camera->m_d3dViewport.Height))
		? &m_UiHitObject : NULL;
}
