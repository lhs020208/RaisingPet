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
	virtual void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID,
		WPARAM wParam, LPARAM lParam) override;
	virtual void Animate(float fTimeElapsed) override;
	virtual CGameObject* PickObjectPointedByCursor(int xClient, int yClient,
		CCamera* pCamera) override;

private:
	struct UI_IMAGE_RESOURCE
	{
		ID3D12Resource* pd3dTexture = NULL;
		ID3D12Resource* pd3dTextureUploadBuffer = NULL;
		ID3D12DescriptorHeap* pd3dSrvDescriptorHeap = NULL;
	};
	struct GLYPH_RESOURCE
	{
		char ch = 0;
		UI_IMAGE_RESOURCE image;
		float imageWidth = 0.0f;
		float imageHeight = 0.0f;
		float pixelWidth = 0.0f;
		float pixelHeight = 0.0f;
		float u0 = 0.0f;
		float v0 = 0.0f;
		float topOffset = 0.0f;
	};
	struct LOGIN_ERROR_LOG
	{
		XMFLOAT4 rectangle = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		float elapsedTime = 0.0f;
		int type = 0;
	};
	enum class LOGIN_PAGE
	{
		INPUT,
		LOADING,
		REGISTER
	};
	enum class REGISTER_RESULT
	{
		SUCCESS,
		FAILURE
	};

	ID3D12PipelineState* m_pd3dUiPipelineState = NULL;
	ID3D12PipelineState* m_pd3dTextPipelineState = NULL;
	ID3D12PipelineState* m_pd3dRotatingUiPipelineState = NULL;
	UI_IMAGE_RESOURCE m_ShopBoard;
	UI_IMAGE_RESOURCE m_CloseIcon;
	UI_IMAGE_RESOURCE m_LoginFrame;
	UI_IMAGE_RESOURCE m_IdLog;
	UI_IMAGE_RESOURCE m_PasswordLog;
	UI_IMAGE_RESOURCE m_TextFrame;
	UI_IMAGE_RESOURCE m_LoginButton;
	UI_IMAGE_RESOURCE m_GuestButton;
	UI_IMAGE_RESOURCE m_RegisterButton;
	UI_IMAGE_RESOURCE m_TextCursor;
	UI_IMAGE_RESOURCE m_LoginErrorLog;
	UI_IMAGE_RESOURCE m_LoginErrorLog2;
	UI_IMAGE_RESOURCE m_RegisterSuccessLog;
	UI_IMAGE_RESOURCE m_RegisterFailLog;
	UI_IMAGE_RESOURCE m_LoginLoading;
	UI_IMAGE_RESOURCE m_LoadingTexts[3];
	UI_IMAGE_RESOURCE m_DirectStartButton;
	UI_IMAGE_RESOURCE m_LoginBackButton;
	UI_IMAGE_RESOURCE m_PasswordHideIcon;
	UI_IMAGE_RESOURCE m_PasswordHideCheckBox;
	std::vector<GLYPH_RESOURCE> m_Glyphs;
	std::vector<LOGIN_ERROR_LOG> m_LoginErrorLogs;
	std::string m_LoginId;
	std::string m_LoginPassword;
	size_t m_CursorIndices[2] = { 0, 0 };
	int m_nActiveTextField = -1;
	float m_fCursorBlinkElapsed = 0.0f;
	float m_fLoadingElapsedTime = 0.0f;
	bool m_bPasswordVisible = false;
	LOGIN_PAGE m_eLoginPage = LOGIN_PAGE::INPUT;
	CGameObject m_UiHitObject;

	void RenderUiImage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		UI_IMAGE_RESOURCE& resource, const XMFLOAT4& rectangle, UINT nColor = 0x00FFFFFF);
	void RenderRotatingUiImage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		UI_IMAGE_RESOURCE& resource, const XMFLOAT4& rectangle, UINT nColor = 0x00FFFFFF);
	void RenderInputPage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		float fViewportWidth, float fViewportHeight);
	void RenderLoadingPage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		float fViewportWidth, float fViewportHeight);
	void RenderRegisterPage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		float fViewportWidth, float fViewportHeight);
	void RenderTextField(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		int nFieldIndex, const XMFLOAT4& rectangle);
	GLYPH_RESOURCE* FindGlyph(char ch);
	float GetGlyphAdvance(char ch, float scale) const;
	float MeasureText(const std::string& text, size_t characterCount, float scale) const;
	void ResetCursorBlink();
	void MoveCursorFromClick(int nFieldIndex, float x, const XMFLOAT4& rectangle);
	void SpawnLoginErrorLog(float fViewportWidth, float fViewportHeight, int nType = 0);
	void EnterLoadingPage();
	void ReturnToInputPageWithLoginFailure(float fViewportWidth, float fViewportHeight);
	void EnterRegisterPage();
	void ReturnToInputPageWithRegisterResult(float fViewportWidth, float fViewportHeight,
		REGISTER_RESULT eResult);
	void LoadSavedAccountInformation();
	void SaveAccountInformation() const;
};
