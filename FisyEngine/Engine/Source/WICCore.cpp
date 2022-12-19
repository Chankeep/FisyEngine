#include "WICCore.h"

#include <iostream>

WICCore::WICCore(const WCHAR* fileName)
{
	ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&m_wicFactory)));
	ThrowIfFailed(m_wicFactory->CreateDecoderFromFilename(
		fileName,
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnDemand,
		&m_wicDecoder)
	);
	// 获取第一帧图片(因为GIF等格式文件可能会有多帧图片，其他的格式一般只有一帧图片)
	// 实际解析出来的往往是位图格式数据
	ThrowIfFailed(m_wicDecoder->GetFrame(0, &m_wicFrame));

	WICPixelFormatGUID wpf = {};

	//获取WIC图片格式
	ThrowIfFailed(m_wicFrame->GetPixelFormat(&wpf));
	GUID tgFormat = {};

	//通过第一道转换之后获取DXGI的等价格式
	if (GetTargetPixelFormat(&wpf, &tgFormat))
	{
		m_texFormat = GetDXGIFormatFromPixelFormat(&tgFormat);
	}

	if (DXGI_FORMAT_UNKNOWN == m_texFormat)
	{
		// 不支持的图片格式 目前退出了事 
		// 一般 在实际的引擎当中都会提供纹理格式转换工具，
		// 图片都需要提前转换好，所以不会出现不支持的现象
		throw HrException(S_FALSE);
	}


	if (!InlineIsEqualGUID(wpf, tgFormat))
	{
		// 这个判断很重要，如果原WIC格式不是直接能转换为DXGI格式的图片时
		// 我们需要做的就是转换图片格式为能够直接对应DXGI格式的形式
		//创建图片格式转换器
		ComPtr<IWICFormatConverter> m_converter;
		ThrowIfFailed(m_wicFactory->CreateFormatConverter(&m_converter));

		//初始化一个图片转换器，实际也就是将图片数据进行了格式转换
		ThrowIfFailed(m_converter->Initialize(
			m_wicFrame.Get(), // 输入原图片数据
			tgFormat, // 指定待转换的目标格式
			WICBitmapDitherTypeNone, // 指定位图是否有调色板，现代都是真彩位图，不用调色板，所以为None
			NULL, // 指定调色板指针
			0.f, // 指定Alpha阀值
			WICBitmapPaletteTypeCustom // 调色板类型，实际没有使用，所以指定为Custom
		));
		// 调用QueryInterface方法获得对象的位图数据源接口
		ThrowIfFailed(m_converter.As(&m_bmp));
	}
	else
	{
		//图片数据格式不需要转换，直接获取其位图数据源接口
		ThrowIfFailed(m_wicFrame.As(&m_bmp));
	}
	//获得图片大小（单位：像素）
	ThrowIfFailed(m_bmp->GetSize(&m_texWidth, &m_texHeight));

	//获取图片像素的位大小的BPP（Bits Per Pixel）信息，用以计算图片行数据的真实大小（单位：字节）
	ComPtr<IWICComponentInfo> pIWICmntinfo;
	ThrowIfFailed(m_wicFactory->CreateComponentInfo(tgFormat, pIWICmntinfo.GetAddressOf()));

	WICComponentType type;
	ThrowIfFailed(pIWICmntinfo->GetComponentType(&type));

	if (type != WICPixelFormat)
	{
		throw HrException(S_FALSE);
	}

	ComPtr<IWICPixelFormatInfo> pIWICPixelinfo;
	ThrowIfFailed(pIWICmntinfo.As(&pIWICPixelinfo));

	// 到这里终于可以得到BPP了，这也是我看的比较吐血的地方，为了BPP居然饶了这么多环节
	ThrowIfFailed(pIWICPixelinfo->GetBitsPerPixel(&m_bppNum));

	// 计算图片实际的行大小（单位：字节），这里使用了一个上取整除法即（A+B-1）/B ，
	// 这曾经被传说是微软的面试题,希望你已经对它了如指掌
	m_PicRowPitch = (uint64_t(m_texWidth) * uint64_t(m_bppNum) + 7u) / 8u;
}
