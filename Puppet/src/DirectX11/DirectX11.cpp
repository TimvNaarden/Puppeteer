#include "pch.h"
#include "DirectX11.h"



namespace Puppeteer
{
	DirectX11::DirectX11()
	{
		m_Device = nullptr;
		m_DeviceContext = nullptr;
		m_Factory1 = nullptr;
		m_Adapter = nullptr;
		m_Output = nullptr;
		m_Output1 = nullptr;
		m_DeskDupl = nullptr;
		m_AcquiredDesktopImage = nullptr;
		m_OutputDesc = {};

		m_FrameInfo = {};
		m_DesktopResource = nullptr;



		m_hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&m_Factory1));
		if (FAILED(m_hr)) {
			std::cout << "Failed to create DXGI Factory" << std::endl;
			return;
		}


		m_hr = m_Factory1->EnumAdapters(0, &m_Adapter);
		if (FAILED(m_hr)) {
			std::cout << "Failed to create adapter" << std::endl;
			return;
		}

		m_hr = m_Adapter->EnumOutputs(0, &m_Output);
		if (FAILED(m_hr)) {
			std::cout << "Failed to create output" << std::endl;
			return;
		}

		m_hr = m_Output->QueryInterface(__uuidof(m_Output1), (void**)(&m_Output1));
		if (FAILED(m_hr)) {
			std::cout << "Failed to create output1" << std::endl;
			return;
		}

		m_hr = D3D11CreateDevice(m_Adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &m_Device, &m_FeatureLevel, &m_DeviceContext);
		if (FAILED(m_hr)) {
			std::cout << "Failed to create D3D11 device" << std::endl;
			return;
		}

		m_hr = m_Output1->DuplicateOutput(m_Device, &m_DeskDupl);
		if (FAILED(m_hr)) {
			std::cout << "Failed to create output1" << std::endl;
			return;
		}

		m_hr = m_Output->GetDesc(&m_OutputDesc);
		if (FAILED(m_hr)) {
			std::cout << "Failed to get output description" << std::endl;
			return;
		}
	}

	void DirectX11::getScreen() {
		

		// Get new frame
		m_hr = m_DeskDupl->AcquireNextFrame(500, &m_FrameInfo, &m_DesktopResource);
		if (FAILED(m_hr)) {
			std::cout << "Failed to acquire next frame" << std::endl;
			return;
		}

		m_hr = m_DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_AcquiredDesktopImage));
		if (FAILED(m_hr)) {
			std::cout << "Failed to query interface" << std::endl;
			return;
		}
	}
}