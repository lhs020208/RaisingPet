#pragma once

struct D2D_TEXT_ITEM
{
	std::wstring text;
	XMFLOAT4 rectangle = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	float fontSize = 24.0f;
	UINT color = 0xFF000000;
	bool horizontalCenter = false;
	bool verticalCenter = true;
};

class CD2DTextRenderer
{
public:
	CD2DTextRenderer() {}
	~CD2DTextRenderer() { ReleaseObjects(); }

	bool CreateObjects(ID3D12Device* d3dDevice, ID3D12CommandQueue* commandQueue,
		ID3D12Resource* const* swapChainBuffers, UINT swapChainBufferCount);
	void ReleaseObjects();
	bool IsReady() const { return m_bReady; }

	void QueueText(const std::wstring& text, const XMFLOAT4& rectangle, float fontSize,
		UINT color = 0xFF000000, bool horizontalCenter = false, bool verticalCenter = true);
	void Render(UINT swapChainBufferIndex);
	size_t GetQueuedTextCount() const { return m_queuedTexts.size(); }

private:
	Microsoft::WRL::ComPtr<ID3D11Device> m_d3d11Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3d11DeviceContext;
	Microsoft::WRL::ComPtr<ID3D11On12Device> m_d3d11On12Device;
	Microsoft::WRL::ComPtr<ID2D1Factory3> m_d2dFactory;
	Microsoft::WRL::ComPtr<ID2D1Device2> m_d2dDevice;
	Microsoft::WRL::ComPtr<ID2D1DeviceContext2> m_d2dDeviceContext;
	Microsoft::WRL::ComPtr<IDWriteFactory3> m_dWriteFactory;
	Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_textBrush;
	std::vector<Microsoft::WRL::ComPtr<ID3D11Resource>> m_wrappedBackBuffers;
	std::vector<Microsoft::WRL::ComPtr<ID2D1Bitmap1>> m_d2dRenderTargets;
	std::vector<D2D_TEXT_ITEM> m_queuedTexts;
	bool m_bReady = false;

	static D2D1_COLOR_F ToD2DColor(UINT color);
};
