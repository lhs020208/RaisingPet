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

struct SHOP_STOCK_HOLDER_INFO
{
	std::wstring wstrPlayerId;
	UINT nQuantity = 0;
};

struct SHOP_STOCK_PRICE_INFO
{
	UINT nPreviousPrice = 0;
	UINT nNewPrice = 0;
	UINT nBoughtQuantity = 0;
	UINT nSoldQuantity = 0;
	std::wstring wstrChangedTime;
};

struct SHOP_STOCK_MANAGEMENT_INFO
{
	bool bIssued = false;
	std::wstring wstrStockName;
	UINT nSoldQuantity = 0;
	UINT nUnsoldQuantity = 0;
	UINT nSaleableQuantity = 800;
	UINT nIssuanceRevenue = 0;
	UINT nRecentTradeQuantity = 0;
	UINT nCurrentPrice = 100;
	UINT nPreviousPrice = 100;
	SHOP_STOCK_HOLDER_INFO TopHolders[3];
	std::vector<SHOP_STOCK_PRICE_INFO> RecentPrices;
};

struct SHOP_STOCK_TRANSACTION_INFO
{
	UINT nStockId = 0;
	std::wstring wstrStockName;
	std::wstring wstrIssuerId;
	UINT nCurrentPrice = 100;
	UINT nPreviousPrice = 100;
	UINT nSaleableQuantity = 0;
	UINT nMyQuantity = 0;
	UINT nRecentTradeQuantity = 0;
	std::vector<SHOP_STOCK_PRICE_INFO> RecentPrices;
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
	bool IsPointOverClickableButton(float x, float y, float fViewportWidth, float fViewportHeight,
		UINT nMoney, size_t nPetCount, size_t nActivePetIndex, CPet* pActivePet,
		const SHOP_TEXT_RENDER_CONTEXT& textContext, bool bNetworkConnected) const;
	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam,
		UINT nMoney, size_t nPetCount, size_t nActivePetIndex, CPet* pActivePet,
		const SHOP_TEXT_RENDER_CONTEXT& textContext, bool bNetworkConnected);
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	bool ConsumePetConfirmationRequest(size_t& nSelectedPetIndex);
	bool ConsumePetEnhancementRequest(int& nEnhancementType);
	bool ConsumeFinancialProductRequest(int& nCategory, int& nProductIndex);
	bool ConsumeStockIssueRequest(std::wstring& stockName);
	bool ConsumeStockManagementInfoRequest();
	bool ConsumeStockTransactionListRequest();
	bool ConsumeStockTradeRequest(int& nAction, UINT& nStockId, UINT& nQuantity);
	bool ConsumeLoginSceneReturnRequest();
	void SetFinancialProductActive(int nCategory, int nProductIndex, UINT nDurationSeconds);
	void ClearFinancialProductActive(int nCategory, int nProductIndex);
	void SetFinancialMaximumProductIndex(int nCategory, int nProductIndex);
	void SetFinancialProgressCount(int nCategory, int nProgressCount);
	void NotifyFinancialApplicationFailed(int nCategory);
	void SetStockIssued(bool bIssued, const std::wstring& stockName = std::wstring());
	void NotifyStockIssueFailed();
	void NotifyStockTradeFailed(int nAction);
	void SetStockManagementInfo(const SHOP_STOCK_MANAGEMENT_INFO& info);
	void SetStockTransactionInfos(const std::vector<SHOP_STOCK_TRANSACTION_INFO>& infos);
	bool IsStockIssued() const { return m_bStockIssued; }
	const std::wstring& GetStockName() const { return m_wstrStockName; }
	bool IsPetMovementEnabled() const { return m_bSettingPetOptions[0]; }
	bool IsPetDragEnabled() const { return m_bSettingPetOptions[1]; }
	bool IsCoinEffectEnabled() const { return m_bSettingPetOptions[2]; }
	float GetMasterVolumeScale() const;
	float GetClickVolumeScale() const;
	float GetErrorVolumeScale() const;
	float GetCoinVolumeScale() const;
	float GetPetSizeScale() const;
	void GetSettingValues(bool bPetOptions[3], UINT& nPetSizePercent, UINT nVolumePercents[4]) const;
	void SetSettingValues(const bool bPetOptions[3], UINT nPetSizePercent, const UINT nVolumePercents[4]);
	bool ConsumeSettingChangeRequest();

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
		SHOP_MENU,
		PET_CHANGE,
		PET_ENHANCE,
		BANK,
		STOCK_MENU,
		STOCK_TRANSACTION,
		STOCK_CANT_PUBLISH,
		STOCK_MANAGEMENT,
		STOCK_SEE_MYGRAPH,
		STOCK_SEE_TARGET_GRAPH
	};

	ID3D12PipelineState* m_pd3dUiImagePipelineState = NULL;
	UI_IMAGE_RESOURCE m_ShopIconResource;
	UI_IMAGE_RESOURCE m_SettingIconResource;
	UI_IMAGE_RESOURCE m_SettingsResource;
	UI_IMAGE_RESOURCE m_SettingCheckBoxResource;
	UI_IMAGE_RESOURCE m_SettingCheckResource;
	UI_IMAGE_RESOURCE m_SettingSliderBarResource;
	UI_IMAGE_RESOURCE m_SettingSliderHandleResource;
	UI_IMAGE_RESOURCE m_ShopBoardResource;
	UI_IMAGE_RESOURCE m_PageTitleResource;
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
	UI_IMAGE_RESOURCE m_FinancialApplicationButtonResource;
	UI_IMAGE_RESOURCE m_FinancialFailLogResources[2];
	UI_IMAGE_RESOURCE m_InternetOnIconResource;
	UI_IMAGE_RESOURCE m_InternetOffIconResource;
	UI_IMAGE_RESOURCE m_NetworkErrorLogResource;
	UI_IMAGE_RESOURCE m_StockSlotResources[2];
	UI_IMAGE_RESOURCE m_StockLimitResources[2];
	UI_IMAGE_RESOURCE m_CantCreateStockGenLogResource;
	UI_IMAGE_RESOURCE m_StockNameResource;
	UI_IMAGE_RESOURCE m_StockHoldersResource;
	UI_IMAGE_RESOURCE m_StockManagementTableResource;
	UI_IMAGE_RESOURCE m_StockChartResource;
	UI_IMAGE_RESOURCE m_MyGraphResource;
	UI_IMAGE_RESOURCE m_TargetGraphResource;
	UI_IMAGE_RESOURCE m_SeeGraphResource;
	UI_IMAGE_RESOURCE m_StockUpMarkResource;
	UI_IMAGE_RESOURCE m_StockDownMarkResource;
	UI_IMAGE_RESOURCE m_IssuanceStockResource;
	UI_IMAGE_RESOURCE m_IssuanceStockErrorLogResource;
	UI_IMAGE_RESOURCE m_StockTransactionTitleResource;
	UI_IMAGE_RESOURCE m_StockTransactionIssuerResource;
	UI_IMAGE_RESOURCE m_StockTransactionGraphResource;
	UI_IMAGE_RESOURCE m_StockTransactionDescriptionResource;
	UI_IMAGE_RESOURCE m_SeeStockResource;
	UI_IMAGE_RESOURCE m_StockBuyingResource;
	UI_IMAGE_RESOURCE m_StockSellingResource;
	UI_IMAGE_RESOURCE m_StockBuyingFailLogResource;
	UI_IMAGE_RESOURCE m_StockSellingFailLogResource;
	UI_IMAGE_RESOURCE m_StockReceiptResource;
	UI_IMAGE_RESOURCE m_TextCursorResource;

	bool m_bShopActive = false;
	SHOP_PAGE m_eShopPage = SHOP_PAGE::SHOP_MENU;
	int m_nSelectedShopSlot = -1;
	bool m_bShopBoardDragging = false;
	bool m_bSettingActive = false;
	bool m_bSettingBoardOnTop = false;
	bool m_bSettingBoardDragging = false;
	bool m_bBlockShopDirectWriteText = false;
	bool m_bBlockSettingDirectWriteText = false;
	bool m_bResetSettingPositionOnNextOpen = false;
	bool m_bResetShopPositionOnNextOpen = false;
	XMFLOAT2 m_xmf2ShopBoardOffset = XMFLOAT2(0.0f, 0.0f);
	XMFLOAT2 m_xmf2SettingBoardOffset = XMFLOAT2(0.0f, 0.0f);
	XMFLOAT2 m_xmf2ShopDragLastCursor = XMFLOAT2(0.0f, 0.0f);
	XMFLOAT2 m_xmf2SettingDragLastCursor = XMFLOAT2(0.0f, 0.0f);
	XMFLOAT4 m_xmf4ShopDirectWriteBlockRectangle = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	XMFLOAT4 m_xmf4SettingDirectWriteBlockRectangle = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	bool m_bSettingPetOptions[3] = { true, true, true };
	UINT m_nSettingPetSizePercent = 50;
	UINT m_nSettingVolumePercents[4] = { 100, 100, 100, 100 };
	bool m_bPendingSettingChangeRequest = false;
	int m_nDraggingSettingSlider = -1;
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
	bool m_bPendingFinancialFailLog[2] = { false, false };
	bool m_bStockCreationAvailable = false;
	std::wstring m_wstrStockName;
	size_t m_nStockNameCursorIndex = 0;
	bool m_bStockNameInputActive = false;
	float m_fStockNameCursorBlinkElapsed = 0.0f;
	bool m_bStockIssueButtonPressed = false;
	bool m_bStockIssued = false;
	bool m_bPendingStockIssueRequest = false;
	bool m_bPendingStockIssueErrorLog = false;
	bool m_bPendingStockManagementInfoRequest = false;
	bool m_bPendingStockTransactionListRequest = false;
	std::wstring m_wstrPendingStockIssueName;
	SHOP_STOCK_MANAGEMENT_INFO m_StockManagementInfo;
	std::vector<SHOP_STOCK_TRANSACTION_INFO> m_StockTransactionInfos;
	size_t m_nStockTransactionScrollOffset = 0;
	size_t m_nMaximumStockTransactionScrollOffset = 0;
	size_t m_nSelectedStockTransactionIndex = 0;
	UINT m_nStockTransactionOrderQuantity = 0;
	bool m_bStockTransactionQuantityInputActive = false;
	float m_fStockTransactionQuantityCursorBlinkElapsed = 0.0f;
	int m_nPendingStockTradeAction = -1;
	UINT m_nPendingStockTradeStockId = 0;
	UINT m_nPendingStockTradeQuantity = 0;
	bool m_bPendingStockTradeFailLog[2] = { false, false };
	std::vector<SHOP_NETWORK_ERROR_LOG> m_NetworkErrorLogs;
	std::vector<SHOP_NETWORK_ERROR_LOG> m_FinancialFailLogs[2];
	std::vector<SHOP_NETWORK_ERROR_LOG> m_StockIssueErrorLogs;
	std::vector<SHOP_NETWORK_ERROR_LOG> m_StockTradeFailLogs[2];

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
	void QueueShopDirectWriteText(const std::wstring& text, const XMFLOAT4& rectangle,
		float fFontSize, UINT nColor = 0xFF000000,
		bool bHorizontalCenter = false, bool bVerticalCenter = true) const;
	bool IsShopDirectWriteTextBlocked(const XMFLOAT4& rectangle) const;
	void QueueSettingDirectWriteText(const std::wstring& text, const XMFLOAT4& rectangle,
		float fFontSize, UINT nColor = 0xFF000000,
		bool bHorizontalCenter = false, bool bVerticalCenter = true) const;
	bool IsSettingDirectWriteTextBlocked(const XMFLOAT4& rectangle) const;
	XMFLOAT4 GetMoneyUiRectangle(float fViewportWidth, float fViewportHeight, UINT nMoney,
		const SHOP_TEXT_RENDER_CONTEXT& textContext) const;
	bool ProcessShopUIClick(float x, float y, float fViewportWidth, float fViewportHeight,
		UINT nMoney, size_t nPetCount, size_t nActivePetIndex,
		const SHOP_TEXT_RENDER_CONTEXT& textContext, bool bNetworkConnected);
	void SpawnNetworkErrorLog(float fViewportWidth, float fViewportHeight, int nSlotIndex);
	void SpawnNetworkErrorLogAtNetworkIcon(float fViewportWidth, float fViewportHeight);
	bool IsNetworkRequiredPage() const;
	void ReturnToShopMenuAfterNetworkDisconnected(float fViewportWidth, float fViewportHeight);
	void RenderStockIssueErrorLogs(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
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
	UINT GetNextEnhancementValue(UINT nValue, int nType) const;
	UINT GetEnhancementPrice(UINT nValue, int nType) const;
	bool IsEnhanceButtonDisabled(CPet* pActivePet, UINT nMoney, int nType) const;
	XMFLOAT4 GetEnhanceButtonRectangle(int nType, float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetEnhancePriceRectangle(int nType, float fViewportWidth, float fViewportHeight) const;
	void RenderFinancialPage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		UINT nMoney, const SHOP_TEXT_RENDER_CONTEXT& textContext);
	bool ProcessFinancialClick(float x, float y, float fViewportWidth, float fViewportHeight,
		UINT nMoney, const SHOP_TEXT_RENDER_CONTEXT& textContext);
	bool IsFinancialApplicationButtonDisabled(UINT nMoney) const;
	void RenderFinancialFailLogs(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		UINT nMoney, const SHOP_TEXT_RENDER_CONTEXT& textContext);
	void RenderStockMenuPage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	void RenderStockTransactionPage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		UINT nMoney, const SHOP_TEXT_RENDER_CONTEXT& textContext);
	void RenderStockManagementPage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	void RenderStockGraphPage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		UINT nMoney, const SHOP_TEXT_RENDER_CONTEXT& textContext);
	void RenderStockTradeFailLogs(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		UINT nMoney, const SHOP_TEXT_RENDER_CONTEXT& textContext);
	void RenderStockQuantityCursor(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		const XMFLOAT4& rectangle, UINT nQuantity, float fFontSize);
	void RenderPageTitle(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	void RenderSettingBoard(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	bool ProcessSettingBoardClick(HWND hWnd, float x, float y, float fViewportWidth, float fViewportHeight);
	void UpdateSettingSliderFromCursor(int nSliderIndex, float x, float fViewportWidth, float fViewportHeight);
	float GetSettingSliderRatio(int nSliderIndex) const;
	UINT GetSettingSliderPercent(int nSliderIndex) const;
	XMFLOAT4 GetStockChartRectangle(float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetStockGraphButtonRectangle(float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetStockNameInputRectangle(float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetStockIssuanceButtonRectangle(float fViewportWidth, float fViewportHeight) const;
	float GetStockNameInputFontSize(float fViewportWidth, float fViewportHeight) const;
	float MeasureStockNameTextWidth(const std::wstring& text, float fFontSize, float fAvailableWidth,
		float fAvailableHeight) const;
	void MoveStockNameCursorFromClick(float x, const XMFLOAT4& rectangle, float fFontSize);
	void RenderCantCreateStockPage(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera,
		const SHOP_TEXT_RENDER_CONTEXT& textContext);
	bool ProcessStockMenuClick(float x, float y, float fViewportWidth, float fViewportHeight);
	XMFLOAT4 GetFinancialCategoryButtonRectangle(int nCategory, float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetFinancialLeftButtonRectangle(float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetFinancialRightButtonRectangle(float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetFinancialApplicationButtonRectangle(float fViewportWidth, float fViewportHeight,
		UINT nMoney, const SHOP_TEXT_RENDER_CONTEXT& textContext) const;
	XMFLOAT4 GetStockSlotRectangle(int nIndex, float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetStockTransactionListRowRectangle(size_t nRow, float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetStockTransactionScrollThumbRectangle(float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetStockTransactionQuantityRectangle(float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetStockTransactionGraphButtonRectangle(float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetStockTransactionBuyingButtonRectangle(float fViewportWidth, float fViewportHeight,
		const SHOP_TEXT_RENDER_CONTEXT& textContext) const;
	XMFLOAT4 GetStockTransactionSellingButtonRectangle(float fViewportWidth, float fViewportHeight,
		const SHOP_TEXT_RENDER_CONTEXT& textContext) const;
	XMFLOAT4 GetStockTargetReceiptRectangle(float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetStockTargetQuantityRectangle(float fViewportWidth, float fViewportHeight) const;
	XMFLOAT4 GetStockTargetBuyingButtonRectangle(float fViewportWidth, float fViewportHeight,
		const SHOP_TEXT_RENDER_CONTEXT& textContext) const;
	XMFLOAT4 GetStockTargetSellingButtonRectangle(float fViewportWidth, float fViewportHeight,
		const SHOP_TEXT_RENDER_CONTEXT& textContext) const;
	bool IsStockTradeButtonDisabled(int nAction, UINT nMoney) const;
	void QueueStockTradeRequest(int nAction);
	void PlayUiClickSound() const;
	void PlayUiErrorSound() const;
};
