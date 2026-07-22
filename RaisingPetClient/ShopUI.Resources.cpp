// resource, primitive UI rendering, preview, and shared rectangle helpers for ShopUI.cpp
// This file is intentionally included by ShopUI.cpp to keep helper functions in one translation unit.

void CShopUI::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	ID3D12RootSignature* rootSignature, size_t nInitialPetCount)
{
	RebuildPetScrollMetrics(nInitialPetCount);
	ID3DBlob* vertexShader = NULL;
	ID3DBlob* pixelShader = NULL;
	ID3DBlob* errorBlob = NULL;
	HRESULT result = D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "VSTextGlyph", "vs_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &vertexShader, &errorBlob);
	if (FAILED(result)) return;
	if (errorBlob) { errorBlob->Release(); errorBlob = NULL; }
	result = D3DCompileFromFile(L"Shaders.hlsl", NULL, NULL, "PSUiImage", "ps_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, &pixelShader, &errorBlob);
	if (FAILED(result)) { vertexShader->Release(); return; }
	if (errorBlob) errorBlob->Release();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
	pipelineDesc.pRootSignature = rootSignature;
	pipelineDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	pipelineDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
	CShader stateFactory;
	pipelineDesc.RasterizerState = stateFactory.CreateRasterizerState();
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	pipelineDesc.BlendState = stateFactory.CreateBlendState();
	pipelineDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	pipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	pipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	pipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	pipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	pipelineDesc.DepthStencilState = stateFactory.CreateDepthStencilState();
	pipelineDesc.DepthStencilState.DepthEnable = FALSE;
	pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	pipelineDesc.SampleMask = UINT_MAX;
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.NumRenderTargets = 1;
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
	pipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	pipelineDesc.SampleDesc.Count = 1;
	result = device->CreateGraphicsPipelineState(&pipelineDesc, __uuidof(ID3D12PipelineState),
		reinterpret_cast<void**>(&m_pd3dUiImagePipelineState));
	vertexShader->Release();
	pixelShader->Release();
	if (FAILED(result)) return;

	auto loadImage = [device, commandList](const wchar_t* fileName, UI_IMAGE_RESOURCE& image) -> bool
	{
		std::unique_ptr<uint8_t[]> data;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		HRESULT hr = DirectX::LoadDDSTextureFromFile(device, fileName, &image.pd3dTexture, data, subresources);
		if (FAILED(hr) || subresources.empty()) return(false);
		const UINT64 uploadSize = GetRequiredIntermediateSize(image.pd3dTexture, 0,
			static_cast<UINT>(subresources.size()));
		CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);
		hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, NULL, __uuidof(ID3D12Resource),
			reinterpret_cast<void**>(&image.pd3dTextureUploadBuffer));
		if (FAILED(hr)) return(false);
		UpdateSubresources(commandList, image.pd3dTexture, image.pd3dTextureUploadBuffer,
			0, 0, static_cast<UINT>(subresources.size()), subresources.data());
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(image.pd3dTexture,
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->ResourceBarrier(1, &barrier);
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		hr = device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap),
			reinterpret_cast<void**>(&image.pd3dSrvDescriptorHeap));
		if (FAILED(hr)) return(false);
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = image.pd3dTexture->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = image.pd3dTexture->GetDesc().MipLevels;
		device->CreateShaderResourceView(image.pd3dTexture, &srvDesc,
			image.pd3dSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		return(true);
	};

	loadImage(L"Assets/Image/Shop/ShopIcon.dds", m_ShopIconResource);
	loadImage(L"Assets/Image/Shop/ShopBoard.dds", m_ShopBoardResource);
	loadImage(L"Assets/Image/Shop/PageTitle.dds", m_PageTitleResource);
	loadImage(L"Assets/Image/Shop/ShopCloseIcon.dds", m_ShopCloseIconResource);
	loadImage(L"Assets/Image/Shop/ShopBackSpaceIcon.dds", m_ShopBackSpaceIconResource);
	loadImage(L"Assets/Image/Shop/ShopSlot1.dds", m_ShopSlotResources[0]);
	loadImage(L"Assets/Image/Shop/ShopSlot2.dds", m_ShopSlotResources[1]);
	loadImage(L"Assets/Image/Shop/ShopSlot3.dds", m_ShopSlotResources[2]);
	loadImage(L"Assets/Image/Shop/ShopSlot4.dds", m_ShopSlotResources[3]);
	loadImage(L"Assets/Image/Shop/InternetOnIcon.dds", m_InternetOnIconResource);
	loadImage(L"Assets/Image/Shop/InternetOffIcon.dds", m_InternetOffIconResource);
	loadImage(L"Assets/Image/Shop/NetworkErrorLog.dds", m_NetworkErrorLogResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockSlot1.dds", m_StockSlotResources[0]);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockSlot2.dds", m_StockSlotResources[1]);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/Limit1.dds", m_StockLimitResources[0]);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/Limit2.dds", m_StockLimitResources[1]);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/CantCreateStockGenLog.dds", m_CantCreateStockGenLogResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockName.dds", m_StockNameResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockHolders.dds", m_StockHoldersResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockManagementTable.dds", m_StockManagementTableResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockChart.dds", m_StockChartResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/MyGraph.dds", m_MyGraphResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/TargetGraph.dds", m_TargetGraphResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/SeeGraph.dds", m_SeeGraphResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockUpMark.dds", m_StockUpMarkResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/StockDownMark.dds", m_StockDownMarkResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/IssuanceStock.dds", m_IssuanceStockResource);
	loadImage(L"Assets/Image/Shop/Stock/StockManager/IssuanceStockErrorLog.dds", m_IssuanceStockErrorLogResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockTitle.dds", m_StockTransactionTitleResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockIssuer.dds", m_StockTransactionIssuerResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockGraph.dds", m_StockTransactionGraphResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockDescription.dds", m_StockTransactionDescriptionResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/SeeStock.dds", m_SeeStockResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockBuying.dds", m_StockBuyingResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockSelling.dds", m_StockSellingResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockBuyingFailLog.dds", m_StockBuyingFailLogResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockSellingFailLog.dds", m_StockSellingFailLogResource);
	loadImage(L"Assets/Image/Shop/Stock/StockTransaction/StockReceipt.dds", m_StockReceiptResource);
	loadImage(L"Assets/Image/Login/TextCursor.dds", m_TextCursorResource);
	loadImage(L"Assets/Image/Common/EmptySquare.dds", m_EmptySquareResources[0]);
	loadImage(L"Assets/Image/Common/EmptySquare.dds", m_EmptySquareResources[1]);
	loadImage(L"Assets/Image/Shop/PetConfirmationButton.dds", m_PetConfirmationButtonResource);
	loadImage(L"Assets/Image/Shop/ScrollBackGround.dds", m_ScrollBackgroundResource);
	loadImage(L"Assets/Image/Shop/Scroll.dds", m_ScrollResource);
	loadImage(L"Assets/Image/Common/EmptyFrame.dds", m_EmptyFrameResource);
	loadImage(L"Assets/Image/Shop/PetEnhanceButton.dds", m_PetEnhanceButtonResource);
	loadImage(L"Assets/Image/Shop/PetEnhancePriceFrame.dds", m_PetEnhancePriceFrameResource);
	loadImage(L"Assets/Image/Shop/PetEnhanceLog1.dds", m_PetEnhanceLogResources[0]);
	loadImage(L"Assets/Image/Shop/PetEnhanceLog2.dds", m_PetEnhanceLogResources[1]);
	loadImage(L"Assets/Image/Shop/Financial/BankFrame.dds", m_BankFrameResource);
	loadImage(L"Assets/Image/Shop/Financial/InstallmentSavingsButton.dds", m_FinancialCategoryButtonResources[0]);
	loadImage(L"Assets/Image/Shop/Financial/LoansButton.dds", m_FinancialCategoryButtonResources[1]);
	for (int i = 0; i < 10; ++i)
	{
		const std::wstring number = std::to_wstring(i + 1);
		loadImage((L"Assets/Image/Shop/Financial/IS" + number + L".dds").c_str(),
			m_FinancialProductNameResources[0][i]);
		loadImage((L"Assets/Image/Shop/Financial/Loans" + number + L".dds").c_str(),
			m_FinancialProductNameResources[1][i]);
	}
	loadImage(L"Assets/Image/Shop/Financial/LeftButton.dds", m_FinancialLeftButtonResource);
	loadImage(L"Assets/Image/Shop/Financial/RightButton.dds", m_FinancialRightButtonResource);
	loadImage(L"Assets/Image/Shop/Financial/TimerFrame.dds", m_FinancialTimerFrameResource);
	loadImage(L"Assets/Image/Shop/Financial/MoneyFrame.dds", m_FinancialMoneyFrameResource);
	loadImage(L"Assets/Image/Shop/Financial/RightPoint.dds", m_FinancialRightPointResource);
	loadImage(L"Assets/Image/Shop/Financial/ApplicationButton.dds", m_FinancialApplicationButtonResource);
	loadImage(L"Assets/Image/Shop/Financial/ISFailLog.dds", m_FinancialFailLogResources[0]);
	loadImage(L"Assets/Image/Shop/Financial/LoansFailLog.dds", m_FinancialFailLogResources[1]);
}

void CShopUI::ReleaseObjects()
{
	if (m_pPreviewPet)
	{
		delete m_pPreviewPet;
		m_pPreviewPet = NULL;
		m_nPreviewPetIndex = static_cast<size_t>(-1);
	}
	if (m_pd3dUiImagePipelineState) m_pd3dUiImagePipelineState->Release();
	UI_IMAGE_RESOURCE* images[] = { &m_ShopIconResource, &m_ShopBoardResource, &m_PageTitleResource,
		&m_ShopCloseIconResource, &m_ShopBackSpaceIconResource, &m_ShopSlotResources[0],
		&m_ShopSlotResources[1], &m_ShopSlotResources[2], &m_ShopSlotResources[3],
		&m_EmptySquareResources[0], &m_EmptySquareResources[1], &m_PetConfirmationButtonResource,
		&m_ScrollBackgroundResource, &m_ScrollResource, &m_EmptyFrameResource,
		&m_PetEnhanceButtonResource, &m_PetEnhancePriceFrameResource,
		&m_PetEnhanceLogResources[0], &m_PetEnhanceLogResources[1], &m_BankFrameResource,
		&m_FinancialCategoryButtonResources[0], &m_FinancialCategoryButtonResources[1],
		&m_FinancialLeftButtonResource, &m_FinancialRightButtonResource,
		&m_FinancialTimerFrameResource, &m_FinancialMoneyFrameResource,
		&m_FinancialRightPointResource, &m_FinancialApplicationButtonResource,
		&m_FinancialFailLogResources[0], &m_FinancialFailLogResources[1],
		&m_InternetOnIconResource,
		&m_InternetOffIconResource, &m_NetworkErrorLogResource,
		&m_StockSlotResources[0], &m_StockSlotResources[1],
		&m_StockLimitResources[0], &m_StockLimitResources[1],
		&m_CantCreateStockGenLogResource, &m_StockNameResource,
		&m_StockHoldersResource, &m_StockManagementTableResource,
		&m_StockChartResource, &m_MyGraphResource, &m_TargetGraphResource, &m_SeeGraphResource, &m_StockUpMarkResource,
		&m_StockDownMarkResource, &m_IssuanceStockResource, &m_IssuanceStockErrorLogResource,
		&m_StockTransactionTitleResource, &m_StockTransactionIssuerResource,
		&m_StockTransactionGraphResource, &m_StockTransactionDescriptionResource,
		&m_SeeStockResource, &m_StockBuyingResource, &m_StockSellingResource,
		&m_StockBuyingFailLogResource, &m_StockSellingFailLogResource,
		&m_StockReceiptResource, &m_TextCursorResource };
	for (UI_IMAGE_RESOURCE* image : images)
	{
		if (image->pd3dTexture) image->pd3dTexture->Release();
		if (image->pd3dTextureUploadBuffer) image->pd3dTextureUploadBuffer->Release();
		if (image->pd3dSrvDescriptorHeap) image->pd3dSrvDescriptorHeap->Release();
	}
	for (int category = 0; category < 2; ++category)
	{
		for (int index = 0; index < 10; ++index)
		{
			UI_IMAGE_RESOURCE& image = m_FinancialProductNameResources[category][index];
			if (image.pd3dTexture) image.pd3dTexture->Release();
			if (image.pd3dTextureUploadBuffer) image.pd3dTextureUploadBuffer->Release();
			if (image.pd3dSrvDescriptorHeap) image.pd3dSrvDescriptorHeap->Release();
		}
	}
}

void CShopUI::Animate(float elapsedTime)
{
	if (m_bShopActive && m_eShopPage == SHOP_PAGE::PET_CHANGE && m_pPreviewPet)
		m_pPreviewPet->AnimateWithoutMovement(elapsedTime);
	if (m_bStockNameInputActive)
	{
		m_fStockNameCursorBlinkElapsed += elapsedTime;
		while (m_fStockNameCursorBlinkElapsed >= 1.0f)
			m_fStockNameCursorBlinkElapsed -= 1.0f;
	}
	if (m_bStockTransactionQuantityInputActive)
	{
		m_fStockTransactionQuantityCursorBlinkElapsed += elapsedTime;
		while (m_fStockTransactionQuantityCursorBlinkElapsed >= 1.0f)
			m_fStockTransactionQuantityCursorBlinkElapsed -= 1.0f;
	}
	for (int category = 0; category < 2; ++category)
	{
		if (!m_bFinancialProductActive[category]) continue;
		m_fActiveFinancialElapsedSeconds[category] += elapsedTime;
		if (m_fActiveFinancialElapsedSeconds[category] >
			static_cast<float>(m_nActiveFinancialDurationSeconds[category]))
			m_fActiveFinancialElapsedSeconds[category] =
			static_cast<float>(m_nActiveFinancialDurationSeconds[category]);
	}
	for (SHOP_NETWORK_ERROR_LOG& log : m_NetworkErrorLogs)
		log.fElapsedTime += elapsedTime;
	m_NetworkErrorLogs.erase(std::remove_if(m_NetworkErrorLogs.begin(), m_NetworkErrorLogs.end(),
		[](const SHOP_NETWORK_ERROR_LOG& log) { return(log.fElapsedTime >= 1.0f); }),
		m_NetworkErrorLogs.end());
	for (int i = 0; i < 2; ++i)
	{
		for (SHOP_NETWORK_ERROR_LOG& log : m_FinancialFailLogs[i])
			log.fElapsedTime += elapsedTime;
		m_FinancialFailLogs[i].erase(std::remove_if(m_FinancialFailLogs[i].begin(),
			m_FinancialFailLogs[i].end(),
			[](const SHOP_NETWORK_ERROR_LOG& log) { return(log.fElapsedTime >= 1.0f); }),
			m_FinancialFailLogs[i].end());
	}
	for (SHOP_NETWORK_ERROR_LOG& log : m_StockIssueErrorLogs)
		log.fElapsedTime += elapsedTime;
	m_StockIssueErrorLogs.erase(std::remove_if(m_StockIssueErrorLogs.begin(), m_StockIssueErrorLogs.end(),
		[](const SHOP_NETWORK_ERROR_LOG& log) { return(log.fElapsedTime >= 1.0f); }),
		m_StockIssueErrorLogs.end());
	for (int i = 0; i < 2; ++i)
	{
		for (SHOP_NETWORK_ERROR_LOG& log : m_StockTradeFailLogs[i])
			log.fElapsedTime += elapsedTime;
		m_StockTradeFailLogs[i].erase(std::remove_if(m_StockTradeFailLogs[i].begin(),
			m_StockTradeFailLogs[i].end(),
			[](const SHOP_NETWORK_ERROR_LOG& log) { return(log.fElapsedTime >= 1.0f); }),
			m_StockTradeFailLogs[i].end());
	}
}

void CShopUI::ReleaseUploadBuffers()
{
	UI_IMAGE_RESOURCE* images[] = { &m_ShopIconResource, &m_ShopBoardResource, &m_PageTitleResource,
		&m_ShopCloseIconResource, &m_ShopBackSpaceIconResource, &m_ShopSlotResources[0],
		&m_ShopSlotResources[1], &m_ShopSlotResources[2], &m_ShopSlotResources[3],
		&m_EmptySquareResources[0], &m_EmptySquareResources[1], &m_PetConfirmationButtonResource,
		&m_ScrollBackgroundResource, &m_ScrollResource, &m_EmptyFrameResource,
		&m_PetEnhanceButtonResource, &m_PetEnhancePriceFrameResource,
		&m_PetEnhanceLogResources[0], &m_PetEnhanceLogResources[1], &m_BankFrameResource,
		&m_FinancialCategoryButtonResources[0], &m_FinancialCategoryButtonResources[1],
		&m_FinancialLeftButtonResource, &m_FinancialRightButtonResource,
		&m_FinancialTimerFrameResource, &m_FinancialMoneyFrameResource,
		&m_FinancialRightPointResource, &m_FinancialApplicationButtonResource,
		&m_FinancialFailLogResources[0], &m_FinancialFailLogResources[1],
		&m_InternetOnIconResource,
		&m_InternetOffIconResource, &m_NetworkErrorLogResource,
		&m_StockSlotResources[0], &m_StockSlotResources[1],
		&m_StockLimitResources[0], &m_StockLimitResources[1],
		&m_CantCreateStockGenLogResource, &m_StockNameResource,
		&m_StockHoldersResource, &m_StockManagementTableResource,
		&m_StockChartResource, &m_MyGraphResource, &m_TargetGraphResource, &m_SeeGraphResource, &m_StockUpMarkResource,
		&m_StockDownMarkResource, &m_IssuanceStockResource, &m_IssuanceStockErrorLogResource,
		&m_StockTransactionTitleResource, &m_StockTransactionIssuerResource,
		&m_StockTransactionGraphResource, &m_StockTransactionDescriptionResource,
		&m_SeeStockResource, &m_StockBuyingResource, &m_StockSellingResource,
		&m_StockBuyingFailLogResource, &m_StockSellingFailLogResource,
		&m_StockReceiptResource, &m_TextCursorResource };
	for (UI_IMAGE_RESOURCE* image : images)
	{
		if (image->pd3dTextureUploadBuffer)
		{
			image->pd3dTextureUploadBuffer->Release();
			image->pd3dTextureUploadBuffer = NULL;
		}
	}
	for (int category = 0; category < 2; ++category)
	{
		for (int index = 0; index < 10; ++index)
		{
			UI_IMAGE_RESOURCE& image = m_FinancialProductNameResources[category][index];
			if (image.pd3dTextureUploadBuffer)
			{
				image.pd3dTextureUploadBuffer->Release();
				image.pd3dTextureUploadBuffer = NULL;
			}
		}
	}
}

void CShopUI::RenderUiImage(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	UI_IMAGE_RESOURCE& image, const XMFLOAT4& rectangle, UINT tintColor)
{
	if (!m_pd3dUiImagePipelineState || !camera || !image.pd3dSrvDescriptorHeap) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	const float constants[4] = { rectangle.x / width * 2.0f - 1.0f,
		1.0f - rectangle.y / height * 2.0f, rectangle.z / width * 2.0f - 1.0f,
		1.0f - rectangle.w / height * 2.0f };
	ID3D12DescriptorHeap* heaps[] = { image.pd3dSrvDescriptorHeap };
	commandList->SetDescriptorHeaps(1, heaps);
	commandList->SetGraphicsRootDescriptorTable(3,
		image.pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRoot32BitConstants(4, 4, constants, 0);
	commandList->SetGraphicsRoot32BitConstant(5, tintColor, 0);
	commandList->SetPipelineState(m_pd3dUiImagePipelineState);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(6, 1, 0, 0);
}

void CShopUI::RenderTextLine(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	const std::string& text, float left, float top, float scale, UINT color,
	const SHOP_TEXT_RENDER_CONTEXT& context)
{
	if (!context.pTextPipelineState || !context.pGlyphResources || !camera) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	float cursorX = left;
	const float glyphGap = (scale * 8.0f > 1.0f) ? scale * 8.0f : 1.0f;
	const float spaceWidth = scale * 70.0f;
	commandList->SetPipelineState(context.pTextPipelineState);
	commandList->SetGraphicsRoot32BitConstant(5, color, 0);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	for (char ch : text)
	{
		if (ch == ' ') { cursorX += spaceWidth; continue; }
		TEXT_GLYPH_RESOURCE* selected = NULL;
		for (TEXT_GLYPH_RESOURCE& glyph : *context.pGlyphResources)
			if (glyph.ch == ch) { selected = &glyph; break; }
		if (!selected || !selected->pd3dSrvDescriptorHeap) continue;
		const float imageWidth = selected->fPixelWidth / (selected->fU1 - selected->fU0);
		const float imageHeight = selected->fPixelHeight / (selected->fV1 - selected->fV0);
		const float rectLeft = cursorX - selected->fU0 * imageWidth * scale;
		const float rectTop = top + selected->fTopOffset * scale - selected->fV0 * imageHeight * scale;
		const float rectRight = rectLeft + imageWidth * scale;
		const float rectBottom = rectTop + imageHeight * scale;
		const float constants[4] = { rectLeft / width * 2.0f - 1.0f,
			1.0f - rectTop / height * 2.0f, rectRight / width * 2.0f - 1.0f,
			1.0f - rectBottom / height * 2.0f };
		ID3D12DescriptorHeap* heaps[] = { selected->pd3dSrvDescriptorHeap };
		commandList->SetDescriptorHeaps(1, heaps);
		commandList->SetGraphicsRootDescriptorTable(3,
			selected->pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		commandList->SetGraphicsRoot32BitConstants(4, 4, constants, 0);
		commandList->DrawInstanced(6, 1, 0, 0);
		cursorX += selected->fPixelWidth * scale + glyphGap;
	}
	float textWidth = cursorX - left;
	if (!text.empty() && text.back() != ' ' && textWidth >= glyphGap) textWidth -= glyphGap;
}

void CShopUI::RenderSolidUiRectangle(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	float left, float top, float right, float bottom, UINT color,
	const SHOP_TEXT_RENDER_CONTEXT& context)
{
	if (!context.pSolidUiPipelineState || !camera) return;
	const float width = camera->m_d3dViewport.Width;
	const float height = camera->m_d3dViewport.Height;
	const float constants[4] = { left / width * 2.0f - 1.0f, 1.0f - top / height * 2.0f,
		right / width * 2.0f - 1.0f, 1.0f - bottom / height * 2.0f };
	commandList->SetPipelineState(context.pSolidUiPipelineState);
	commandList->SetGraphicsRoot32BitConstants(4, 4, constants, 0);
	commandList->SetGraphicsRoot32BitConstant(5, color, 0);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(6, 1, 0, 0);
}

void CShopUI::RenderStockQuantityCursor(ID3D12GraphicsCommandList* commandList, CCamera* camera,
	const XMFLOAT4& rectangle, UINT quantity, float fontSize)
{
	if (!g_pFramework || !camera || m_fStockTransactionQuantityCursorBlinkElapsed >= 0.5f)
		return;
	const std::wstring text = std::to_wstring(quantity);
	float measuredWidth = 0.0f;
	float measuredHeight = 0.0f;
	g_pFramework->MeasureDirectWriteText(text, fontSize,
		max(rectangle.z - rectangle.x, 1.0f), max(rectangle.w - rectangle.y, 1.0f),
		measuredWidth, measuredHeight);
	const float textLeft = (rectangle.x + rectangle.z - measuredWidth) * 0.5f;
	const float cursorHeight = (rectangle.w - rectangle.y) * 0.72f;
	const float cursorWidth = max(2.0f, cursorHeight * 0.13f);
	const float cursorX = min(textLeft + measuredWidth + 2.0f, rectangle.z - cursorWidth);
	const float cursorTop = (rectangle.y + rectangle.w - cursorHeight) * 0.5f;
	RenderUiImage(commandList, camera, m_TextCursorResource,
		XMFLOAT4(cursorX, cursorTop, cursorX + cursorWidth, cursorTop + cursorHeight));
}

XMFLOAT4 CShopUI::GetMoneyUiRectangle(float width, float height, UINT money,
	const SHOP_TEXT_RENDER_CONTEXT& context) const
{
	if (!context.pGlyphResources) return(XMFLOAT4());
	auto findGlyph = [&context](char ch) -> const TEXT_GLYPH_RESOURCE*
	{
		for (const TEXT_GLYPH_RESOURCE& glyph : *context.pGlyphResources)
			if (glyph.ch == ch) return(&glyph);
		return(NULL);
	};
	float fixedWidth = 0.0f;
	for (const char* p = gMoneyUiLayout.pWidthReferenceText; *p; ++p)
	{
		const TEXT_GLYPH_RESOURCE* glyph = findGlyph(*p);
		if (glyph) fixedWidth += glyph->fPixelWidth * gMoneyUiLayout.fGlyphScale + gMoneyUiLayout.fGlyphGap;
	}
	if (fixedWidth > 0.0f) fixedWidth -= gMoneyUiLayout.fGlyphGap;
	float minimumTop = FLT_MAX;
	float maximumBottom = -FLT_MAX;
	for (char ch : FormatPossession(money))
	{
		const TEXT_GLYPH_RESOURCE* glyph = findGlyph(ch);
		if (!glyph) continue;
		minimumTop = min(minimumTop, glyph->fTopOffset * gMoneyUiLayout.fGlyphScale);
		maximumBottom = max(maximumBottom,
			(glyph->fTopOffset + glyph->fPixelHeight) * gMoneyUiLayout.fGlyphScale);
	}
	if (minimumTop == FLT_MAX) return(XMFLOAT4());
	const float panelWidth = fixedWidth + gMoneyUiLayout.fHorizontalPadding * 2.0f;
	const float panelHeight = maximumBottom - minimumTop + gMoneyUiLayout.fVerticalPadding * 2.0f;
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float right = board.z - gShopUiLayout.fMoneyRightPadding;
	const float bottom = board.w - gShopUiLayout.fMoneyBottomPadding;
	return(XMFLOAT4(right - panelWidth, bottom - panelHeight, right, bottom));
}

void CShopUI::RenderMoneyUI(ID3D12GraphicsCommandList* commandList, CCamera* camera, UINT money,
	const SHOP_TEXT_RENDER_CONTEXT& context)
{
	if (!context.pGlyphResources || !camera) return;
	const std::string text = FormatPossession(money);
	float textWidth = 0.0f;
	float minimumTop = FLT_MAX;
	for (char ch : text)
	{
		for (const TEXT_GLYPH_RESOURCE& glyph : *context.pGlyphResources)
		{
			if (glyph.ch != ch) continue;
			textWidth += glyph.fPixelWidth * gMoneyUiLayout.fGlyphScale + gMoneyUiLayout.fGlyphGap;
			minimumTop = min(minimumTop, glyph.fTopOffset * gMoneyUiLayout.fGlyphScale);
			break;
		}
	}
	if (textWidth > 0.0f) textWidth -= gMoneyUiLayout.fGlyphGap;
	const XMFLOAT4 panel = GetMoneyUiRectangle(camera->m_d3dViewport.Width,
		camera->m_d3dViewport.Height, money, context);
	const float outline = gMoneyUiLayout.fOutlineThickness;
	RenderSolidUiRectangle(commandList, camera, panel.x - outline, panel.y - outline,
		panel.z + outline, panel.w + outline, 0x00000000, context);
	RenderSolidUiRectangle(commandList, camera, panel.x, panel.y, panel.z, panel.w, 0x00FFFFFF, context);
	const float left = panel.z - gMoneyUiLayout.fHorizontalPadding - textWidth;
	const float top = panel.y + gMoneyUiLayout.fVerticalPadding - minimumTop;
	RenderTextLine(commandList, camera, text, left, top, gMoneyUiLayout.fGlyphScale, 0x00000000, context);
}

void CShopUI::EnsurePreviewPet(const std::vector<SHOP_PET_RENDER_RESOURCE>& pets)
{
	if (m_nSelectedPetIndex >= pets.size() || !pets[m_nSelectedPetIndex].pPet)
	{
		if (m_pPreviewPet) delete m_pPreviewPet;
		m_pPreviewPet = NULL;
		m_nPreviewPetIndex = static_cast<size_t>(-1);
		return;
	}
	if (m_pPreviewPet && m_nPreviewPetIndex == m_nSelectedPetIndex) return;

	if (m_pPreviewPet) delete m_pPreviewPet;
	CPet* sourcePet = pets[m_nSelectedPetIndex].pPet;
	m_pPreviewPet = new CPet();
	m_pPreviewPet->SetMesh(sourcePet->m_pMesh);
	m_pPreviewPet->SetShader(sourcePet->m_pShader);
	m_pPreviewPet->SetName(sourcePet->GetName());
	m_pPreviewPet->SetPay(sourcePet->GetPay());
	m_pPreviewPet->GetMaxPossession(sourcePet->GetMaxPossession());
	m_pPreviewPet->GetNowPossession(0);
	m_pPreviewPet->SetPosition(0.0f, 0.0f, 0.0f);
	m_nPreviewPetIndex = m_nSelectedPetIndex;
}

void CShopUI::RenderPreviewPet(ID3D12GraphicsCommandList* commandList, CCamera* mainCamera,
	const XMFLOAT4& panel, const std::vector<SHOP_PET_RENDER_RESOURCE>& pets)
{
	EnsurePreviewPet(pets);
	if (!m_pPreviewPet || m_nPreviewPetIndex >= pets.size()
		|| !pets[m_nPreviewPetIndex].pd3dSrvDescriptorHeap) return;

	const LONG inset = 3;
	const LONG left = static_cast<LONG>(panel.x) + inset;
	const LONG top = static_cast<LONG>(panel.y) + inset;
	const LONG right = static_cast<LONG>(panel.z) - inset;
	const LONG bottom = static_cast<LONG>(panel.w) - inset;
	const int viewportWidth = right - left;
	const int viewportHeight = bottom - top;
	if (viewportWidth <= 0 || viewportHeight <= 0) return;

	m_PreviewPetCamera.SetViewport(left, top, viewportWidth, viewportHeight, 0.0f, 0.2f);
	m_PreviewPetCamera.SetScissorRect(left, top, right, bottom);

	const BoundingOrientedBox& bounds = m_pPreviewPet->m_pMesh->m_xmOOBB;
	XMFLOAT3 target = bounds.Center;
	XMStoreFloat3(&target, XMVector3TransformCoord(XMLoadFloat3(&bounds.Center),
		XMLoadFloat4x4(&m_pPreviewPet->m_xmf4x4World)));
	const float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
	const float fieldOfView = 45.0f;
	const float tanHalfVerticalFov = tanf(XMConvertToRadians(fieldOfView * 0.5f));
	const float tanHalfHorizontalFov = tanHalfVerticalFov * aspectRatio;
	const float horizontalRadius = sqrtf(bounds.Extents.x * bounds.Extents.x
		+ bounds.Extents.z * bounds.Extents.z);
	const float verticalDistance = bounds.Extents.y / (tanHalfVerticalFov * 0.70f);
	const float horizontalDistance = horizontalRadius / (tanHalfHorizontalFov * 0.70f);
	float cameraDistance = ((verticalDistance > horizontalDistance)
		? verticalDistance : horizontalDistance) + horizontalRadius;
	if (cameraDistance < 1.0f) cameraDistance = 10.0f;
	const XMFLOAT3 cameraPosition(target.x, target.y, target.z - cameraDistance);
	m_PreviewPetCamera.GenerateViewMatrix(cameraPosition, target, XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_PreviewPetCamera.GenerateProjectionMatrix(0.1f, 1000.0f,
		aspectRatio, fieldOfView);
	m_PreviewPetCamera.SetViewportsAndScissorRects(commandList);
	m_PreviewPetCamera.UpdateShaderVariables(commandList);

	ID3D12DescriptorHeap* descriptorHeaps[] = { pets[m_nPreviewPetIndex].pd3dSrvDescriptorHeap };
	commandList->SetDescriptorHeaps(1, descriptorHeaps);
	commandList->SetGraphicsRootDescriptorTable(3,
		pets[m_nPreviewPetIndex].pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	m_pPreviewPet->Render(commandList, &m_PreviewPetCamera);

	mainCamera->SetViewportsAndScissorRects(commandList);
	mainCamera->UpdateShaderVariables(commandList);
}

XMFLOAT4 CShopUI::GetEnhanceButtonRectangle(int type, float width, float height) const
{
	const XMFLOAT4 panel = GetPetContentPanelRectangle(type != 0, width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float panelWidth = panel.z - panel.x;
	const float panelHeight = panel.w - panel.y;
	const float buttonWidth = panelWidth * 0.40f;
	const float buttonHeight = buttonWidth * (216.0f / 447.0f);
	const float left = panel.x + panelWidth * 0.06f;
	const float top = panel.y + panelHeight * 0.76f;
	return(XMFLOAT4(left, top, left + buttonWidth, top + buttonHeight));
}

XMFLOAT4 CShopUI::GetEnhancePriceRectangle(int type, float width, float height) const
{
	const XMFLOAT4 button = GetEnhanceButtonRectangle(type, width, height);
	const float buttonHeight = button.w - button.y;
	const float priceHeight = buttonHeight * 0.70f;
	const float priceWidth = buttonHeight * (493.0f / 213.0f);
	const float gap = (button.z - button.x) * 0.05f;
	const float top = (button.y + button.w - priceHeight) * 0.5f;
	return(XMFLOAT4(button.z + gap, top, button.z + gap + priceWidth, top + priceHeight));
}

XMFLOAT4 CShopUI::GetFinancialCategoryButtonRectangle(int category, float width, float height) const
{
	const XMFLOAT4 bank = GetFinancialBankFrameRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float bankWidth = bank.z - bank.x;
	const float buttonWidth = bankWidth * 0.168f;
	const float buttonHeight = buttonWidth * (197.0f / 439.0f);
	const float gap = 0.0f;
	const float totalWidth = buttonWidth * 2.0f + gap;
	const float left = (bank.x + bank.z - totalWidth) * 0.5f + category * (buttonWidth + gap);
	const float top = bank.y + (bank.w - bank.y) * 0.05f;
	return(XMFLOAT4(left, top, left + buttonWidth, top + buttonHeight));
}

XMFLOAT4 CShopUI::GetFinancialLeftButtonRectangle(float width, float height) const
{
	const XMFLOAT4 bank = GetFinancialBankFrameRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const XMFLOAT4 product = GetFinancialProductNameRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float buttonHeight = (product.w - product.y) * 1.35f;
	const float buttonWidth = buttonHeight * (201.0f / 234.0f);
	const float left = bank.x + (bank.z - bank.x) * 0.035f;
	const float centerY = (product.y + product.w) * 0.5f;
	return(XMFLOAT4(left, centerY - buttonHeight * 0.5f, left + buttonWidth,
		centerY + buttonHeight * 0.5f));
}

XMFLOAT4 CShopUI::GetFinancialRightButtonRectangle(float width, float height) const
{
	const XMFLOAT4 bank = GetFinancialBankFrameRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const XMFLOAT4 leftButton = GetFinancialLeftButtonRectangle(width, height);
	const float buttonWidth = leftButton.z - leftButton.x;
	const float right = bank.z - (bank.z - bank.x) * 0.035f;
	return(XMFLOAT4(right - buttonWidth, leftButton.y, right, leftButton.w));
}

XMFLOAT4 CShopUI::GetFinancialApplicationButtonRectangle(float width, float height,
	UINT money, const SHOP_TEXT_RENDER_CONTEXT& context) const
{
	const XMFLOAT4 moneyRect = GetMoneyUiRectangle(width, height, money, context);
	const XMFLOAT4 confirm = GetPetConfirmationRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float buttonWidth = confirm.z - confirm.x;
	const float buttonHeight = confirm.w - confirm.y;
	const float gap = 8.0f;
	const float top = moneyRect.y + ((moneyRect.w - moneyRect.y - buttonHeight) * 0.5f);
	return(XMFLOAT4(moneyRect.x - gap - buttonWidth, top,
		moneyRect.x - gap, top + buttonHeight));
}

XMFLOAT4 CShopUI::GetStockSlotRectangle(int index, float width, float height) const
{
	const XMFLOAT4 board = GetShopBoardRectangle(width, height,
		m_xmf2ShopBoardOffset.x, m_xmf2ShopBoardOffset.y);
	const float boardWidth = board.z - board.x;
	const float boardHeight = board.w - board.y;
	const float slotWidth = boardWidth * 0.46f;
	const float slotHeight = slotWidth * (260.0f / 1190.0f);
	const float left = board.x + boardWidth * 0.065f;
	const float top = board.y + boardHeight * 0.28f + index * (slotHeight + boardHeight * 0.14f);
	return(XMFLOAT4(left, top, left + slotWidth, top + slotHeight));
}

