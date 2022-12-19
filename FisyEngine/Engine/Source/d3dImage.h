#pragma once
#include "d3dCore.h"
#include "WICCore.h"

class d3dImage
{
public:
	d3dImage();
	d3dImage(const WCHAR* fileName);

	void LoadImage(const WCHAR* fileName);
	void UpdateImageData(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12Resource* textureBuffer, ID3D12Resource* uploadHeap);

	UINT GetRowPitch() const { return m_imageHandle->GetRowPitch(); }
	UINT GetWidth() const { return m_imageHandle->GetWidth(); }
	UINT GetHeight() const { return m_imageHandle->GetHeight(); }
	UINT GetBpp() const { return m_imageHandle->GetBpp(); }
	DXGI_FORMAT GetFormat() const { return m_imageHandle->GetFormat(); }
	ComPtr<IWICBitmapSource> GetBmp() const { return m_imageHandle->GetBmp(); }

private:

	std::unique_ptr<WICCore> m_imageHandle = nullptr;
};

