#pragma once

#include "Shader.h"

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

struct SHOP_TEXT_RENDER_CONTEXT
{
	ID3D12PipelineState* pTextPipelineState = NULL;
	ID3D12PipelineState* pSolidUiPipelineState = NULL;
	std::vector<TEXT_GLYPH_RESOURCE>* pGlyphResources = NULL;
};

struct SHOP_PET_RENDER_RESOURCE
{
	CPet* pPet = NULL;
	ID3D12DescriptorHeap* pd3dSrvDescriptorHeap = NULL;
};

class CShopUI
{
public:
	void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
		ID3D12RootSignature* pd3dRootSignature, size_t nInitialPetCount);
	void ReleaseObjects();
	void ReleaseUploadBuffers();
	void Animate(float fElapsedTime);

	void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, UINT nMoney,
		size_t nActivePetIndex, const std::vector<SHOP_PET_RENDER_RESOURCE>& pets,
		const SHOP_TEXT_RENDER_CONTEXT& textContext);
	bool IsPointOver(float x, float y, float fViewportWidth, float fViewportHeight) const;
	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam,
		UINT nMoney, size_t nPetCount, size_t nActivePetIndex,
		const SHOP_TEXT_RENDER_CONTEXT& textContext);
	bool ConsumePetConfirmationRequest(size_t& nSelectedPetIndex);
	bool ConsumePetEnhancementRequest(int& nEnhancementType);

private:
	struct UI_IMAGE_RESOURCE
	{
		ID3D12Resource* pd3dTexture = NULL;
		ID3D12Resource* pd3dTextureUploadBuffer = NULL;
		ID3D12DescriptorHeap* pd3dSrvDescriptorHeap = NULL;
	};

	enum class SHOP_PAGE
	{
		SLOT_MENU,
		SLOT_CONTENT_1,
		SLOT_CONTENT_2,
		SLOT_CONTENT_3,
		SLOT_CONTENT_4
	};

	ID3D12PipelineState* m_pd3dUiImagePipelineState = NULL;
	UI_IMAGE_RESOURCE m_ShopIconResource;
	UI_IMAGE_RESOURCE m_ShopBoardResource;
	UI_IMAGE_RESOURCE m_ShopCloseIconResource;
	UI_IMAGE_RESOURCE m_ShopBackSpaceIconResource;
	UI_IMAGE_RESOURCE m_ShopSlotResources[4];
	UI_IMAGE_RESOURCE m_EmptySquareResources[2];
	UI_IMAGE_RESOURCE m_PetConfirmationButtonResource;
	UI_IMAGE_RESOURCE m_ScrollBackgroundResource;
	UI_IMAGE_RESOURCE m_ScrollResource;
	UI_IMAGE_RESOURCE m_EmptyFrameResource;
	UI_IMAGE_RESOURCE m_PetEnhanceButtonResource;
	UI_IMAGE_RESOURCE m_PetEnhanceLogResources[2];

	bool m_bShopActive = false;
	SHOP_PAGE m_eShopPage = SHOP_PAGE::SLOT_MENU;
	int m_nSelectedShopSlot = -1;
	bool m_bShopBoardDragging = false;
	bool m_bResetShopPositionOnNextOpen = false;
	XMFLOAT2 m_xmf2ShopBoardOffset = XMFLOAT2(0.0f, 0.0f);
	XMFLOAT2 m_xmf2ShopDragLastCursor = XMFLOAT2(0.0f, 0.0f);
	size_t m_nPetScrollOffset = 0;
	size_t m_nCachedPetCount = 0;
	size_t m_nMaximumPetScrollOffset = 0;
	float m_fPetScrollThumbRatio = 1.0f;
	bool m_bPetScrollDragging = false;
	float m_fPetScrollDragLastY = 0.0f;
	size_t m_nSelectedPetIndex = 0;
	size_t m_nPendingConfirmedPetIndex = static_cast<size_t>(-1);
	size_t m_nPreviewPetIndex = static_cast<size_t>(-1);
	CPet* m_pPreviewPet = NULL;
	CCamera m_PreviewPetCamera;
	int m_nPressedEnhanceButton = -1;
	int m_nPendingEnhancementType = -1;

	void RenderUiImage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		UI_IMAGE_RESOURCE& imageResource, const XMFLOAT4& rectangle, UINT nTintColor = 0x00FFFFFF);
	void RenderMoneyUI(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, UINT nMoney,
		const SHOP_TEXT_RENDER_CONTEXT& textContext);
	void RenderSolidUiRectangle(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		float fLeft, float fTop, float fRight, float fBottom, UINT nColor,
		const SHOP_TEXT_RENDER_CONTEXT& textContext);
	void RenderTextLine(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		const std::string& text, float fLeft, float fTop, float fGlyphScale, UINT nColor,
		const SHOP_TEXT_RENDER_CONTEXT& textContext);
	XMFLOAT4 GetMoneyUiRectangle(float fViewportWidth, float fViewportHeight, UINT nMoney,
		const SHOP_TEXT_RENDER_CONTEXT& textContext) const;
	bool ProcessShopUIClick(float x, float y, float fViewportWidth, float fViewportHeight,
		UINT nMoney, size_t nPetCount, size_t nActivePetIndex,
		const SHOP_TEXT_RENDER_CONTEXT& textContext);
	void DeactivateShop(float fViewportWidth, float fViewportHeight, size_t nActivePetIndex);
	void ResetSelectedPet(size_t nActivePetIndex, size_t nPetCount);
	void RebuildPetScrollMetrics(size_t nPetCount);
	void RebuildPetScrollMetricsIfNeeded(size_t nPetCount);
	XMFLOAT4 GetPetScrollThumbRectangle(float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetPetListRowRectangle(size_t nRow, float fViewportWidth, float fViewportHeight) const;
	void EnsurePreviewPet(const std::vector<SHOP_PET_RENDER_RESOURCE>& pets);
	void RenderPreviewPet(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pMainCamera,
		const XMFLOAT4& panel, const std::vector<SHOP_PET_RENDER_RESOURCE>& pets);
	void RenderEnhancementPage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		CPet* pActivePet, const SHOP_TEXT_RENDER_CONTEXT& textContext);
	XMFLOAT4 GetEnhanceButtonRectangle(int nType, float fViewportWidth, float fViewportHeight) const;
};
