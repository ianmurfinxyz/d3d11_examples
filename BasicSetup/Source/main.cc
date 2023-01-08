#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <iostream>
#include <vector>
#include <locale>
#include <codecvt>
#include <string>
#include <cassert>
#include <sstream>

namespace d3d = DirectX;

constexpr LPCTSTR WndClassName = L"GameWindow";
HWND hwnd = nullptr;

constexpr int width = 800;
constexpr int height = 600;

std::wstring s2ws(const std::string& str) {
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr) {
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

HANDLE consoleInput{INVALID_HANDLE_VALUE};
void LogMsg(std::wstring message) {
	if(consoleInput == INVALID_HANDLE_VALUE) return;
	WriteConsole(consoleInput, (void*)message.c_str(), (DWORD)message.size(), nullptr, nullptr);
}
void LogMsg(std::string message) {
	if(consoleInput == INVALID_HANDLE_VALUE) return;
	auto wmessage = s2ws(message);
	WriteConsole(consoleInput, (void*)wmessage.c_str(), (DWORD)wmessage.size(), nullptr, nullptr);
}

void Terminate() {
	MessageBox(0, L"Error happened! Ok to exit", L"Exit?", MB_OK);
	std::exit(-1);
}

void Check(HRESULT result) {
	if(result != S_OK) {
		LogMsg(L"D3D/Win32 operation failed, terminating!");
		std::exit(-1);
	}
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
	case WM_KEYDOWN:
		if(wParam == VK_ESCAPE) {
			if(MessageBox(0, L"Are you sure you want to exit?", L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES) {
				DestroyWindow(hwnd);
			}
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

HRESULT CompileShader(_In_ LPCWSTR srcFile, _In_ LPCSTR entryPoint, _In_ LPCSTR profile, _Outptr_ ID3DBlob** blob) {
	if(!srcFile || !entryPoint || !profile || !blob)
		return E_INVALIDARG;

	*blob = nullptr;

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	flags |= D3DCOMPILE_DEBUG;
#endif

	const D3D_SHADER_MACRO defines[] = {"EXAMPLE_DEFINE", "1", nullptr, nullptr};

	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	auto result = D3DCompileFromFile(srcFile, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint, profile, flags, 0, &shaderBlob, &errorBlob);

	if(FAILED(result)) {
		if(errorBlob) {
			LogMsg((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}
		if(shaderBlob) {
			shaderBlob->Release();
		}
		return result;
	}

	*blob = shaderBlob;
	return result;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	if(AllocConsole() == 0) {
		MessageBox(nullptr, L"Failed to create console, continuing without", L"Error", MB_OK);
	} else {
		consoleInput = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	WNDCLASSEX wc{};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = WndClassName;
	wc.hIconSm = LoadIcon(nullptr, IDI_WINLOGO);

	if(!RegisterClassEx(&wc)) {
		MessageBox(nullptr, L"Error registering class", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	hwnd = CreateWindowEx(
		0,
		WndClassName,
		L"DX11 Basic Example",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width,
		height,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	if(!hwnd) {
		MessageBox(nullptr, L"Error creating window", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	ShowWindow(hwnd, nShowCmd);
	UpdateWindow(hwnd);

	{
		TCHAR buffer[256];
		auto result = GetCurrentDirectory(256, buffer);
		std::wstring cwd{L"CWD: "};
		cwd += buffer;
		cwd += L"\n";
		LogMsg(cwd);
	}

	{
		IDXGIFactory* factory{};
		if(FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory))) {
			std::cout << "CreateDXGIFactory1 failed!" << std::endl;
			Terminate();
		}

		IDXGIAdapter* adapter{};
		DXGI_ADAPTER_DESC adaptorDesc{};
		for(UINT i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
			if(adapter->GetDesc(&adaptorDesc) != S_OK) {
				LogMsg(L"Failed to get description for adaptor");
				break;
			}
			std::wstring msg{};
			msg += L"== ADAPTOR DESCRIPTION =================";
			msg += L"\n";
			msg += L"DedicatedSystemMemory: ";
			msg += std::to_wstring(adaptorDesc.DedicatedSystemMemory);
			msg += L"\n";
			msg += L"DedicatedVideoMemory: ";
			msg += std::to_wstring(adaptorDesc.DedicatedVideoMemory);
			msg += L"\n";
			msg += L"Description: ";
			msg += adaptorDesc.Description;
			msg += L"\n";
			msg += L"DeviceId: ";
			msg += std::to_wstring(adaptorDesc.DeviceId);
			msg += L"\n";
			msg += L"Revision: ";
			msg += std::to_wstring(adaptorDesc.Revision);
			msg += L"\n";
			msg += L"SharedSystemMemory: ";
			msg += std::to_wstring(adaptorDesc.SharedSystemMemory);
			msg += L"\n";
			msg += L"SubSysId: ";
			msg += std::to_wstring(adaptorDesc.SubSysId);
			msg += L"\n";
			msg += L"VendorId: ";
			msg += std::to_wstring(adaptorDesc.VendorId);
			msg += L"\n";
			LogMsg(msg);
			adapter->Release();
		}

		factory->Release();
	}

	ID3D11Device* d3dDevice{};
	ID3D11DeviceContext* d3dDeviceCtx{};
	IDXGISwapChain* d3dSwapChain{};
	ID3D11RenderTargetView* renderTargetView{};

	{
		DXGI_MODE_DESC backBufferDesc{};
		ZeroMemory(&backBufferDesc, sizeof(DXGI_MODE_DESC));
		backBufferDesc.Width = width;
		backBufferDesc.Height = height;
		backBufferDesc.RefreshRate.Numerator = 60;
		backBufferDesc.RefreshRate.Denominator = 1;
		backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		backBufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		backBufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		// describes multi-sampling parameters for a resource (resource will be our swap chain)
		DXGI_SAMPLE_DESC sampleDesc{};
		ZeroMemory(&sampleDesc, sizeof(DXGI_SAMPLE_DESC));
		sampleDesc.Count = 1;
		sampleDesc.Quality = 0;

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
		swapChainDesc.BufferDesc = backBufferDesc;
		swapChainDesc.SampleDesc = sampleDesc;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2;
		swapChainDesc.OutputWindow = hwnd;
		swapChainDesc.Windowed = TRUE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		auto result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
		D3D11_SDK_VERSION, &swapChainDesc, &d3dSwapChain, &d3dDevice, nullptr, &d3dDeviceCtx);

		if(result != S_OK) {
			LogMsg(L"Failed to create display device; teminating program.");
			Terminate();
		}

		// get the backbuffer from the swap chain and set it as our render target
		ID3D11Texture2D* d3dBackBuffer{};
		Check(d3dSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&d3dBackBuffer));
		assert(d3dBackBuffer != nullptr);
		Check(d3dDevice->CreateRenderTargetView(d3dBackBuffer, nullptr, &renderTargetView));
		assert(renderTargetView != nullptr);
		d3dDeviceCtx->OMSetRenderTargets(1, &renderTargetView, nullptr);
	}

	struct Vertex {
		d3d::XMFLOAT3 pos;
		d3d::XMFLOAT4 color;
	};

	ID3D11InputLayout* inputLayout{};
	ID3D11Buffer* triangleVertBuffer{};
	ID3D11VertexShader* vertexShader{};
	ID3D11PixelShader* pixelShader{};

	{
		ID3DBlob* vsBlob = nullptr;
		HRESULT hr = CompileShader(L"../Source/VertexShader.hlsl", "main", "vs_5_0", &vsBlob);
		if(FAILED(hr)) {
			LogMsg("Failed compiling vertex shader");
			Terminate();
		}

		ID3DBlob* psBlob = nullptr;
		hr = CompileShader(L"../Source/PixelShader.hlsl", "main", "ps_5_0", &psBlob);
		if(FAILED(hr)) {
			LogMsg("Failed compiling pixel shader");
			Terminate();
		}

		if(d3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader) != S_OK) {
			LogMsg("Failed creating pixel shader");
			Terminate();
		}

		if(d3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader) != S_OK) {
			LogMsg("Failed creating pixel shader");
			Terminate();
		}

		d3dDeviceCtx->VSSetShader(vertexShader, 0, 0);
		d3dDeviceCtx->PSSetShader(pixelShader, 0, 0);

		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
		UINT numElements = ARRAYSIZE(layout);

		if(d3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout) != S_OK) {
			LogMsg("Failed creating input layout");
			Terminate();
		}

		Vertex triangle[] = {
			Vertex{{0.0f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
			Vertex{{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
			Vertex{{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
		};

		D3D11_BUFFER_DESC vertexBufferDesc;
		ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = sizeof(Vertex) * 3;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA vertexBufferData;
		ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
		vertexBufferData.pSysMem = triangle;
		if(d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &triangleVertBuffer) != S_OK) {
			LogMsg("Failed creating vertex buffer for triangle");
			Terminate();
		}

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		d3dDeviceCtx->IASetVertexBuffers(0, 1, &triangleVertBuffer, &stride, &offset);

		auto result = d3dDevice->CreateInputLayout(layout, numElements, vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(), &inputLayout);
		if(result != S_OK) {
			LogMsg("Failed creating input layout");
			Terminate();
		}

		d3dDeviceCtx->IASetInputLayout(inputLayout);
		d3dDeviceCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D11_VIEWPORT viewport;
		ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = width;
		viewport.Height = height;

		d3dDeviceCtx->RSSetViewports(1, &viewport);

		vsBlob->Release();
		psBlob->Release();
	}

	float rgb[4]{0.0f, 0.0f, 0.0f, 1.0f};
	float rgbSpeed[4]{0.00005f, 0.00002f, 0.00001f, 0.0f};
	int rgbDir[4]{1, 1, 1, 0};

	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	while(true) {
		if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if(msg.message == WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			for(int i{}; i < 3; ++i) {
				rgb[i] += rgbDir[i] * rgbSpeed[i];
				if(rgb[i] >= 1.0f || rgb[i] <= 0.0f)
					rgbDir[i] *= -1;
			}
			d3dDeviceCtx->ClearRenderTargetView(renderTargetView, rgb);
			d3dDeviceCtx->Draw(3, 0);
			d3dSwapChain->Present(0, 0);
		}
	}

	triangleVertBuffer->Release();
	vertexShader->Release();
	pixelShader->Release();
	vertexShader->Release();
	pixelShader->Release();
	inputLayout->Release();
	d3dSwapChain->Release();
	d3dDeviceCtx->Release();
	d3dDevice->Release();

	return 0;
}

