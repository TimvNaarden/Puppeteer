#include "pch.h"
#include "DirectX11.h"

namespace Puppeteer
{
	ID3D11Device* m_Device = nullptr;
	ID3D11DeviceContext* m_DeviceContext = nullptr;
	IDXGIFactory1* m_Factory1 = nullptr;
	IDXGIAdapter* m_Adapter = nullptr;
	IDXGIOutput* m_Output = nullptr;
	IDXGIOutput1* m_Output1 = nullptr;
	IDXGIOutputDuplication* m_DeskDupl = nullptr;


	DXGI_OUTDUPL_FRAME_INFO		m_FrameInfo = {};

	DXGI_OUTPUT_DESC 			m_OutputDesc = {};
	HRESULT						m_dxhr = S_OK;
	D3D11_MAPPED_SUBRESOURCE	screenCapSubRes = {};

	int DX11_init = 1;
	int DirextX11Counter = 0;

	DirectX11::DirectX11()
	{
		DirextX11Counter++;
		m_DesktopResource = nullptr;
		m_AcquiredDesktopImage = nullptr;

		if (DX11_init == 0) {
			std::cout << "DirectX11 already initialized" << std::endl;
			return;
		}

		DX11_init = 0;
		m_dxhr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&m_Factory1));
		if (FAILED(m_dxhr)) {
			std::cout << "Failed to create DXGI Factory" << std::endl;
			return;
		}


		m_dxhr = m_Factory1->EnumAdapters(0, &m_Adapter);
		if (FAILED(m_dxhr)) {
			std::cout << "Failed to create adapter" << std::endl;
			return;
		}

		m_dxhr = m_Adapter->EnumOutputs(0, &m_Output);
		if (FAILED(m_dxhr)) {
			std::cout << "Failed to create output" << std::endl;
			return;
		}

		m_dxhr = m_Output->QueryInterface(__uuidof(m_Output1), (void**)(&m_Output1));
		if (FAILED(m_dxhr)) {
			std::cout << "Failed to create output1" << std::endl;
			return;
		}

		m_dxhr = D3D11CreateDevice(m_Adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &m_Device, &m_FeatureLevel, &m_DeviceContext);
		if (FAILED(m_dxhr)) {
			std::cout << "Failed to create D3D11 device" << std::endl;
			return;
		}

		m_dxhr = m_Output1->DuplicateOutput(m_Device, &m_DeskDupl);
		if (FAILED(m_dxhr)) {
			std::cout << "Failed to create output1" << std::endl;
			return;
		}

		m_dxhr = m_Output->GetDesc(&m_OutputDesc);
		if (FAILED(m_dxhr)) {
			std::cout << "Failed to get output description" << std::endl;
			return;
		}
	}
	DirectX11::~DirectX11()
	{
		DirextX11Counter--;
		if(DirextX11Counter != 0) return;
		if (m_DeskDupl) m_DeskDupl->Release();
		if (m_Output1) m_Output1->Release();
		if (m_Output) m_Output->Release();
		if (m_Adapter) m_Adapter->Release();
		if (m_Factory1) m_Factory1->Release();
		if (m_DeviceContext) m_DeviceContext->Release();
		if (m_Device) m_Device->Release();
	}
	screenCapture DirectX11::getScreen() {

		screenCapture screenCap = {};

		// Get new frame
		if (m_DeskDupl == nullptr) {
			std::cout << "DeskDupl is null" << std::endl;
			m_dxhr = m_Output1->DuplicateOutput(m_Device, &m_DeskDupl);
			if (FAILED(m_dxhr)) {
				std::cout << "Failed to create output1" << std::endl;
				return screenCap;
			}

			m_dxhr = m_Output->GetDesc(&m_OutputDesc);
			if (FAILED(m_dxhr)) {
				std::cout << "Failed to get output description" << std::endl;
				return screenCap;
			}
			return screenCap;
		}
		m_dxhr = m_DeskDupl->AcquireNextFrame(500, &m_FrameInfo, &m_DesktopResource);
		if (FAILED(m_dxhr)) {
			std::cout << "Failed to acquire next frame" << std::endl;
			return screenCap;
		}

		//Map pixels
		m_dxhr = m_DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_AcquiredDesktopImage));
		if (FAILED(m_dxhr)) {
			std::cout << "Failed to query interface" << std::endl;
			return screenCap;
		}

		// Create a second texture that can be accessed by the CPU
		D3D11_TEXTURE2D_DESC desc;
		m_AcquiredDesktopImage->GetDesc(&desc);
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.BindFlags = 0;
		desc.MiscFlags = 0;

		screenCap.width = desc.Width;
		screenCap.height = desc.Height;

		// Map into GLuint
		ID3D11Texture2D* pTexture;
		m_dxhr = m_Device->CreateTexture2D(&desc, NULL, &pTexture);
		if (FAILED(m_dxhr)) {
			std::cout << "Failed to create texture" << std::endl;
			return screenCap;
		}

		if (m_AcquiredDesktopImage) m_DeviceContext->CopyResource(pTexture, m_AcquiredDesktopImage);
		else return screenCap;

		m_DeviceContext->Unmap(pTexture, 0);
		m_dxhr = m_DeviceContext->Map(pTexture, 0, D3D11_MAP_READ, 0, &screenCapSubRes);
		if (FAILED(m_dxhr)) {
			std::cout << "Failed to map resource" << std::endl;
			return screenCap;
		}
		screenCap.size = screenCapSubRes.RowPitch * desc.Height;
		//result.data = resource;	
		pTexture->Release();
		m_AcquiredDesktopImage->Release();
		m_DeskDupl->ReleaseFrame();
		m_DesktopResource->Release();
		return screenCap;
	}
}