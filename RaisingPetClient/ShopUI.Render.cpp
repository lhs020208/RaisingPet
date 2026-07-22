// top-level shop rendering for ShopUI.cpp
// This file is intentionally included by ShopUI.cpp to keep helper functions in one translation unit.

void CShopUI::Render(ID3D12GraphicsCommandList* commandList, CCamera* camera, UINT money,
	size_t activePetIndex, const std::vector<SHOP_PET_RENDER_RESOURCE>& pets,
	const SHOP_TEXT_RENDER_CONTEXT& context, bool networkConnected)
{
	if (!camera) return;
	RebuildPetScrollMetricsIfNeeded(pets.size());
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	if (m_bShopActive && !networkConnected && IsNetworkRequiredPage())
		ReturnToShopMenuAfterNetworkDisconnected(width, height);
	m_bBlockShopDirectWriteText = m_bSettingActive && m_bSettingBoardOnTop;
	m_xmf4ShopDirectWriteBlockRectangle = m_bBlockShopDirectWriteText
		? GetShopBoardRectangle(width, height,
			m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y)
		: XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	auto renderSettingBoard = [&]()
	{
		RenderUiImage(commandList, camera, m_ShopBoardResource,
			GetShopBoardRectangle(width, height,
				m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y));
		RenderUiImage(commandList, camera, m_ShopCloseIconResource,
			GetShopCloseRectangle(width, height,
				m_xmf2SettingBoardOffset.x, m_xmf2SettingBoardOffset.y));
	};
	if (m_bSettingActive && !m_bSettingBoardOnTop)
		renderSettingBoard();
	if (m_bShopActive)
	{
		RenderUiImage(commandList, camera, m_ShopBoardResource,
			GetShopBoardRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
		RenderPageTitle(commandList, camera);
		if (m_eShopPage == SHOP_PAGE::SHOP_MENU)
		{
			for (int i = 0; i < 4; ++i)
			{
				const UINT slotTint = (!networkConnected && (i == 2 || i == 3))
					? 0x00BFBFBF : 0x00FFFFFF;
				RenderUiImage(commandList, camera, m_ShopSlotResources[i],
					GetShopSlotRectangle(i, width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y),
					slotTint);
			}
			RenderUiImage(commandList, camera,
				networkConnected ? m_InternetOnIconResource : m_InternetOffIconResource,
				GetShopNetworkIconRectangle(width, height,
					m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
			for (const SHOP_NETWORK_ERROR_LOG& log : m_NetworkErrorLogs)
			{
				const float alphaRatio = max(0.0f, 1.0f - log.fElapsedTime);
				const UINT alpha = max(1u, static_cast<UINT>(alphaRatio * 255.0f + 0.5f));
				const float moveY = -20.0f * log.fElapsedTime;
				RenderUiImage(commandList, camera, m_NetworkErrorLogResource,
					XMFLOAT4(log.rectangle.x, log.rectangle.y + moveY,
						log.rectangle.z, log.rectangle.w + moveY),
					(alpha << 24) | 0x00FFFFFF);
			}
		}
		else if (m_eShopPage == SHOP_PAGE::PET_CHANGE)
		{
			const XMFLOAT4 leftPanel = GetPetContentPanelRectangle(false, width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
			const XMFLOAT4 rightPanel = GetPetContentPanelRectangle(true, width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
			RenderUiImage(commandList, camera, m_EmptySquareResources[0], leftPanel);
			RenderUiImage(commandList, camera, m_EmptySquareResources[1], rightPanel);
			RenderPreviewPet(commandList, camera, rightPanel, pets);
			RenderUiImage(commandList, camera, m_PetConfirmationButtonResource,
				GetPetConfirmationRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
			const XMFLOAT4 scrollTrack = GetPetScrollTrackRectangle(width, height,
				m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
			RenderUiImage(commandList, camera, m_ScrollBackgroundResource, scrollTrack);
			RenderUiImage(commandList, camera, m_ScrollResource,
				GetPetScrollThumbRectangle(width, height));
			const float rowHeight = (leftPanel.w - leftPanel.y) / 10.0f;
			const float glyphScale = rowHeight / 320.0f;
			for (size_t row = 0; row < 10; ++row)
			{
				const size_t petIndex = m_nPetScrollOffset + row;
				if (petIndex >= pets.size() || !pets[petIndex].pPet) continue;
				if (petIndex == m_nSelectedPetIndex)
				{
					const XMFLOAT4 selection = GetPetListRowRectangle(row, width, height);
					RenderSolidUiRectangle(commandList, camera, selection.x, selection.y,
						selection.z, selection.w, 0x00BFBFBF, context);
				}
				const std::string name = std::to_string(petIndex + 1) + ". " + pets[petIndex].pPet->GetName();
				RenderTextLine(commandList, camera, name, leftPanel.x + 18.0f,
					leftPanel.y + rowHeight * row + rowHeight * 0.15f, glyphScale, 0x00000000, context);
			}
		}
		else if (m_eShopPage == SHOP_PAGE::PET_ENHANCE)
		{
			CPet* activePet = (activePetIndex < pets.size()) ? pets[activePetIndex].pPet : NULL;
			RenderEnhancementPage(commandList, camera, activePet, money, context);
		}
		else if (m_eShopPage == SHOP_PAGE::BANK)
		{
			RenderFinancialPage(commandList, camera, money, context);
		}
		else if (m_eShopPage == SHOP_PAGE::STOCK_MENU)
		{
			RenderStockMenuPage(commandList, camera);
		}
		else if (m_eShopPage == SHOP_PAGE::STOCK_TRANSACTION)
		{
			RenderStockTransactionPage(commandList, camera, money, context);
			RenderStockTradeFailLogs(commandList, camera, money, context);
		}
		else if (m_eShopPage == SHOP_PAGE::STOCK_MANAGEMENT)
		{
			RenderStockManagementPage(commandList, camera);
			RenderStockIssueErrorLogs(commandList, camera);
		}
		else if (m_eShopPage == SHOP_PAGE::STOCK_CANT_PUBLISH)
		{
			RenderCantCreateStockPage(commandList, camera, context);
		}
		else if (m_eShopPage == SHOP_PAGE::STOCK_SEE_MYGRAPH
			|| m_eShopPage == SHOP_PAGE::STOCK_SEE_TARGET_GRAPH)
		{
			RenderStockGraphPage(commandList, camera, money, context);
			if (m_eShopPage == SHOP_PAGE::STOCK_SEE_TARGET_GRAPH)
				RenderStockTradeFailLogs(commandList, camera, money, context);
		}
		RenderMoneyUI(commandList, camera, money, context);
		RenderUiImage(commandList, camera, m_ShopBackSpaceIconResource,
			GetShopBackRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
		RenderUiImage(commandList, camera, m_ShopCloseIconResource,
			GetShopCloseRectangle(width, height, m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y));
	}
	if (m_bSettingActive && m_bSettingBoardOnTop)
		renderSettingBoard();
	m_bBlockShopDirectWriteText = false;
	RenderUiImage(commandList, camera, m_SettingIconResource, GetSettingIconRectangle(width, height));
	RenderUiImage(commandList, camera, m_ShopIconResource, GetShopIconRectangle(width, height));
}

