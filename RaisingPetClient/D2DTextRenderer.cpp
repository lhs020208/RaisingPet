#include "stdafx.h"
#include "D2DTextRenderer.h"

namespace
{
void OutputD2DTextRendererError(const wchar_t* step, HRESULT result)
{
	wchar_t message[256] = {};
	swprintf_s(message, L"[D2DTextRenderer] %s failed. HRESULT=0x%08X\n",
		step, static_cast<unsigned int>(result));
	OutputDebugStringW(message);
}
}

bool CD2DTextRenderer::CreateObjects(ID3D12Device* d3dDevice, ID3D12CommandQueue* commandQueue,
	ID3D12Resource* const* swapChainBuffers, UINT swapChainBufferCount)
{
	ReleaseObjects();
	if (!d3dDevice || !commandQueue || !swapChainBuffers || swapChainBufferCount == 0)
	{
		OutputDebugStringW(L"[D2DTextRenderer] Invalid CreateObjects arguments.\n");
		return(false);
	}

	UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	IUnknown* queues[] = { commandQueue };
	HRESULT result = D3D11On12CreateDevice(d3dDevice, d3d11DeviceFlags, nullptr, 0,
		queues, _countof(queues), 0, &m_d3d11Device, &m_d3d11DeviceContext, nullptr);
	if (FAILED(result)) { OutputD2DTextRendererError(L"D3D11On12CreateDevice", result); return(false); }

	result = m_d3d11Device.As(&m_d3d11On12Device);
	if (FAILED(result)) { OutputD2DTextRendererError(L"Query ID3D11On12Device", result); return(false); }

	D2D1_FACTORY_OPTIONS factoryOptions = {};
#if defined(_DEBUG)
	factoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
	result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3),
		&factoryOptions, reinterpret_cast<void**>(m_d2dFactory.GetAddressOf()));
	if (FAILED(result)) { OutputD2DTextRendererError(L"D2D1CreateFactory", result); return(false); }

	ComPtr<IDXGIDevice> dxgiDevice;
	result = m_d3d11Device.As(&dxgiDevice);
	if (FAILED(result)) { OutputD2DTextRendererError(L"Query IDXGIDevice", result); return(false); }
	result = m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice);
	if (FAILED(result)) { OutputD2DTextRendererError(L"Create D2D device", result); return(false); }
	result = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dDeviceContext);
	if (FAILED(result)) { OutputD2DTextRendererError(L"Create D2D device context", result); return(false); }
	m_d2dDeviceContext->SetDpi(96.0f, 96.0f);

	result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3),
		reinterpret_cast<IUnknown**>(m_dWriteFactory.GetAddressOf()));
	if (FAILED(result)) { OutputD2DTextRendererError(L"DWriteCreateFactory", result); return(false); }

	result = m_dWriteFactory->CreateTextFormat(L"Malgun Gothic", nullptr,
		DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		26.0f, L"ko-kr", &m_textFormat);
	if (FAILED(result)) { OutputD2DTextRendererError(L"Create default text format", result); return(false); }
	m_textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

	result = m_d2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
		&m_textBrush);
	if (FAILED(result)) { OutputD2DTextRendererError(L"Create text brush", result); return(false); }

	m_wrappedBackBuffers.resize(swapChainBufferCount);
	m_d2dRenderTargets.resize(swapChainBufferCount);
	for (UINT i = 0; i < swapChainBufferCount; ++i)
	{
		D3D11_RESOURCE_FLAGS d3d11Flags = {};
		d3d11Flags.BindFlags = D3D11_BIND_RENDER_TARGET;
		result = m_d3d11On12Device->CreateWrappedResource(swapChainBuffers[i], &d3d11Flags,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT,
			__uuidof(ID3D11Resource), reinterpret_cast<void**>(m_wrappedBackBuffers[i].GetAddressOf()));
		if (FAILED(result)) { OutputD2DTextRendererError(L"Create wrapped back buffer", result); return(false); }

		ComPtr<IDXGISurface> dxgiSurface;
		result = m_wrappedBackBuffers[i].As(&dxgiSurface);
		if (FAILED(result)) { OutputD2DTextRendererError(L"Query IDXGISurface", result); return(false); }

		D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			96.0f, 96.0f);
		result = m_d2dDeviceContext->CreateBitmapFromDxgiSurface(dxgiSurface.Get(),
			&bitmapProperties, &m_d2dRenderTargets[i]);
		if (FAILED(result)) { OutputD2DTextRendererError(L"Create D2D bitmap from back buffer", result); return(false); }
	}

	m_bReady = true;
	OutputDebugStringW(L"[D2DTextRenderer] Ready.\n");
	return(true);
}

void CD2DTextRenderer::ReleaseObjects()
{
	m_bReady = false;
	m_queuedTexts.clear();
	m_d2dRenderTargets.clear();
	m_wrappedBackBuffers.clear();
	m_textBrush.Reset();
	m_textFormat.Reset();
	m_dWriteFactory.Reset();
	m_d2dDeviceContext.Reset();
	m_d2dDevice.Reset();
	m_d2dFactory.Reset();
	m_d3d11On12Device.Reset();
	m_d3d11DeviceContext.Reset();
	m_d3d11Device.Reset();
}

void CD2DTextRenderer::QueueText(const std::wstring& text, const XMFLOAT4& rectangle, float fontSize,
	UINT color, bool horizontalCenter, bool verticalCenter)
{
	if (text.empty()) return;
	D2D_TEXT_ITEM item;
	item.text = text;
	item.rectangle = rectangle;
	item.fontSize = fontSize;
	item.color = color;
	item.horizontalCenter = horizontalCenter;
	item.verticalCenter = verticalCenter;
	m_queuedTexts.push_back(item);
}

void CD2DTextRenderer::Render(UINT swapChainBufferIndex)
{
	if (!m_d3d11On12Device || !m_d3d11DeviceContext || !m_d2dDeviceContext ||
		swapChainBufferIndex >= m_wrappedBackBuffers.size()) return;
	if (!m_queuedTexts.empty())
	{
		wchar_t message[128] = {};
		swprintf_s(message, L"[D2DTextRenderer] Render queued text count=%zu\n", m_queuedTexts.size());
		OutputDebugStringW(message);
	}

	ID3D11Resource* resources[] = { m_wrappedBackBuffers[swapChainBufferIndex].Get() };
	m_d3d11On12Device->AcquireWrappedResources(resources, _countof(resources));
	m_d2dDeviceContext->SetTarget(m_d2dRenderTargets[swapChainBufferIndex].Get());
	m_d2dDeviceContext->BeginDraw();

	for (const D2D_TEXT_ITEM& item : m_queuedTexts)
	{
		ComPtr<IDWriteTextFormat> textFormat;
		HRESULT result = m_dWriteFactory->CreateTextFormat(L"Malgun Gothic", nullptr,
			DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			item.fontSize, L"ko-kr", &textFormat);
		if (FAILED(result))
		{
			OutputD2DTextRendererError(L"Create queued text format", result);
			continue;
		}

		textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		textFormat->SetTextAlignment(item.horizontalCenter
			? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_LEADING);
		textFormat->SetParagraphAlignment(item.verticalCenter
			? DWRITE_PARAGRAPH_ALIGNMENT_CENTER : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

		m_textBrush->SetColor(ToD2DColor(item.color));
		const D2D1_RECT_F layoutRect = D2D1::RectF(item.rectangle.x, item.rectangle.y,
			item.rectangle.z, item.rectangle.w);
		m_d2dDeviceContext->DrawTextW(item.text.c_str(), static_cast<UINT32>(item.text.length()),
			textFormat.Get(), layoutRect, m_textBrush.Get());
	}

	const HRESULT drawResult = m_d2dDeviceContext->EndDraw();
	if (FAILED(drawResult)) OutputD2DTextRendererError(L"EndDraw", drawResult);
	m_d3d11On12Device->ReleaseWrappedResources(resources, _countof(resources));
	m_d3d11DeviceContext->Flush();
	m_queuedTexts.clear();
}

D2D1_COLOR_F CD2DTextRenderer::ToD2DColor(UINT color)
{
	const float a = ((color >> 24) & 0xFF) / 255.0f;
	const float r = ((color >> 16) & 0xFF) / 255.0f;
	const float g = ((color >> 8) & 0xFF) / 255.0f;
	const float b = (color & 0xFF) / 255.0f;
	return(D2D1::ColorF(r, g, b, (a <= 0.0f) ? 1.0f : a));
}
