#pragma once
#include "DX12_Config.h"

class DX12_DeviceSystem {
DEFAULT_SINGLETON(DX12_DeviceSystem)
public:
	// Initialize DirectX 12 resources
	inline void Initialize() {
		InitDebugLayer();
		CreateFactory();
#if defined(DEBUG) || defined(_DEBUG) 
		LogAdapters();
#endif
		CreateDevice();
	}

	ID3D12Device* GetDevice() const {
		return mDevice.Get();
	}
	IDXGIFactory4* GetFactory() const {
		return mFactory.Get();
	}
private:
#if defined(DEBUG) || defined(_DEBUG) 
	Microsoft::WRL::ComPtr<ID3D12Debug> mDebugController;
	Microsoft::WRL::ComPtr<IDXGIDebug1> mDxgiDebug;
	Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings> mDredSettings;
#endif
	Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
	Microsoft::WRL::ComPtr<IDXGIFactory4> mFactory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> mAdapter;
	DXGI_FORMAT mSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	inline void InitDebugLayer()
	{
#if defined(DEBUG) || defined(_DEBUG) 
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(mDebugController.GetAddressOf())));
		ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(mDxgiDebug.GetAddressOf())));
		mDebugController->EnableDebugLayer();
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(mDredSettings.GetAddressOf()))))
		{
			mDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			mDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		}
		LOG_INFO("DirectX 12 Debug Layer Enabled");
#endif
	}
	inline void CreateFactory()
	{
		UINT dxgiFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
		ThrowIfFailed(CreateDXGIFactory2(_DEBUG, IID_PPV_ARGS(&mFactory)));
		LOG_INFO("DXGI Factory Created Successfully");
	}
	inline void CreateDevice()
	{
		if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)))) {
			LOG_WARN("Failed to create D3D12 device. Falling back to WARP adapter.");
			Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
			ThrowIfFailed(mFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
			ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)));
		}
		LOG_INFO("DirectX 12 Device Created Successfully");
	}

	inline void LogAdapters()
	{
		UINT i = 0;
		IDXGIAdapter* adapter = nullptr;
		std::vector<IDXGIAdapter*> adapterList;
		while (mFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);

			std::wstring text = L"***Adapter: ";
			text += desc.Description;
			LOG_INFO("{}", WStringToString(text));
			adapterList.push_back(adapter);

			++i;
		}

		for (i = 0; i < adapterList.size(); ++i)
		{
			LogAdapterOutputs(adapterList[i]);
			ReleaseCom(adapterList[i]);
		}
	}
	inline void LogAdapterOutputs(IDXGIAdapter* adapter)
	{
		UINT i = 0;
		IDXGIOutput* output = nullptr;
		while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_OUTPUT_DESC desc;
			output->GetDesc(&desc);

			std::wstring text = L"***Output: ";
			text += desc.DeviceName;
			LOG_INFO("{}", WStringToString(text));

			LogOutputDisplayModes(output, DXGI_FORMAT_R8G8B8A8_UNORM);

			ReleaseCom(output);

			++i;
		}
	}
	inline void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
	{
		UINT count = 0;
		UINT flags = 0;

		// Call with nullptr to get list count.
		output->GetDisplayModeList(format, flags, &count, nullptr);

		std::vector<DXGI_MODE_DESC> modeList(count);
		output->GetDisplayModeList(format, flags, &count, &modeList[0]);

		for (auto& x : modeList)
		{
			UINT n = x.RefreshRate.Numerator;
			UINT d = x.RefreshRate.Denominator;
			std::wstring text =
				L"Width = " + std::to_wstring(x.Width) + L" " +
				L"Height = " + std::to_wstring(x.Height) + L" " +
				L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d);

			LOG_INFO("{}", WStringToString(text));
		}
	}
};
