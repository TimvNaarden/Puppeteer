#pragma once

#include <iostream>

#include <d3d11.h>
#include <dxgi1_2.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")


namespace Puppeteer
{
	extern D3D11_MAPPED_SUBRESOURCE screenCapSubRes;
	
	typedef struct screenCapture {
		float width;
		float height;
		size_t size;
	} screenCapture;
	
	class DirectX11 {
	public:
		D3D_FEATURE_LEVEL			m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;

		DirectX11();
		~DirectX11();

		screenCapture getScreen();

	private:
		ID3D11Texture2D* m_AcquiredDesktopImage;
		IDXGIResource* m_DesktopResource;
	};
}

