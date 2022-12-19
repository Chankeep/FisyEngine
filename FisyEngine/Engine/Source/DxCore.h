#pragma once

#include <iostream>
#include "d3dUtil.h"
#include "d3dImage.h"
#include "d3dTimer.h"
#include "d3dcompiler.h"

#include "imguiManager.h"

constexpr UINT frameCount = 2;

class DxCore
{
public:
	DxCore() = default;
	DxCore(HWND hwnd);

	static bool isInitialized;

	void OnInit();
	void OnUpdate(XMMATRIX viewMat, XMMATRIX projMat);
	void OnRender();
	void OnDestroy();
	void OnResize(UINT newWidth, UINT newHeight);

	LPCWSTR GetAdapterName()
	{
		return adapterDesc.Description;
	}

	void present(UINT x, UINT y) { swapChain->Present(x, y); }

	void rotateBox(float deltaTime);

private:
	struct vertex
	{
		XMFLOAT3 position;
		XMFLOAT2 texcoord;
	};

	std::unique_ptr<d3dImage> textures[2];

	HWND dxHWND;
	DXGI_ADAPTER_DESC adapterDesc;

	UINT width = 800;
	UINT height = 600;
	UINT rtvDescriptorSize;
	UINT dsvDescriptorSize;

	CD3DX12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) };
	CD3DX12_RECT scissorRect{ 0, 0, width, height };

	ComPtr<IDXGISwapChain3> swapChain;
	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<ID3D12Device> device;
	ComPtr<IDXGIFactory4> dxgiFactory;
	ComPtr<ID3D12CommandQueue> cmdQueue;
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> cmdList;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;

	UINT frameIndex = 0;
	ComPtr<ID3D12Fence> fence;
	HANDLE fenceEvent;
	UINT fenceValue = 0;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	ComPtr<ID3D12DescriptorHeap> rtvDescHeap;
	ComPtr<ID3D12DescriptorHeap> dsvDescHeap;
	ComPtr<ID3D12DescriptorHeap> srvDescHeap;
	ComPtr<ID3D12DescriptorHeap> samplerDescHeap;
	ComPtr<ID3D12DescriptorHeap> imguiSrvDescHeap;
	ComPtr<ID3D12Resource> renderTargets[frameCount];
	ComPtr<ID3D12Resource> depthStencilBuffer;
	ComPtr<ID3D12Resource> textureBuffer;
	ComPtr<ID3D12Resource> uploadBuffer;
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> indexBuffer;
	ComPtr<ID3D12Resource> matrixBuffer;
	ComPtr<ID3D12Heap> texHeap;
	ImVec4 clearColor = { 0.5f, 0.7f, 1.0f, 1.0f };


	void LoadPipeline();
	void LoadAsset();
	void PopulateCommandList();
	void WaitForPreviousFrame();

	void InitializeDevice();
	void InitializeCommandQueue();
	void InitializeSwapChain();
	void InitializeRenderTargets();
	void InitializeDepthStencil();
	void InitializeCommandAllcator();

	double rotationYAngle = 0.0;
	double fPalstance = 10.0f * XM_PI / 180.0f;
};
