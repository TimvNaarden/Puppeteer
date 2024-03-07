#include "pch.h"
#include "PuppetLayer.h"

namespace Puppeteer
{
	PuppetLayer::PuppetLayer(char* Ip, Credentials_T Creds) 
		: m_Ip(Ip), m_PCInfo(false), m_Fps(0), m_UpdatingTexture(true), m_Credentials(Creds),
		lastWidth(1920), lastHeight(1080), m_Textures(), m_Texture(1), m_TextureID(2), m_Socket(nullptr), m_Initialized(0),
		m_Input(0), m_UserInput(0), m_Name("")
	{
		std::thread(&PuppetLayer::UpdateTexture, this).detach();
		glGenTextures(1, &m_Texture);
	}

	void PuppetLayer::UpdateTexture() {
		Networking::TCPClient s(Networking::IPV4, 54000, m_Ip, 1);
		char* Sends = (char*)&m_Credentials;
		s.Send(Sends, sizeof(m_Credentials));
		char* response;
		int Result = s.Receive(response);
		if (Result == -1 || response == "Not Authenticated") {
			this->m_Initialized = 1;
			this->m_UpdatingTexture = false;
			return;
		}
		Action.Type = ActionType::ReqPCInfo;
		Sends = (char*)&Action;
		Result = s.Send(Sends, sizeof(Action));
		if (Result == -1) {
			this->m_Initialized = 1;
			this->m_UpdatingTexture = false;
			return;
		}
		delete[] response;
		Result = s.Receive(response);
		if (Result == -1) {
			this->m_Initialized = 1;
			this->m_UpdatingTexture = false;
			return;
		}
		int inPCSvec = 0;
		this->m_PCInfo = PCInfo(ParseJson<std::map<std::string, std::string>>(response));
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
			std::map<std::string, std::string> Save = { 
				{"username", m_Credentials.Username},
				{"ip", m_Ip}, 
				{"domain", m_Credentials.Domain},
				{"pcs", WriteJson(pcs)} 
			};
			OverrideJsonTable("Puppeteer", WriteJson(Save));
		}
		this->m_Name = this->m_PCInfo.m_Systemname.data();
		this->m_Socket = &s;
		this->m_Initialized = 1;
		delete[] response;
		while (this->m_UpdatingTexture) {
			this->m_Mutex.lock();
			Action.Type = ActionType::Screen;
			char* Sends = (char*)&Action;
			Result = s.Send(Sends, sizeof(Action));
			if (Result == -1) {
				this->m_UpdatingTexture = false;
				this->m_Socket = nullptr;
				this->m_Mutex.unlock();
				break;
			}
			
			Result = s.Receive(response);
			if (Result == -1) { 
				this->m_UpdatingTexture = false;
				this->m_Socket = nullptr;
				this->m_Mutex.unlock();
				break;
			}
			this->m_Mutex.unlock();
			
			std::map<std::string, int> ResponseMap = ParseJson<std::map<std::string, int>>(response);
			ImageData Image{ResponseMap["width"], ResponseMap["height"] };
			this->m_Mutex.lock();
			delete[] response;
			Result = s.Receive(response);
			if (Result == -1) { 
				this->m_UpdatingTexture = false; 
				this->m_Socket = nullptr; 
				this->m_Mutex.unlock();
				break;
			}
			this->m_Mutex.unlock();
			if (ResponseMap["size"] <= 1) continue; // When there are no changes, the size is 1 

			Image.Texture = new char[ResponseMap["size"]];
			int bDecompressed = LZ4_decompress_safe(response, Image.Texture, ResponseMap["csize"], ResponseMap["size"]);

			delete[] response;

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
			delete[] Image.Texture;
			m_Textures.pop();
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
				Action.Type = ActionType::LockInput;
				char* Sends = (char*)&Action;
				if (m_Socket) m_Socket->Send(Sends, sizeof(Action));
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
				Action.Type = ActionType::Close;
				char* Sends = (char*)&Action;
				if (m_Socket) m_Socket->Send(Sends, sizeof(Action));
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
	bool PuppetLayer::KeyPressend(KeyPressedEvent& e) {
		m_Mutex.lock();
		Action.Type = ActionType::Keystrokes;
		Action.Inputdata = Key::WindowsCodes[e.GetKeyCode()];
		Action.Flags = 0x0000;
		if (m_Socket) {
			char* Sends = (char*)&Action;
			m_Socket->Send(Sends, sizeof(Action));
		}
		m_Mutex.unlock();

		return false;
	}

	bool PuppetLayer::KeyReleased(KeyReleasedEvent& e) {
		m_Mutex.lock();
		Action.Type = ActionType::Keystrokes;
		Action.Inputdata = Key::WindowsCodes[e.GetKeyCode()];
		Action.Flags = 0x0002;
		if (m_Socket) {
			char* Sends = (char*)&Action;
			m_Socket->Send(Sends, sizeof(Action));
		}
		m_Mutex.unlock();

		return false;
	}

	bool PuppetLayer::MouseMoves(MouseMovedEvent& e) {
		m_Mutex.lock();
		Action.Type = ActionType::Mouse;
		Action.dx = (e.GetX() - m_ViewportOffset.x) * (65536 / lastWidth);
		Action.dy = (e.GetY() - m_ViewportOffset.y * 2.2) * (65536 / lastHeight);
		Action.Inputdata = 0;
		Action.Flags = 0x0001 | 0x8000;
		if (m_Socket) {
			char* Sends = (char*)&Action;
			m_Socket->Send(Sends, sizeof(Action));
		}
		m_Mutex.unlock();

		return false;
	}	

	bool PuppetLayer::MouseScrolled(MouseScrolledEvent& e) {
		m_Mutex.lock();
		Action.Type = ActionType::Mouse;
		Action.dx = 0;
		Action.dy = 0;
		Action.Inputdata = e.GetYOffset() * 100;
		Action.Flags = 0x0800;
		if (m_Socket) {
			char* Sends = (char*)&Action;
			m_Socket->Send(Sends, sizeof(Action));
		}
		m_Mutex.unlock();

		return false;
	}

	bool PuppetLayer::MouseButtonPressed(MouseButtonPressedEvent& e) {
		m_Mutex.lock();
		Action.Type = ActionType::Mouse;
		Action.dx = 0;
		Action.dy = 0;
		Action.Inputdata = 0;
		Action.Flags = Mouse::WindowsMouseDown[e.GetMouseButton()];
		if (m_Socket) {
			char* Sends = (char*)&Action;
			m_Socket->Send(Sends, sizeof(Action));
		}
		m_Mutex.unlock();

		return false;
	}

	bool PuppetLayer::MouseButtonReleased(MouseButtonReleasedEvent& e) {
		m_Mutex.lock();
		Action.Type = ActionType::Mouse;
		Action.dx = 0;
		Action.dy = 0;
		Action.Inputdata = 0;
		Action.Flags = Mouse::WindowsMouseUp[e.GetMouseButton()];
		if (m_Socket) {
			char* Sends = (char*)&Action;
			m_Socket->Send(Sends, sizeof(Action));
		}
		m_Mutex.unlock();

		return false;
	}

	void PuppetLayer::OnEvent(Event& event) {
		if(!m_Input || !m_Initialized) return;
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(PuppetLayer::KeyPressend));
		dispatcher.Dispatch<KeyReleasedEvent>(BIND_EVENT_FN(PuppetLayer::KeyReleased));
		dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN(PuppetLayer::MouseMoves));
		dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(PuppetLayer::MouseScrolled));
		dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(PuppetLayer::MouseButtonPressed));
		dispatcher.Dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(PuppetLayer::MouseButtonReleased));
	}
}
