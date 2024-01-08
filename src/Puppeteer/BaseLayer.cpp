#include "pch.h"
#include "BaseLayer.h"

#include "Core/Application.h"

#include <imgui.h>
#include <glad/glad.h>
#include <Windows.h>

#include <d3d11.h>
#include <dxgi1_2.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")


namespace Puppeteer
{
	ID3D11Device*			device;
	ID3D11DeviceContext*	deviceContext;
	IDXGIOutputDuplication* deskDupl;
	ID3D11Texture2D*		AcquiredDesktopImage;
	IDXGIOutput*			Output;
	IDXGIOutput1*			Output1;
	DXGI_OUTPUT_DESC 		OutputDesc;
	bool init = false;

	static bool initd3d11() {
		init = true;
		HRESULT hr = S_OK;
		IDXGIFactory1* DXGIFactory1 = nullptr;
		IDXGIAdapter* Adapter = nullptr;
		D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;
		Output = nullptr;
		deskDupl = nullptr;


		hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&DXGIFactory1));
		if (FAILED(hr)) {
			std::cout << "Failed to create DXGI Factory" << std::endl;
			return false;
		}


		hr = DXGIFactory1->EnumAdapters(0, &Adapter);
		if (FAILED(hr)) {
			std::cout << "Failed to create adapter" << std::endl;
			return false;
		}

		hr = Adapter->EnumOutputs(0, &Output);
		if (FAILED(hr)) {
			std::cout << "Failed to create output" << std::endl;
			return false;
		}

		hr = Output->QueryInterface(__uuidof(Output1), (void**)(&Output1));
		if (FAILED(hr)) {
			std::cout << "Failed to create output1" << std::endl;
			return false;
		}

		hr = D3D11CreateDevice(Adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &device, &FeatureLevel, &deviceContext);
		if (FAILED(hr)) {
			std::cout << "Failed to create D3D11 device" << std::endl;
			return false;
		}

		hr = Output1->DuplicateOutput(device, &deskDupl);
		if (FAILED(hr)) {
			std::cout << "Failed to create output1" << std::endl;
			return false;
		}

		hr = Output->GetDesc(&OutputDesc);
		if (FAILED(hr)) {
			std::cout << "Failed to get output description" << std::endl;
			return false;
		}



		return true;
	}
	GLuint m_texture;
	static ImTextureID getScreenUsingDuplicationApi() {
		if (!init) initd3d11();
		if (m_texture) glDeleteTextures(1, &m_texture);

		HRESULT hr = S_OK;

		IDXGIResource* DesktopResource;
		DXGI_OUTDUPL_FRAME_INFO FrameInfo;

		// Get new frame
		hr = deskDupl->AcquireNextFrame(1, &FrameInfo, &DesktopResource);
		if (FAILED(hr)) {
			std::cout << "Failed to acquire next frame" << std::endl;
			return nullptr;
		}

		//Map pixels
		DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&AcquiredDesktopImage));

		if (FAILED(hr)) {
			std::cout << "Failed to query interface" << std::endl;
			return nullptr;
		}

		// Create a second texture that can be accessed by the CPU
		
		D3D11_TEXTURE2D_DESC desc;
		AcquiredDesktopImage->GetDesc(&desc);
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.BindFlags = 0;
		desc.MiscFlags = 0;
		


		
		// Map into GLuint
		ID3D11Texture2D* pTexture;
		hr = device->CreateTexture2D(&desc, NULL, &pTexture);
		if (FAILED(hr)) {
			std::cout << "Failed to create texture" << std::endl;
			return nullptr;
		}

		if(AcquiredDesktopImage) deviceContext->CopyResource(pTexture, AcquiredDesktopImage);
		else return nullptr;

		D3D11_MAPPED_SUBRESOURCE resource;
		hr = deviceContext->Map(pTexture, 0, D3D11_MAP_READ, 0, &resource);
		if (FAILED(hr)) {
			std::cout << "Failed to map resource" << std::endl;
			return nullptr;
		}

		// Flip image
		UINT rowPitch = desc.Width * 4;
		BYTE* flippedData = new BYTE[resource.RowPitch * desc.Height];

		for (UINT y = 0; y < desc.Height; ++y) {
			memcpy(flippedData + y * rowPitch, static_cast<BYTE*>(resource.pData) + (desc.Height - 1 - y) * resource.RowPitch, rowPitch);
		}


		memcpy(resource.pData, flippedData, resource.RowPitch * desc.Height);

		delete[] flippedData;


		
		// Copy into normal texture
		glBindTexture(GL_TEXTURE_2D, 0);
		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, desc.Width, desc.Height, 0, GL_BGRA, GL_UNSIGNED_BYTE, resource.pData);
		glGenerateMipmap(GL_TEXTURE_2D);
		deviceContext->Unmap(pTexture, 0);

		// Release the desktop texture
		pTexture->Release();
		AcquiredDesktopImage->Release();
		deskDupl->ReleaseFrame();
		DesktopResource->Release();
		
		return (ImTextureID)m_texture;

	
	}
	void BaseLayer::OnAttach()
	{
		FramebufferSpecification spec;
		spec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::Depth };
		spec.Width = 1280;
		spec.Height = 720;
		m_Framebuffer = CreateRef<Framebuffer>(spec);

		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	void BaseLayer::OnDetach()
	{
		glDeleteTextures(1, &m_TextureID);
	}

	void BaseLayer::OnUpdate(float dt)
	{
		m_Fps = 1.0f / dt;

		if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
			m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
		{
			m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		}

		m_Framebuffer->Bind();

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		m_Framebuffer->Unbind();
	}

	void BaseLayer::OnImGuiRender()
	{
		// Viewport
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });
		ImGui::Begin("Viewport");

		ImVec2 main = ImGui::GetMainViewport()->Pos;
		ImVec2 pos = ImGui::GetWindowPos();

		m_ViewportOffset = glm::vec2{ pos.x, pos.y } - glm::vec2{ main.x, main.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused && !m_ViewportHovered);

		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportSize.x, viewportSize.y };

		uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
		ImGui::Image((void*)textureID, ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::Begin("Statistics");
		ImGui::Text("%.2f FPS", m_Fps);
		ImGui::End();

		ImGui::Begin("Desktop");
		ImGui::Image(getScreenUsingDuplicationApi(), ImVec2{ 1280, 720 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		ImGui::End();

	}

	void BaseLayer::OnEvent(Event& event)
	{
	}
}