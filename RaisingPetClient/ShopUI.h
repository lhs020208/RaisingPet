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
		const SHOP_TEXT_RENDER_CONTEXT& textContext, bool bNetworkConnected);
	bool IsPointOver(float x, float y, float fViewportWidth, float fViewportHeight) const;
	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam,
		UINT nMoney, size_t nPetCount, size_t nActivePetIndex,
		const SHOP_TEXT_RENDER_CONTEXT& textContext, bool bNetworkConnected);
	bool ConsumePetConfirmationRequest(size_t& nSelectedPetIndex);
	bool ConsumePetEnhancementRequest(int& nEnhancementType);
	bool ConsumeFinancialProductRequest(int& nCategory, int& nProductIndex);
	bool ConsumeLoginSceneReturnRequest();
	void SetFinancialProductActive(int nCategory, int nProductIndex, UINT nDurationSeconds);
	void ClearFinancialProductActive(int nCategory, int nProductIndex);
	void SetFinancialMaximumProductIndex(int nCategory, int nProductIndex);
	void SetFinancialProgressCount(int nCategory, int nProgressCount);

private:
	struct UI_IMAGE_RESOURCE
	{
		ID3D12Resource* pd3dTexture = NULL;
		ID3D12Resource* pd3dTextureUploadBuffer = NULL;
		ID3D12DescriptorHeap* pd3dSrvDescriptorHeap = NULL;
	};

	struct SHOP_NETWORK_ERROR_LOG
	{
		XMFLOAT4 rectangle = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
		float fElapsedTime = 0.0f;
	};

	enum class SHOP_PAGE
	{
		SLOT_MENU,
		SLOT_CONTENT_1,
		SLOT_CONTENT_2,
		SLOT_CONTENT_3,
		SLOT_CONTENT_4,
		STOCK_CONTENT_1,
		STOCK_CONTENT_2,
		STOCK_CONTENT_3
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
	UI_IMAGE_RESOURCE m_PetEnhancePriceFrameResource;
	UI_IMAGE_RESOURCE m_PetEnhanceLogResources[2];
	UI_IMAGE_RESOURCE m_BankFrameResource;
	UI_IMAGE_RESOURCE m_FinancialCategoryButtonResources[2];
	UI_IMAGE_RESOURCE m_FinancialProductNameResources[2][10];
	UI_IMAGE_RESOURCE m_FinancialLeftButtonResource;
	UI_IMAGE_RESOURCE m_FinancialRightButtonResource;
	UI_IMAGE_RESOURCE m_FinancialTimerFrameResource;
	UI_IMAGE_RESOURCE m_FinancialMoneyFrameResource;
	UI_IMAGE_RESOURCE m_FinancialRightPointResource;
	UI_IMAGE_RESOURCE m_InternetOnIconResource;
	UI_IMAGE_RESOURCE m_InternetOffIconResource;
	UI_IMAGE_RESOURCE m_NetworkErrorLogResource;
	UI_IMAGE_RESOURCE m_StockSlotResources[2];
	UI_IMAGE_RESOURCE m_StockLimitResources[2];
	UI_IMAGE_RESOURCE m_CantCreateStockGenLogResource;

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
	bool m_bPendingLoginSceneReturnRequest = false;
	int m_nFinancialCategory = 0;
	int m_nFinancialProductIndices[2] = { 0, 0 };
	int m_nFinancialMaximumProductIndices[2] = { 0, 0 };
	int m_nFinancialProgressCounts[2] = { 0, 0 };
	int m_nPendingFinancialCategory = -1;
	int m_nPendingFinancialProductIndex = -1;
	bool m_bFinancialProductActive[2] = { false, false };
	int m_nActiveFinancialProductIndex[2] = { -1, -1 };
	UINT m_nActiveFinancialDurationSeconds[2] = { 0, 0 };
	float m_fActiveFinancialElapsedSeconds[2] = { 0.0f, 0.0f };
	bool m_bStockCreationAvailable = false;
	std::vector<SHOP_NETWORK_ERROR_LOG> m_NetworkErrorLogs;

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
		const SHOP_TEXT_RENDER_CONTEXT& textContext, bool bNetworkConnected);
	void SpawnNetworkErrorLog(float fViewportWidth, float fViewportHeight, int nSlotIndex);
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
		CPet* pActivePet, UINT nMoney, const SHOP_TEXT_RENDER_CONTEXT& textContext);
	XMFLOAT4 GetEnhanceButtonRectangle(int nType, float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetEnhancePriceRectangle(int nType, float fViewportWidth, float fViewportHeight) const;
	void RenderFinancialPage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		const SHOP_TEXT_RENDER_CONTEXT& textContext);
	bool ProcessFinancialClick(float x, float y, float fViewportWidth, float fViewportHeight);
	void RenderStockMenuPage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	void RenderCantCreateStockPage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		const SHOP_TEXT_RENDER_CONTEXT& textContext);
	bool ProcessStockMenuClick(float x, float y, float fViewportWidth, float fViewportHeight);
	XMFLOAT4 GetFinancialCategoryButtonRectangle(int nCategory, float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetFinancialLeftButtonRectangle(float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetFinancialRightButtonRectangle(float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetStockSlotRectangle(int nIndex, float fViewportWidth, float fViewportHeight) const;
};
