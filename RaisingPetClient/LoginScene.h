#pragma once

#include "Scene.h"

class CLoginScene : public CScene
{
public:
	virtual void BuildObjects(ID3D12Device* pd3dDevice,
		ID3D12GraphicsCommandList* pd3dCommandList) override;
	virtual void ReleaseObjects() override;
	virtual void ReleaseUploadBuffers() override;
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) override;
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID,
		WPARAM wParam, LPARAM lParam) override;
	virtual CGameObject* PickObjectPointedByCursor(int xClient, int yClient,
		CCamera* pCamera) override;

private:
	struct UI_IMAGE_RESOURCE
	{
		ID3D12Resource* pd3dTexture = NULL;
		ID3D12Resource* pd3dTextureUploadBuffer = NULL;
		ID3D12DescriptorHeap* pd3dSrvDescriptorHeap = NULL;
	};

	ID3D12PipelineState* m_pd3dUiPipelineState = NULL;
	UI_IMAGE_RESOURCE m_ShopBoard;
	UI_IMAGE_RESOURCE m_CloseIcon;
	UI_IMAGE_RESOURCE m_LoginFrame;
	UI_IMAGE_RESOURCE m_IdLog;
	UI_IMAGE_RESOURCE m_PasswordLog;
	UI_IMAGE_RESOURCE m_TextFrame;
	UI_IMAGE_RESOURCE m_LoginButton;
	UI_IMAGE_RESOURCE m_GuestButton;
	CGameObject m_UiHitObject;

	void RenderUiImage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		UI_IMAGE_RESOURCE& resource, const XMFLOAT4& rectangle);
};
