#include "DxCore.h"

UINT8* pCbvDataBegin;
bool DxCore::isInitialized = false;

struct Matrix_Buffer
{
	XMFLOAT4X4 mvp_matrix;
};

Matrix_Buffer mvpBuffer;

DxCore::DxCore(HWND hwnd) : dxHWND(hwnd)
{
	textures[0] = std::make_unique<d3dImage>(L"Engine\\Asset\\Texture\\Brick_Diffuse.jpg");
}

void DxCore::OnInit()
{
	LoadPipeline();
	LoadAsset();

	//create imgui srvHeap
	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
	cbvSrvHeapDesc.NumDescriptors = 1;
	cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&imguiSrvDescHeap)));

	ImGui_ImplDX12_Init(device.Get(), frameCount,
		DXGI_FORMAT_R8G8B8A8_UNORM, imguiSrvDescHeap.Get(),
		imguiSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		imguiSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

	isInitialized = true;
}

void DxCore::OnUpdate(XMMATRIX viewMat, XMMATRIX projMat)
{
	if (rotationYAngle > XM_2PI)
	{
		rotationYAngle = fmod(rotationYAngle, XM_2PI);
	}

	XMMATRIX Mrotation = XMMatrixRotationX(static_cast<float>(rotationYAngle));

	XMMATRIX viewProj = XMMatrixMultiply(viewMat, projMat);

	XMMATRIX MVP = XMMatrixMultiply(Mrotation, viewProj);

	Matrix_Buffer nmvpBuffer;
	XMStoreFloat4x4(&nmvpBuffer.mvp_matrix, XMMatrixTranspose(MVP));
	memcpy(pCbvDataBegin, &nmvpBuffer, sizeof(nmvpBuffer));
}

void DxCore::OnRender()
{
	PopulateCommandList();
	ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailed(swapChain->Present(1, 0));
	WaitForPreviousFrame();
}

void DxCore::OnDestroy()
{
	WaitForPreviousFrame();
	CloseHandle(fenceEvent);
}

void DxCore::OnResize(UINT newWidth, UINT newHeight)
{
	WaitForPreviousFrame();
	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), pipelineState.Get()));

	//Reset the renderTargetView and depthStencilView
	for (UINT n = 0; n < frameCount; n++)
	{
		renderTargets[n].Reset();
	}
	depthStencilBuffer.Reset();

	//Resize the swap chain
	ThrowIfFailed(swapChain->ResizeBuffers(
		frameCount,
		newWidth,
		newHeight,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
	frameIndex = swapChain->GetCurrentBackBufferIndex();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT n = 0; n < frameCount; n++)
	{
		ThrowIfFailed(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
		device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, rtvDescriptorSize);
	}

	CD3DX12_RESOURCE_DESC tex2D = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
		newWidth,
		newHeight,
		1,
		0,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	auto dsvHeapPro = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	ThrowIfFailed(device->CreateCommittedResource(
		&dsvHeapPro,
		D3D12_HEAP_FLAG_NONE,
		&tex2D,
		D3D12_RESOURCE_STATE_COMMON,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(depthStencilBuffer.GetAddressOf())));
	//Create the depthStencilView
	device->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr,
		dsvDescHeap->GetCPUDescriptorHandleForHeapStart());

	auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);

	cmdList->ResourceBarrier(1, &resourceBarrier);

	WaitForPreviousFrame();

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(newWidth);
	viewport.Height = static_cast<float>(newHeight);
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;

	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = newWidth;
	scissorRect.bottom = newHeight;

	width = newWidth;
	height = newHeight;

	ThrowIfFailed(cmdList->Close());
}

void DxCore::rotateBox(float deltaTime)
{
	rotationYAngle += deltaTime * fPalstance;
}

float speed = 0.01f;

void DxCore::LoadPipeline()
{
	InitializeDevice();
	InitializeCommandQueue();
	InitializeSwapChain();
	InitializeRenderTargets();
	InitializeDepthStencil();
	InitializeCommandAllcator();

	//创建常量缓冲区、着色器资源视图描述符堆
	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
	cbvSrvHeapDesc.NumDescriptors = 2;
	cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&srvDescHeap)));

	//动态采样器描述符堆
	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
	samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	samplerHeapDesc.NumDescriptors = 5;
	ThrowIfFailed(device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&samplerDescHeap)));
}

void DxCore::LoadAsset()
{
	//创建根签名
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		CD3DX12_DESCRIPTOR_RANGE1 descRanges[3];
		descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
		descRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
		descRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[3];
		rootParameters[0].InitAsDescriptorTable(1,
			&descRanges[0],
			D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsDescriptorTable(1,
			&descRanges[1],
			D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[2].InitAsDescriptorTable(1,
			&descRanges[2],
			D3D12_SHADER_VISIBILITY_ALL);


		//之前通过静态参数化的方式添加静态采样器
		// CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		// rootSignatureDesc.Init_1_1(_countof(rootParameters),
		// 	rootParameters,
		// 	1,
		// 	&samplerDesc,
		// 	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		//这里有两个之前创建的根参数，一个是sampler的，一个是SRV当前也就是我们用的纹理的
		//具体在渲染的时候指定sampler堆就可以
		rootSignatureDesc.Init_1_1(_countof(rootParameters),
			rootParameters,
			0,
			nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc,
			featureData.HighestVersion,
			&signature,
			&error));
		ThrowIfFailed(device->CreateRootSignature(0,
			signature->GetBufferPointer(),
			signature->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature)));
	}

	{
		//静态采样器
		// D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
		// samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		// samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		// samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		// samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		// samplerDesc.MipLODBias = 0;
		// samplerDesc.MaxAnisotropy = 0;
		// samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		// samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		// samplerDesc.MinLOD = 0.0f;
		// samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		// samplerDesc.ShaderRegister = 0;
		// samplerDesc.RegisterSpace = 0;
		// samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		//创建动态采样器
		auto samplerDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		CD3DX12_CPU_DESCRIPTOR_HANDLE SamplerHeapHandle(samplerDescHeap->GetCPUDescriptorHandleForHeapStart());

		D3D12_SAMPLER_DESC samplerDesc = {};
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.MipLODBias = 0.0f;

		//sampler1
		samplerDesc.BorderColor[0] = 1.0f;
		samplerDesc.BorderColor[1] = 0.0f;
		samplerDesc.BorderColor[2] = 1.0f;
		samplerDesc.BorderColor[3] = 1.0f;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		device->CreateSampler(&samplerDesc, SamplerHeapHandle);

		//偏移Handle的地址到准备存下一个采样器的地址
		SamplerHeapHandle.Offset(samplerDescSize);

		//sampler2
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		device->CreateSampler(&samplerDesc, SamplerHeapHandle);

		//偏移Handle的地址到准备存下一个采样器的地址
		SamplerHeapHandle.Offset(samplerDescSize);

		//sampler3
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		device->CreateSampler(&samplerDesc, SamplerHeapHandle);

		//偏移Handle的地址到准备存下一个采样器的地址
		SamplerHeapHandle.Offset(samplerDescSize);

		//sampler4
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		device->CreateSampler(&samplerDesc, SamplerHeapHandle);

		//偏移Handle的地址到准备存下一个采样器的地址
		SamplerHeapHandle.Offset(samplerDescSize);

		//sampler5
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		device->CreateSampler(&samplerDesc, SamplerHeapHandle);

		//后面没有要创建的采样器了所以不用继续偏移地址
		//后续要做的就是在创建根签名时明确指出我们需要的是动态采样器
	}
	{
		//编译着色器
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ThrowIfFailed(D3DCompileFromFile(std::wstring(L"Engine\\Shader\\VS.hlsl").c_str(), nullptr, nullptr, "main",
			"vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(std::wstring(L"Engine\\Shader\\PS.hlsl").c_str(), nullptr, nullptr, "main",
			"ps_5_0", compileFlags, 0, &pixelShader, nullptr));

		//创建顶点输入布局
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[]
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			// {"NORMAL", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};

		//创建管线状态对象
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = rootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
	}
	//创建命令列表，创建完成后关闭命令列表
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr,
		IID_PPV_ARGS(&cmdList)));

	ThrowIfFailed(cmdList->Close());

	//创建围栏
	{
		ThrowIfFailed(device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		fenceValue = 1;

		fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
	}

	{
		float fBoxSize = 2.0f;
		float fTCMax = 3.0f;
		//创建并加载顶点缓冲区
		vertex Vertices[] =
		{
			{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
			{{-1.0f, +1.0f, -1.0f}, {3.0f, 3.0f}},
			{{+1.0f, +1.0f, -1.0f}, {0.0f, 3.0f}},
			{{+1.0f, -1.0f, -1.0f}, {3.0f, 0.0f}},
			{{-1.0f, -1.0f, +1.0f}, {0.0f, 3.0f}},
			{{-1.0f, +1.0f, +1.0f}, {3.0f, 0.0f}},
			{{+1.0f, +1.0f, +1.0f}, {0.0f, 0.0f}},
			{{+1.0f, -1.0f, +1.0f}, {3.0f, 3.0f}}
		};

		DWORD Indexs[] =
		{
			// front face
			0, 1, 2,
			0, 2, 3,

			// back face
			4, 6, 5,
			4, 7, 6,

			// left face
			4, 5, 1,
			4, 1, 0,

			// right face
			3, 2, 6,
			3, 6, 7,

			// top face
			1, 5, 6,
			1, 6, 2,

			// bottom face
			4, 0, 3,
			4, 3, 7
		};

		const UINT vertexBufferSize = sizeof(Vertices);
		const UINT indexBufferSize = sizeof(Indexs);

		cmdList->Reset(cmdAllocator.Get(), pipelineState.Get());

		vertexBuffer = d3dutil::CreateDefaultBuffer(device.Get(), cmdList.Get(), Vertices, vertexBufferSize,
			uploadBuffer);

		indexBuffer = d3dutil::CreateDefaultBuffer(device.Get(), cmdList.Get(), Indexs, indexBufferSize,
			uploadBuffer);

		// 方便调试的时候查看信息
		uploadBuffer->SetName(L"Vertex Buffer Upload Resource Heap");
		indexBuffer->SetName(L"Index Buffer Resource Heap");
		vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

		//创建顶点缓冲区视图
		vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(vertex);
		vertexBufferView.SizeInBytes = vertexBufferSize;

		//创建索引缓冲区视图
		indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
		indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		indexBufferView.SizeInBytes = indexBufferSize;

		ThrowIfFailed(cmdList->Close());
		ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
		cmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		WaitForPreviousFrame();
	}
	{
		//以定位placed方式创建资源纹理
		//先创建好能存纹理数据的堆，目前应该理解为申请了一块GPU可访问的存储
		UINT RowPitch = textures[0]->GetRowPitch();
		D3D12_HEAP_DESC texHeapDesc = {};
		texHeapDesc.SizeInBytes = GRS_UPPER(2 * RowPitch * textures[0]->GetHeight(),
			D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		texHeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		texHeapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
		texHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		texHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		texHeapDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS;
		ThrowIfFailed(device->CreateHeap(&texHeapDesc, IID_PPV_ARGS(&texHeap)));

		//创建好堆之后就可以使用CreatePlacedResource方法来创建具体的纹理资源了
		//创建提交committed方式的资源只需要创建一个D3D12_HEAP_TYPE_DEFAULT为参数的CD3DX12_HEAP_PROPERTIES
		//并传入参数即可
		//纹理资源描述符
		CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			textures[0]->GetFormat(),
			textures[0]->GetWidth(),
			textures[0]->GetHeight(),
			1, 1, 1, 0);

		ThrowIfFailed(device->CreatePlacedResource(
			texHeap.Get(),
			0,
			&texDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&textureBuffer)));

		UINT64 n64BufferOffset = GetRequiredIntermediateSize(textureBuffer.Get(), 0, 1);

		//默认堆上的纹理创建好了，就需要创建独立的上传堆了，也使用定位方式，类似默认堆
		//创建纹理上传堆
		ComPtr<ID3D12Heap> uploadHeap;
		D3D12_HEAP_DESC uploadHeapDesc = {};
		uploadHeapDesc.SizeInBytes = GRS_UPPER(2 * RowPitch * textures[0]->GetHeight(),
			D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
		uploadHeapDesc.Alignment = 0;
		uploadHeapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
		uploadHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		uploadHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		uploadHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		ThrowIfFailed(device->CreateHeap(&uploadHeapDesc, IID_PPV_ARGS(&uploadHeap)));

		CD3DX12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(n64BufferOffset);
		//把纹理数据存到上传堆中
		//先在上传堆中创建对应的纹理资源用于存放
		ThrowIfFailed(device->CreatePlacedResource(
			uploadHeap.Get(),
			0,
			&uploadDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)));

		cmdList->Reset(cmdAllocator.Get(), pipelineState.Get());
		//使用封装的函数把纹理数据上传到默认堆
		textures[0]->UpdateImageData(device.Get(), cmdList.Get(), textureBuffer.Get(), uploadBuffer.Get());

		//把纹理资源的状态转换为shader资源状态
		CD3DX12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			textureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		cmdList->ResourceBarrier(1, &resBarrier);

		ThrowIfFailed(cmdList->Close());
		ID3D12CommandList* ppCommandLists[] = { cmdList.Get() };
		cmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		WaitForPreviousFrame();

		D3D12_SHADER_RESOURCE_VIEW_DESC srcDesc = {};
		srcDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srcDesc.Format = texDesc.Format;
		srcDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srcDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(textureBuffer.Get(), &srcDesc,
			srvDescHeap->GetCPUDescriptorHandleForHeapStart());

		//创建常量缓冲constant buffer
		SIZE_T szMVPBuffer = GRS_UPPER_DIV(sizeof(Matrix_Buffer), 256) * 256;
		auto matResDesc = CD3DX12_RESOURCE_DESC::Buffer(szMVPBuffer);
		ThrowIfFailed(device->CreatePlacedResource(
			uploadHeap.Get(),
			n64BufferOffset,
			&matResDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)));
		ThrowIfFailed(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pCbvDataBegin)));
		memcpy(pCbvDataBegin, &mvpBuffer, sizeof(mvpBuffer));

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = uploadBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = static_cast<UINT>(szMVPBuffer);

		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvsrvHandle(srvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			1, device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		device->CreateConstantBufferView(&cbvDesc, cbvsrvHandle);
	}
}

void DxCore::PopulateCommandList()
{
	//先重置命令列表和命令分配器
	ThrowIfFailed(cmdAllocator->Reset());
	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), pipelineState.Get()));

	D3D12_RESOURCE_BARRIER resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmdList->ResourceBarrier(1, &resBarrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex,
		rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvDescHeap->GetCPUDescriptorHandleForHeapStart());

	cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);

	//渲染指令：
	const float clearColorWithAlpha[4] = {
		clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w
	};
	cmdList->ClearRenderTargetView(rtvHandle, clearColorWithAlpha, 0, nullptr);
	cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//设置图形根签名、视口、裁剪矩形
	cmdList->SetGraphicsRootSignature(rootSignature.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { srvDescHeap.Get(), samplerDescHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	//发出渲染命令前时使用根参数指定资源位置
	//指定描述符表中SRV的Handle在GPU显存中的位置，不需要偏移handle因为描述符堆中第一个就是创建的srv
	cmdList->SetGraphicsRootDescriptorTable(0, srvDescHeap->GetGPUDescriptorHandleForHeapStart());

	///接下来继续指定采样器描述符表，在根参数中是第二个元素，采样器全放在一个描述符堆中
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUSampler(
		samplerDescHeap->GetGPUDescriptorHandleForHeapStart(),
		1,
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));
	cmdList->SetGraphicsRootDescriptorTable(1, GPUSampler);
	//第三个元素就是我们的常量缓冲区cbv
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUcbvHandle(srvDescHeap->GetGPUDescriptorHandleForHeapStart(), 1,
		device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	cmdList->SetGraphicsRootDescriptorTable(2, GPUcbvHandle);

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
	cmdList->IASetIndexBuffer(&indexBufferView);

	//Imgui render
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	bool show_demo_window = true;
	ImGui::ShowDemoWindow(&show_demo_window);

	{
		ImGui::Begin("Clear Control Pannel");
		ImGui::ColorEdit4("Clear Coloer", (float*)&clearColor);
		// ImGui::DragFloat3("Eye Pos", (float*)Eye.m128_f32, 0.01);
		ImGui::End();
	}

	ImGui::Render();

	//draw call
	cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);

	//Imgui commit command
	cmdList->SetDescriptorHeaps(1, imguiSrvDescHeap.GetAddressOf());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.Get());

	resBarrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	cmdList->ResourceBarrier(1, &resBarrier);

	ThrowIfFailed(cmdList->Close());
}

void DxCore::WaitForPreviousFrame()
{
	const UINT64 tempFenceValue = fenceValue;
	ThrowIfFailed(cmdQueue->Signal(fence.Get(), tempFenceValue));
	fenceValue++;

	if (fence->GetCompletedValue() < tempFenceValue)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(tempFenceValue, fenceEvent));
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	frameIndex = swapChain->GetCurrentBackBufferIndex();
}

void DxCore::InitializeDevice()
{
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
	}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	IDXGIAdapter1* m_adapter = nullptr;
	for (std::uint32_t i = 0U; i < _countof(featureLevels); ++i)
	{
		m_adapter = d3dutil::GetSupportedAdapter(dxgiFactory, featureLevels[i]);
		if (m_adapter != nullptr)
		{
			break;
		}
	}

	if (m_adapter != nullptr)
	{
		D3D12CreateDevice(m_adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(device.GetAddressOf()));
	}
	m_adapter->GetDesc(&adapterDesc);
}

void DxCore::InitializeCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue)));
}

void DxCore::InitializeSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = frameCount;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
		cmdQueue.Get(),
		dxHWND,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	));

	ThrowIfFailed(swapChain1.As(&swapChain));
	frameIndex = swapChain->GetCurrentBackBufferIndex();
}

void DxCore::InitializeRenderTargets()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = frameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescHeap)));

	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT n = 0; n < frameCount; n++)
	{
		ThrowIfFailed(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
		device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, rtvDescriptorSize);
	}
}

void DxCore::InitializeDepthStencil()
{
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvDescHeap)));

		dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES dsvHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC tex2D = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
		width,
		height,
		1,
		0,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	device->CreateCommittedResource(
		&dsvHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&tex2D,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&depthStencilBuffer));
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvDescHeap->GetCPUDescriptorHandleForHeapStart());

	device->CreateDepthStencilView(depthStencilBuffer.Get(), &depthStencilDesc, dsvHandle);
}

void DxCore::InitializeCommandAllcator()
{
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
}
