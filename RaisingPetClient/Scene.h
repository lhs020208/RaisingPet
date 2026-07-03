//-----------------------------------------------------------------------------
// File: Scene.h
//-----------------------------------------------------------------------------

#pragma once

#include "Shader.h"

class CScene
{
public:
	CScene();
    ~CScene();

	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual CGameObject* PickObjectPointedByCursor(int xClient, int yClient, CCamera* pCamera);

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseObjects();

	ID3D12RootSignature *CreateGraphicsRootSignature(ID3D12Device *pd3dDevice);
	ID3D12RootSignature *GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }

	bool ProcessInput();
	virtual void Animate(float fTimeElapsed);
	void PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera=NULL);

	virtual void ReleaseUploadBuffers();
	void BuildGraphicsRootSignature(ID3D12Device* pd3dDevice);

protected:
	ID3D12RootSignature			*m_pd3dGraphicsRootSignature = NULL;

};


class CGameScene : public CScene {
public:
	CGameScene() {}
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;
	virtual void ReleaseObjects() override;
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) override;
	virtual void Animate(float fElapsedTime) override;

	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;
	virtual CGameObject* PickObjectPointedByCursor(int xClient, int yClient, CCamera* pCamera) override;
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;

private:
	struct PET_RENDER_RESOURCE
	{
		CPet* pPet = NULL;
		ID3D12Resource* pd3dTexture = NULL;
		ID3D12Resource* pd3dTextureUploadBuffer = NULL;
		ID3D12DescriptorHeap* pd3dSrvDescriptorHeap = NULL;
	};

	std::vector<PET_RENDER_RESOURCE> m_vPetResources;
	UINT m_nActivePetIndex = 0;
	CPet* m_pPointedPet = NULL;

	struct TEXT_GLYPH_RESOURCE
	{
		char ch = 0;
		ID3D12Resource* pd3dTexture = NULL;
		ID3D12Resource* pd3dTextureUploadBuffer = NULL;
		ID3D12DescriptorHeap* pd3dSrvDescriptorHeap = NULL;
		float fU0 = 0.0f;
		float fV0 = 0.0f;
		float fU1 = 1.0f;
		float fV1 = 1.0f;
		float fPixelWidth = 0.0f;
		float fPixelHeight = 0.0f;
		float fTopOffset = 0.0f;
	};

	ID3D12PipelineState* m_pd3dTextPipelineState = NULL;
	ID3D12PipelineState* m_pd3dSolidUiPipelineState = NULL;
	ID3D12PipelineState* m_pd3dCoinPipelineState = NULL;
	std::vector<TEXT_GLYPH_RESOURCE> m_vTextGlyphResources;

	struct COIN_EFFECT
	{
		XMFLOAT3 xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 xmf3Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
		float fAge = 0.0f;
		float fLifetime = 1.0f;
	};

	ID3D12Resource* m_pd3dCoinTexture = NULL;
	ID3D12Resource* m_pd3dCoinTextureUploadBuffer = NULL;
	ID3D12DescriptorHeap* m_pd3dCoinSrvDescriptorHeap = NULL;
	std::vector<COIN_EFFECT> m_vCoinEffects;
	std::mt19937 m_CoinRandomEngine{ std::random_device{}() };

	void RenderPetPossessionText(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, CPet* pPet);
	void RenderMoneyUI(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	void RenderSolidUiRectangle(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		float fLeft, float fTop, float fRight, float fBottom, UINT nColor);
	void CollectPetPossession(CPet* pPet);
	void SpawnCoinEffects(CPet* pPet, UINT nPossessionBeforeCollection);
	void AnimateCoinEffects(float fElapsedTime);
	void RenderCoinEffects(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

public:
	UINT GetMoney() { return m_nMoney; }
	void SetMoney(UINT p) { m_nMoney = p; }
	void AddMoney(UINT p) { m_nMoney += p; }
	bool DiscountMoney(UINT p);

private:
	UINT m_nMoney = 0;
};