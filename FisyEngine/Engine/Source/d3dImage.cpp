#include "d3dImage.h"
d3dImage::d3dImage() {}

d3dImage::d3dImage(const WCHAR* fileName)
{
	m_imageHandle = std::make_unique<WICCore>(fileName);
}

void d3dImage::LoadImage(const WCHAR* fileName)
{
	m_imageHandle = std::make_unique<WICCore>(fileName);
}

void d3dImage::UpdateImageData(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12Resource* textureBuffer, ID3D12Resource* uploadHeap)
{
	const UINT RowPitch = GetRowPitch();
	const UINT64 n64UploadBufferSize = GetRequiredIntermediateSize(textureBuffer, 0, 1);

	void* m_imageData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n64UploadBufferSize);
	if (m_imageData == nullptr)
	{
		throw HrException(HRESULT_FROM_WIN32(GetLastError()));
	}

	ThrowIfFailed(GetBmp()->CopyPixels(
		nullptr,
		RowPitch,
		RowPitch * GetHeight(),
		static_cast<BYTE*>(m_imageData))
	);

	UINT64 n64RequiredSize = 0u;
	constexpr UINT nNumSubresources = 1u; //我们只有一副图片，即子资源个数为1
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT stTxtLayouts = {};
	UINT64 n64TextureRowSizes = 0u;
	UINT nTextureRowNum = 0u;

	D3D12_RESOURCE_DESC stDestDesc = textureBuffer->GetDesc();

	//这个方法几乎是我们复制纹理时必须要调用的方法
	//主要用它来得到目标纹理数据的真实尺寸信息
	//因为目标纹理如我们前面所描述的主要都是存储在默认堆上的
	//而CPU是无法直接访问它的
	//所以我们就需要这个方法作为桥梁让我们获知最终存储在默认堆中的纹理的详细尺寸信息

	device->GetCopyableFootprints(&stDestDesc
		, 0
		, nNumSubresources
		, 0
		, &stTxtLayouts
		, &nTextureRowNum
		, &n64TextureRowSizes
		, &n64RequiredSize);

	//因为上传堆实际就是CPU传递数据到GPU的中介
	//所以我们可以使用熟悉的Map方法将它先映射到CPU内存地址中
	//然后我们按行将数据复制到上传堆中
	//需要注意的是之所以按行拷贝是因为GPU资源的行大小
	//与实际图片的行大小是有差异的,二者的内存边界对齐要求是不一样的
	BYTE* pData = nullptr;
	ThrowIfFailed(uploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&pData)));

	BYTE* pDestSlice = reinterpret_cast<BYTE*>(pData) + stTxtLayouts.Offset;
	const auto pSrcSlice = static_cast<const BYTE*>(m_imageData);
	for (UINT y = 0; y < nTextureRowNum; ++y)
	{
		memcpy(pDestSlice + static_cast<SIZE_T>(stTxtLayouts.Footprint.RowPitch) * y
			, pSrcSlice + static_cast<SIZE_T>(RowPitch) * y
			, RowPitch);
	}
	//取消映射 对于易变的数据如每帧的变换矩阵等数据，可以撒懒不用Unmap了，
	//让它常驻内存,以提高整体性能，因为每次Map和Unmap是很耗时的操作
	//因为现在起码都是64位系统和应用了，地址空间是足够的，被长期占用不会影响什么
	uploadHeap->Unmap(0, nullptr);

	//释放图片数据，做一个干净的程序员
	HeapFree(GetProcessHeap(), 0, m_imageData);

	CD3DX12_TEXTURE_COPY_LOCATION Dst(textureBuffer, 0);
	CD3DX12_TEXTURE_COPY_LOCATION Src(uploadHeap, stTxtLayouts);
	cmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
}
