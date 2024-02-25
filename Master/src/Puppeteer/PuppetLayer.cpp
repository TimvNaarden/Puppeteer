#include "pch.h"
#include "PuppetLayer.h"

#include "Core/Application.h"
#include "imgui_internal.h"

#include <imgui.h>
#include <glad/glad.h>
#include <Windows.h>
#include <lz4.h>
#include "Store/StoreJson.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace Puppeteer
{
	PuppetLayer::PuppetLayer(char* Ip, std::string Credentials) 
		: m_Ip(Ip), m_PCInfo(1), m_Fps(0), m_UpdatingTexture(true), m_Credentials(Credentials),
		lastWidth(1920), lastHeight(1080), m_Textures(), m_Texture(1), m_TextureID(2), m_Socket(nullptr), m_Initialized(0),
		m_Input(0), m_UserInput(0), m_Name("")
	{
		std::thread(&PuppetLayer::UpdateTexture, this).detach();
		glGenTextures(1, &m_Texture);
	}

	void PuppetLayer::UpdateTexture() {
		Socket s;
		if (s.Create(IPV4, TCP, CLIENT, 54000, this->m_Ip, true)) {
			this->m_Initialized = 1;
			return;
		}
		s.SendPacket(this->m_Credentials.data());
		char* response = s.ReceivePacket();
		if (response == "Con Closed" || response ==  "Not Authenticated") {
			this->m_Initialized = 1;
			this->m_UpdatingTexture = false;  
			return; 
		}
		s.SendPacket(pcInfoActionJson);
		char* responsePCInfo = s.ReceivePacket();
		if (responsePCInfo == "Con Closed") {
			this->m_Initialized = 1;
			this->m_UpdatingTexture = false;
			return;
		}
		int inPCSvec = 0;
		this->m_PCInfo = PCInfo(ParseJson<std::map<std::string, std::string>>(responsePCInfo));
		for (PCInfo pc : PcInfos) {
			if (pc.m_Systemname == this->m_PCInfo.m_Systemname) {
				this->m_PCInfo = pc;
				inPCSvec = 1;
				break;
			}
		}
		if (inPCSvec == 0) {
			PcInfos.push_back(this->m_PCInfo);
			std::vector<std::map<std::string, std::string>> pcs;
			for (PCInfo pc : PcInfos) {
				pcs.push_back(pc.toMap());
			}
			std::map<std::string, std::string> Save = { {"username", Username}, {"ip", Ip}, {"domain", Domain}, {"pcs", WriteJson(pcs)} };
			OverrideJsonTable("Puppeteer", WriteJson(Save));
		}
		this->m_Name = this->m_PCInfo.m_Systemname.data();
		this->m_Socket = &s;
		this->m_Initialized = 1;
		while (this->m_UpdatingTexture) {
			this->m_Mutex.lock();
			s.SendPacket(screenActionJson);
			std::string ResponseJson = s.ReceivePacket();
			if (ResponseJson == "Con Closed") { 
				this->m_UpdatingTexture = false;
				this->m_Socket->~Socket();
				this->m_Socket = nullptr;
				this->m_Mutex.unlock();
				break;
			}
			this->m_Mutex.unlock();
			
			std::map<std::string, int> ResponseMap = ParseJson<std::map<std::string, int>>(ResponseJson);
			ImageData Image{ResponseMap["width"], ResponseMap["height"] };
			this->m_Mutex.lock();
			char* Screen = s.ReceivePacket();
			if (Screen == "Con Closed") { 
				this->m_UpdatingTexture = false; 
				this->m_Socket->~Socket(); 
				this->m_Socket = nullptr; 
				this->m_Mutex.unlock();
				break;
			}
			this->m_Mutex.unlock();
			if (ResponseMap["size"] <= 1) continue; // When there are no changes, the size is 1 

			Image.Texture = new char[ResponseMap["size"]];
			int bDecompressed = LZ4_decompress_safe(Screen, Image.Texture, ResponseMap["csize"], ResponseMap["size"]);

			if(Screen) delete[] Screen;

			try {
				this->m_Mutex.lock();
				if(this->m_UpdatingTexture) this->m_Textures.push(Image);
				else delete[] Image.Texture;
				this->m_Mutex.unlock();
			}
			catch (std::exception e) {
				this->m_Mutex.unlock();
				delete[] Image.Texture;
			}
		}
		this->m_Mutex.lock();
		if (m_Socket) this->m_Socket->~Socket();
		this->m_Initialized = 0;
		this->m_Mutex.unlock();
	}


	void PuppetLayer::OnAttach()
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

	void PuppetLayer::OnDetach()
	{
		glDeleteTextures(1, &m_TextureID);
	}

	void PuppetLayer::OnUpdate(float dt)
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

	void PuppetLayer::OnImGuiRender()
	{
		if(!m_Initialized) return;
		if (!m_Socket) {
			m_Mutex.lock();
			if (m_Texture) glDeleteTextures(1, &m_Texture);
			m_Initialized = 0;
			app->RemoveLayer(this); 
			m_Mutex.unlock();
			return; 
		}
		// Viewport
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });
		ImGui::SetNextWindowDockID(2, ImGuiCond_FirstUseEver);
		ImGui::Begin(m_Name);

		ImVec2 main = ImGui::GetMainViewport()->Pos;
		ImVec2 pos = ImGui::GetWindowPos();

		m_ViewportOffset = glm::vec2{ pos.x, pos.y } - glm::vec2{ main.x, main.y };

		int focused = ImGui::IsWindowFocused();
		int hovered = ImGui::IsWindowHovered();
		Application::Get().GetImGuiLayer()->BlockEvents(!focused && !hovered);

		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportSize.x, viewportSize.y };

		uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
		
		if (!m_Textures.empty()) {
			ImageData Image = m_Textures.front();
			lastWidth = Image.Width;
			lastHeight = Image.Height;
			//if(m_Texture != 0) glDeleteTextures(1, &m_Texture);
			//glBindTexture(GL_TEXTURE_2D, 0);
			
			glBindTexture(GL_TEXTURE_2D, m_Texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Image.Width, Image.Height, 0, GL_BGRA, GL_UNSIGNED_BYTE, Image.Texture);
			glGenerateMipmap(GL_TEXTURE_2D);
			ImGui::Image((ImTextureID)m_Texture, ImVec2{ (float)Image.Width, (float)Image.Height }, ImVec2{ 0, -1 }, ImVec2{ 1, 0 });
			m_Textures.pop();
			delete[] Image.Texture;
		} else if(m_Texture != 0) {
			ImGui::Image((ImTextureID)m_Texture, ImVec2{ (float)lastWidth, (float)lastHeight }, ImVec2{ 0, -1 }, ImVec2{ 1, 0 });
			
		}
		ImGui::End();
		if (!focused && !m_ViewportFocused &&!m_ViewportHovered) {
			ImGui::PopStyleVar();
			return;
		}
		std::stringstream Controls;
		Controls << "Controls " << m_Name;
		ImGui::SetNextWindowDockID(1, ImGuiCond_FirstUseEver);
		ImGui::Begin(Controls.str().data());

		char* toggleUserInput = (m_UserInput) ? "Enable User Input" : "Disable User Input";
		if (ImGui::Button(toggleUserInput) || (ImGui::IsItemFocused() &&
			((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeyPadEnter))
				|| ImGui::IsKeyPressed(ImGuiKey_Tab)))) {
			if (ImGui::IsKeyPressed(ImGuiKey_Tab)) ImGui::SetKeyboardFocusHere(0);
			else {
				m_UserInput = !m_UserInput;
				m_Mutex.lock();
				if (m_Socket) m_Socket->SendPacket(lockInputActionJson);
				m_Mutex.unlock();
			}
		}
		char* toggleInput = (!m_Input) ? "Enable Input" : "Disable Input";
		if (ImGui::Button(toggleInput) || (ImGui::IsItemFocused() && 
			((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeyPadEnter)) 
			|| ImGui::IsKeyPressed(ImGuiKey_Tab)))) {
			if (ImGui::IsKeyPressed(ImGuiKey_Tab)) ImGui::SetKeyboardFocusHere(0);
			else m_Input = !m_Input;
		}
		if (ImGui::Button("Close") || (ImGui::IsItemFocused() &&
			((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeyPadEnter))
				|| ImGui::IsKeyPressed(ImGuiKey_Tab)))) {
			if (ImGui::IsKeyPressed(ImGuiKey_Tab)) ImGui::SetKeyboardFocusHere(0);
			else {
				m_Mutex.lock();
				if (m_Socket) m_Socket->SendPacket(closeActionJson);
				m_Mutex.unlock();
				m_UpdatingTexture = false;
				app->RemoveLayer(this);
			}
		}
		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void PuppetLayer::OnEvent(Event& event)
	{
		if (!m_Input || !m_Initialized) return;
		if (event.GetName() == "KeyPressed") {
			KeyPressedEvent e = (KeyPressedEvent&)event;
			//Convert keycode to a code SendInput can understand

			std::vector<std::map<std::string, int>> keycodes = { {{"Code", Key::WindowsCodes[e.GetKeyCode()]}, {"Flags", 0x0000}}, };
			std::map<std::string, std::string> sendKeyStrokes = { {"ActionType", "Keystrokes"} , {"Action", WriteJson(keycodes)} };
			m_Mutex.lock();
			if (m_Socket) m_Socket->SendPacket(WriteJson(sendKeyStrokes).data());
			m_Mutex.unlock();
		}
		else if (event.GetName() == "KeyReleased") {
			KeyReleasedEvent e = (KeyReleasedEvent&)event;
			std::vector<std::map<std::string, int>> keycodes = { {{"Code", Key::WindowsCodes[e.GetKeyCode()]}, {"Flags", 0x002}}, };
			std::map<std::string, std::string> sendKeyStrokes = { {"ActionType", "Keystrokes"} , {"Action", WriteJson(keycodes)} };
			m_Mutex.lock();
			if (m_Socket) m_Socket->SendPacket(WriteJson(sendKeyStrokes).data());
			m_Mutex.unlock();
		}
		else if (event.GetName() == "MouseMoved") {
			MouseMovedEvent e = (MouseMovedEvent&)event;
			std::vector<std::map<std::string, int>> mouse = { { {"dx", (e.GetX() - m_ViewportOffset.x)* (65536 / lastWidth) }, {"dy", (e.GetY() - m_ViewportOffset.y * 2.2) * (65536 / lastHeight) }, {"mouseData",0}, {"dwFlags", 0x0001 | 0x8000}} };
			std::map<std::string, std::string> sendMouse = { {"ActionType", "Mouse"} , {"Action", WriteJson(mouse)} };
			m_Mutex.lock();
			if(m_Socket) m_Socket->SendPacket(WriteJson(sendMouse).data());
			m_Mutex.unlock();
		}
		else if (event.GetName() == "MouseButtonPressed") {
			MouseButtonPressedEvent e = (MouseButtonPressedEvent&)event;
			std::vector < std::map<std::string, int>> mouse = { { {"dx", 0}, {"dy", 0}, {"mouseData",0}, {"dwFlags", Mouse::WindowsMouseDown[e.GetMouseButton()]}} };
			std::map<std::string, std::string> sendMouse = { {"ActionType", "Mouse"} , {"Action", WriteJson(mouse)} };
			m_Mutex.lock();
			if (m_Socket) m_Socket->SendPacket(WriteJson(sendMouse).data());
			m_Mutex.unlock();
		}
		else if (event.GetName() == "MouseButtonReleased") {
			MouseButtonReleasedEvent e = (MouseButtonReleasedEvent&)event;
			std::vector < std::map<std::string, int>> mouse = { { {"dx", 0}, {"dy", 0}, {"mouseData",0}, {"dwFlags", Mouse::WindowsMouseUp[e.GetMouseButton()]} } };
			std::map<std::string, std::string> sendMouse = { {"ActionType", "Mouse"} , {"Action", WriteJson(mouse)} };
			m_Mutex.lock();
			if (m_Socket) m_Socket->SendPacket(WriteJson(sendMouse).data());
			m_Mutex.unlock();
		}
		else if (event.GetName() == "MouseScrolled") {
			MouseScrolledEvent e = (MouseScrolledEvent&)event;
			int offset = (e.GetYOffset() != 0) ? e.GetYOffset() : e.GetXOffset();
			std::vector < std::map<std::string, int>> mouse = { { {"dx", 0}, {"dy", 0}, {"mouseData", offset * 100}, {"dwFlags", 0x0800} } };
			std::map<std::string, std::string> sendMouse = { {"ActionType", "Mouse"} , {"Action", WriteJson(mouse)} };
			m_Mutex.lock();
			if (m_Socket) m_Socket->SendPacket(WriteJson(sendMouse).data());
			m_Mutex.unlock();
		}
	}
}