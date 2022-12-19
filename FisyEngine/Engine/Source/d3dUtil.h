#pragma once
#include "d3dCore.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace d3dutil
{
	inline ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
		const void* initData, UINT64 byteSize,
		ComPtr<ID3D12Resource>& uploadBuffer)
	{
		ComPtr<ID3D12Resource> defaultBuffer;
		CD3DX12_HEAP_PROPERTIES defaultHeapPro = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_HEAP_PROPERTIES uploadHeapPro = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

		// 创建默认堆资源
		ThrowIfFailed(device->CreateCommittedResource(
			&defaultHeapPro,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

		// 上传堆资源
		ThrowIfFailed(device->CreateCommittedResource(
			&uploadHeapPro,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

		// 复制数据的描述
		D3D12_SUBRESOURCE_DATA subResourceData = {};
		subResourceData.pData = initData; //初始化数据
		subResourceData.RowPitch = byteSize; //复制的字节数
		subResourceData.SlicePitch = subResourceData.RowPitch; //对缓冲区而言，和上一行一样

		CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		UpdateSubresources(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
		cmdList->ResourceBarrier(1, &resBarrier);
		return defaultBuffer;
	}

	inline IDXGIAdapter1* GetSupportedAdapter(ComPtr<IDXGIFactory4>& dxgiFactory, const D3D_FEATURE_LEVEL featureLevel)
	{
		IDXGIAdapter1* adapter = nullptr;
		for (UINT adapterIndex = 0U; ; ++adapterIndex)
		{
			IDXGIAdapter1* currentAdapter = nullptr;
			if (dxgiFactory->EnumAdapters1(adapterIndex, &currentAdapter) == DXGI_ERROR_NOT_FOUND)
			{
				break;
			}

			const HRESULT hres = D3D12CreateDevice(currentAdapter, featureLevel, _uuidof(ID3D12Device), nullptr);
			if (SUCCEEDED(hres))
			{
				adapter = currentAdapter;
				break;
			}

			currentAdapter->Release();
		}

		return adapter;
	}

}