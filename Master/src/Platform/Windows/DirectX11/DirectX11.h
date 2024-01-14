#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include <imgui.h>
#include <glad/glad.h>

namespace Puppeteer
{
	class DirectX11
	{
	public:
		GLuint						m_Texture;

		D3D_FEATURE_LEVEL			m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;

		DirectX11();
		~DirectX11() = default;

		ImTextureID getScreen();

	private:
		ID3D11Device*				m_Device;
		ID3D11DeviceContext*		m_DeviceContext;
		IDXGIFactory1*				m_Factory1;
		IDXGIAdapter*				m_Adapter;
		IDXGIOutput*				m_Output;
		IDXGIOutput1*				m_Output1;
		IDXGIOutputDuplication*		m_DeskDupl;

		ID3D11Texture2D*			m_AcquiredDesktopImage;
		IDXGIResource*				m_DesktopResource;
		DXGI_OUTDUPL_FRAME_INFO		m_FrameInfo;

		DXGI_OUTPUT_DESC 			m_OutputDesc;
		HRESULT						m_hr = S_OK;
	};
}

