//-----------------------------------------------------------------------------
// File: Scene.h
//-----------------------------------------------------------------------------

#pragma once

#include "Shader.h"
#include "Player.h"

class CScene
{
public:
	CScene() {}
    CScene(CPlayer* pPlayer);
    ~CScene();

	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseObjects();

	ID3D12RootSignature *CreateGraphicsRootSignature(ID3D12Device *pd3dDevice);
	ID3D12RootSignature *GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }

	bool ProcessInput();
	virtual void Animate(float fTimeElapsed);
	void PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera=NULL);

	virtual void ReleaseUploadBuffers();
	void SetPlayer(CPlayer* pPlayer) { m_pPlayer = pPlayer; }
	void BuildGraphicsRootSignature(ID3D12Device* pd3dDevice);
protected:
	CPlayer* m_pPlayer = NULL;
protected:
	ID3D12RootSignature			*m_pd3dGraphicsRootSignature = NULL;

};


class CTankScene : public CScene {
public:
	CTankScene() {}
	CTankScene(CPlayer* pPlayer);
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) override;
	virtual void ReleaseObjects() override;
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) override;
	virtual void Animate(float fElapsedTime) override;

	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;
	CGameObject* PickObjectPointedByCursor(int xClient, int yClient, CCamera* pCamera);
	virtual void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) override;

	void CTankScene::CheckTankByBulletCollisions();
	void CTankScene::CheckPlayerByBulletCollisions();
	void CTankScene::CheckPlayerByObjectCollisions(float fElapsedTime);
	void CTankScene::CheckBulletByObjectCollisions();

private:
	CCubeObject* m_pFloorObject;

	static const int m_nTanks = 10;
	CTankObject* m_pTank[m_nTanks];

	static const int m_nCubeObjects = 5;
	CCubeObject* m_pCubeObjects[m_nCubeObjects];

	int GameSet = 0;
	CTitleObject* m_pYWObjects;
};