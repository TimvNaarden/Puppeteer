#pragma once

#include <iostream>

#include <d3d11.h>
#include <dxgi1_2.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#define DIRECTX11(x) std::cout << "DirectX11: " << x << std::endl;

namespace Puppeteer {

	extern D3D11_MAPPED_SUBRESOURCE screenCapSubRes;
	
	typedef struct screenCapture {
		int width;
		int height;
		size_t size;
	} screenCapture;
	
	class DirectX11 {
		public:
			DirectX11();
			~DirectX11();

			screenCapture getScreen();

		private:
			HRESULT m_HR;
			ID3D11Texture2D* m_AcquiredDesktopImage;
			IDXGIResource* m_DesktopResource;
			D3D_FEATURE_LEVEL m_FeatureLevel;
	};
}

