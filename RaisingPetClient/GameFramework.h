//-----------------------------------------------------------------------------
// File: GameFramework.h
//-----------------------------------------------------------------------------
#pragma once

#include "Timer.h"
#include "Camera.h"
#include "SceneManager.h"
#include "ClientNetworkManager.h"
#include "D2DTextRenderer.h"

class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();

	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();

	void CreateRtvAndDsvDescriptorHeaps();

	void CreateRenderTargetViews();
	void CreateDepthStencilView();
	void CreateRenderTargetViewsAndDepthStencilView();

	void ChangeSwapChainState();

    void BuildObjects();
    void ReleaseObjects();

    void ProcessInput();
    void AnimateObjects();
	bool IsPointOverPet(int xClient, int yClient);
	void UpdateMouseTransparency();
    void FrameAdvance();

	virtual void CreateShaderVariables();
	virtual void UpdateShaderVariables();
	virtual void ReleaseShaderVariables();

	void WaitForGpuComplete();
	void MoveToNextFrame();

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	ID3D12Device* GetDevice() const { return m_pd3dDevice; }
	ID3D12GraphicsCommandList* GetCommandList() const { return m_pd3dCommandList; }
	void RequestSceneChange(SCENE_TYPE sceneType) { m_SceneManager.RequestSceneChange(sceneType); }
	CClientNetworkManager& GetNetworkManager() { return m_NetworkManager; }
	void QueueDirectWriteText(const std::wstring& text, const XMFLOAT4& rectangle, float fontSize,
		UINT color = 0xFF000000, bool horizontalCenter = false, bool verticalCenter = true);
private:
	void ProcessSceneChange();
	HINSTANCE					m_hInstance;
	HWND						m_hWnd; 

	int							m_nWndClientWidth;
	int							m_nWndClientHeight;
        
	IDXGIFactory4				*m_pdxgiFactory;
	IDXGISwapChain3				*m_pdxgiSwapChain;
	IDCompositionDevice			*m_pdcompDevice;
	IDCompositionTarget			*m_pdcompTarget;
	IDCompositionVisual			*m_pdcompVisual;
	ID3D12Device				*m_pd3dDevice;

	bool						m_bMsaa4xEnable = false;
	UINT						m_nMsaa4xQualityLevels = 0;

	static const UINT			m_nSwapChainBuffers = 2;
	UINT						m_nSwapChainBufferIndex;

	ID3D12Resource				*m_ppd3dSwapChainBackBuffers[m_nSwapChainBuffers];
	ID3D12DescriptorHeap		*m_pd3dRtvDescriptorHeap;
	UINT						m_nRtvDescriptorIncrementSize;

	ID3D12Resource				*m_pd3dDepthStencilBuffer;
	ID3D12DescriptorHeap		*m_pd3dDsvDescriptorHeap;
	UINT						m_nDsvDescriptorIncrementSize;

	ID3D12CommandAllocator		*m_pd3dCommandAllocator;
	ID3D12CommandQueue			*m_pd3dCommandQueue;
	ID3D12GraphicsCommandList	*m_pd3dCommandList;

	ID3D12Fence					*m_pd3dFence;
	UINT64						m_nFenceValues[m_nSwapChainBuffers];
	HANDLE						m_hFenceEvent;
	CD2DTextRenderer			m_D2DTextRenderer;

#if defined(_DEBUG)
	ID3D12Debug					*m_pd3dDebugController;
#endif

	CSceneManager				m_SceneManager;
	CClientNetworkManager		m_NetworkManager;
	CCamera						*m_pCamera = NULL;

	POINT						m_ptOldCursorPos;
	bool						m_bMouseTransparent = false;

	CGameTimer					m_GameTimer;
	_TCHAR						m_pszFrameRate[50];

};

