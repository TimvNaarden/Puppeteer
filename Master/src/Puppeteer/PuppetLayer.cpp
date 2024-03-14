#include "pch.h"
#include "PuppetLayer.h"

#define RECEIVE(response) {												\
	if(response) delete[] response;										\
	if (s.Receive(response) == -1) {									\
		if(response) delete[] response;									\
		Error = "Client Disconnected";									\
		modalOpen = true;												\
		return;															\
	}																	\
}
#define RECEIVES(response) {											\
	if(response) delete[] response;										\
	if (this->m_Socket->Receive(response) == -1) {						\
		Error = "Client Disconnected";									\
		modalOpen = true;												\
		this->m_Socket = nullptr;										\
		this->m_Mutex.unlock();											\
		return;															\
	}																	\
}
#define SEND(action, sockets)											\
	if(sockets.Send((char*)&action, sizeof(action)) == -1) {			\
		Error = "Client Disconnected";									\
		modalOpen = true;												\
		return;															\
}
#define SENDS(action)													\
	if(this->m_Socket->Send((char*)&action, sizeof(action)) == -1) {	\
		Error = "Client Disconnected";									\
		modalOpen = true;												\
		this->m_Socket = nullptr;										\
		this->m_Mutex.unlock();											\
		return;															\
}


namespace Puppeteer
{
	PuppetLayer::PuppetLayer(char* Ip, Credentials_T Creds) : m_PCInfo(false) {
		m_Ip = Ip;
		m_Credentials = Creds;

		m_Socket = nullptr;
		m_UpdatingTexture = true;

		m_Name = "";

		m_Fps = 0;
		m_Input = 0;
		m_UserInput = 0;
		m_ImageRatio = 1;
		m_Initialized = 0;

		m_ImageSize = ImVec2(0, 0);

		m_Texture = 0;
		m_TextureID = 0;
		m_Textures = std::queue<ImageData>();

		m_LayerNumber = LayerCount;
		LayerCount++;

		std::thread(&PuppetLayer::UpdateTexture, this).detach();
		glGenTextures(1, &m_Texture);
	}

	void PuppetLayer::UpdateTexture() {
		Networking::TCPClient s(Networking::IPV4, 54000, m_Ip, 1);
		char* response = 0;

		SEND(m_Credentials, s);
		RECEIVE(response);
		if (strcmp(response, "Not Authenticated") == 0) {
			Error = "Wrong Login Credentials";
			modalOpen = true;
			return;
		}

		Action.Type = ActionType::ReqPCInfo;
		SEND(Action, s);
		RECEIVE(response);

		int inPCSvec = 0;
		this->m_PCInfo = PCInfo(ParseJson<std::map<std::string, std::string>>(response));
		for (PCInfo pc : PcInfos) {
			if (pc.m_Systemname == this->m_PCInfo.m_Systemname) {
				inPCSvec = 1;
				break;
			}
		}
		if(!inPCSvec) PcInfos.push_back(this->m_PCInfo);
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
		

		this->m_Name = this->m_PCInfo.m_Systemname.data();
		this->m_Socket = &s;
		this->m_Initialized = 1;

		while (this->m_UpdatingTexture) {
			this->m_Mutex.lock();
			Action.Type = ActionType::Screen;
			if(m_UpdatingTexture) SENDS(Action);

			if (m_UpdatingTexture) RECEIVES(response);


			std::map<std::string, int> ResponseMap = ParseJson<std::map<std::string, int>>(response);
			ImageData Image{ ResponseMap["width"], ResponseMap["height"] };


			if (m_UpdatingTexture) RECEIVES(response);
			this->m_Mutex.unlock();

			if (ResponseMap["size"] <= 1) continue; // When there are no changes, the size is 1 

			Image.Texture = new char[ResponseMap["size"]];
			int bDecompressed = LZ4_decompress_safe(response, Image.Texture, ResponseMap["csize"], ResponseMap["size"]);

			try {
				this->m_Mutex.lock();
				if (this->m_UpdatingTexture) this->m_Textures.push(Image);
				else delete[] Image.Texture;
				this->m_Mutex.unlock();
			}
			catch (std::exception e) {
				delete[] Image.Texture;
			}
		}
	}

	void PuppetLayer::UpdateInfo() {
		m_Mutex.lock();
		Action.Type = ActionType::ReqPCInfo;
		SENDS(Action);
		char* response = 0;
		RECEIVES(response);
		std::vector<std::map<std::string, std::string>> pcs;
		for (PCInfo info : PcInfos) {
			if (info.m_Systemname == m_PCInfo.m_Systemname) {
				info = PCInfo(ParseJson<std::map<std::string, std::string>>(response));
				pcs.push_back(info.toMap());
				break;
			}
			pcs.push_back(info.toMap());
		}
		std::map<std::string, std::string> Save = {
			{"username", m_Credentials.Username},
			{"ip", m_Ip},
			{"domain", m_Credentials.Domain},
			{"pcs", WriteJson(pcs)}
		};
		OverrideJsonTable("Puppeteer", WriteJson(Save));
		m_Mutex.unlock();

	}
	void PuppetLayer::CalculateImageSize(ImVec2 screenSize, ImVec2 imageSize) {
		if (screenSize.x >= imageSize.x && screenSize.y >= imageSize.y) return;
		else if (screenSize.x < imageSize.x && screenSize.y >= imageSize.y) {
			m_ImageRatio = screenSize.x / imageSize.x;
			m_ImageSize = ImVec2(screenSize.x, imageSize.y * m_ImageRatio);
		}else if (screenSize.x >= imageSize.x && screenSize.y < imageSize.y) {
			m_ImageRatio = screenSize.y / imageSize.y;
			m_ImageSize = ImVec2(imageSize.x * m_ImageRatio, screenSize.y);
		}else {
			float xRatio = screenSize.x / imageSize.x;
			float yRatio = screenSize.y / imageSize.y;
			m_ImageRatio = (xRatio < yRatio) ? xRatio : yRatio;
			m_ImageSize = ImVec2(imageSize.x * m_ImageRatio, imageSize.y * m_ImageRatio);
		}
		return;

	}

	void PuppetLayer::OnAttach() {
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

	void PuppetLayer::OnDetach() {
		glDeleteTextures(1, &m_TextureID);
	}

	void PuppetLayer::OnUpdate(float dt) {
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

	void PuppetLayer::OnImGuiRender() {
		if (!m_Initialized) return;
		if (!m_Socket) { app->RemoveLayer(this); return; }


		// Viewport
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });
		ImGui::SetNextWindowDockID(2, ImGuiCond_FirstUseEver);
		ImGui::Begin(m_Name);

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();

		if (m_ViewportFocused) ActiveLayerIndex = m_LayerNumber;

		ImVec2 main = ImGui::GetMainViewport()->Pos;
		ImVec2 pos = ImGui::GetWindowPos();

		m_ViewportOffset = glm::vec2{ pos.x, pos.y } - glm::vec2{ main.x, main.y };

		Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused && !m_ViewportHovered);

		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportSize.x, viewportSize.y };

		uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
		

		if (!m_Textures.empty()) {
			ImageData Image = m_Textures.front();
			m_ImageSize = { static_cast<float>(Image.Width), static_cast<float>(Image.Height) };
			ImVec2 ScreenSizeMin = ImGui::GetWindowContentRegionMin();
			ImVec2 ScreenSizeMax = ImGui::GetWindowContentRegionMax();
			CalculateImageSize({ScreenSizeMax.x - ScreenSizeMin.x, ScreenSizeMax.y - ScreenSizeMin.y}, m_ImageSize);

			glBindTexture(GL_TEXTURE_2D, m_Texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Image.Width, Image.Height, 0, GL_BGRA, GL_UNSIGNED_BYTE, Image.Texture);
			glGenerateMipmap(GL_TEXTURE_2D);
			ImGui::Image((ImTextureID)m_Texture, m_ImageSize, ImVec2{ 0, -1 }, ImVec2{ 1, 0 });
			if (ImGui::IsItemHovered() && m_Input) {
				ImVec2 mousePositionAbsolute = ImGui::GetMousePos();
				ImVec2 screenPositionAbsolute = ImGui::GetItemRectMin();
				ImVec2 mousePositionRelative = ImVec2(mousePositionAbsolute.x - screenPositionAbsolute.x, mousePositionAbsolute.y - screenPositionAbsolute.y);
				if (m_LastMousePos.x != mousePositionRelative.x || m_LastMousePos.y != mousePositionRelative.y)
					MouseMoves((int)mousePositionRelative.x, (int)mousePositionRelative.y);

				for (int key = 512; key < ImGuiKey_COUNT; key++) {
					if (ImGui::IsKeyPressed((ImGuiKey)key)) {
						if (key < 641) KeyPressed(ImGuiToWin[key]);
						else if (key < 645) MouseButton(MouseDown[key]);
						else if (key < 648) MouseScrolled(ImGui::GetIO().MouseWheel < 0 ? -1 : 1);
					}
					if (ImGui::IsKeyReleased((ImGuiKey)key)) {
						if (key < 641) KeyReleased(ImGuiToWin[key]);
						else if (key < 645) MouseButton(MouseUp[key]);
					}
				}
				m_LastMousePos = mousePositionRelative;
			}
			delete[] Image.Texture;
			m_Textures.pop();
		}
		else if (m_Texture != 0) {
			ImGui::Image((ImTextureID)m_Texture, m_ImageSize, ImVec2{ 0, -1 }, ImVec2{ 1, 0 });
			if (ImGui::IsItemHovered() && m_Input) {
				ImVec2 mousePositionAbsolute = ImGui::GetMousePos();
				ImVec2 screenPositionAbsolute = ImGui::GetItemRectMin();
				ImVec2 mousePositionRelative = ImVec2(mousePositionAbsolute.x - screenPositionAbsolute.x, mousePositionAbsolute.y - screenPositionAbsolute.y);
				if(m_LastMousePos.x != mousePositionRelative.x || m_LastMousePos.y != mousePositionRelative.y) 
					MouseMoves((int)mousePositionRelative.x, (int)mousePositionRelative.y);

				for (int key = 512; key < ImGuiKey_COUNT; key++) {
					if (ImGui::IsKeyPressed((ImGuiKey)key)) {
						if (key < 641) KeyPressed(ImGuiToWin[key]);
						else if (key < 645) MouseButton(MouseDown[key]);
						else if (key < 648) MouseScrolled(ImGui::GetIO().MouseWheel < 0 ? -1 : 1);
					}
					if (ImGui::IsKeyReleased((ImGuiKey)key)) {
						if (key < 641) KeyReleased(ImGuiToWin[key]);
						else if (key < 645) MouseButton(MouseUp[key]);
					}
				}
				m_LastMousePos = mousePositionRelative;
			}
		}

		ImGui::End();

		if (ActiveLayerIndex != m_LayerNumber) { ImGui::PopStyleVar(); return; }

		std::stringstream Controls;
		Controls << "Controls " << m_Name;

		ImGui::SetNextWindowDockID(1, ImGuiCond_FirstUseEver);
		ImGui::Begin(Controls.str().data());

		char* toggleUserInput = (m_UserInput) ? "Enable User Input" : "Disable User Input";
		if (ImGui::Button(toggleUserInput) || 
			(ImGui::IsItemFocused() &&
				(  ImGui::IsKeyPressed(ImGuiKey_Enter) 
				|| ImGui::IsKeyPressed(ImGuiKey_KeyPadEnter)
				|| ImGui::IsKeyPressed(ImGuiKey_Tab))
			)) {
			if (ImGui::IsKeyPressed(ImGuiKey_Tab)) ImGui::SetKeyboardFocusHere(0);
			else {
				m_UserInput = !m_UserInput;
				m_Mutex.lock();
				Action.Type = ActionType::LockInput;
				SENDS(Action);
				m_Mutex.unlock();
			}
		}
		char* toggleInput = (!m_Input) ? "Enable Input" : "Disable Input";
		if (ImGui::Button(toggleInput) ||
			(ImGui::IsItemFocused() &&
				(ImGui::IsKeyPressed(ImGuiKey_Enter)
					|| ImGui::IsKeyPressed(ImGuiKey_KeyPadEnter)
					|| ImGui::IsKeyPressed(ImGuiKey_Tab))
				)) {
			if (ImGui::IsKeyPressed(ImGuiKey_Tab)) ImGui::SetKeyboardFocusHere(0);
			else m_Input = !m_Input;
		}
		if (ImGui::Button("Close") ||
			(ImGui::IsItemFocused() &&
				(ImGui::IsKeyPressed(ImGuiKey_Enter)
					|| ImGui::IsKeyPressed(ImGuiKey_KeyPadEnter)
					|| ImGui::IsKeyPressed(ImGuiKey_Tab))
				)) {
			if (ImGui::IsKeyPressed(ImGuiKey_Tab)) ImGui::SetKeyboardFocusHere(0);
			else {
				m_Mutex.lock();
				Action.Type = ActionType::Close;
				SENDS(Action);
				m_UpdatingTexture = false;
				app->RemoveLayer(this);
				m_Mutex.unlock();
			}
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}
	void PuppetLayer::KeyPressed(int key) {
		m_Mutex.lock();
		Action.Type = ActionType::Keystrokes;
		Action.Inputdata = key;
		Action.Flags = 0x0000;
		if (m_Socket) {
			SENDS(Action);
		}
		m_Mutex.unlock();
	}

	void PuppetLayer::KeyReleased(int key) {
		m_Mutex.lock();
		Action.Type = ActionType::Keystrokes;
		Action.Inputdata = key;
		Action.Flags = 0x0002;
		if (m_Socket) {
			SENDS(Action);
		}
		m_Mutex.unlock();
	}

	void PuppetLayer::MouseButton(int Flags) {
		m_Mutex.lock();
		Action.Type = ActionType::Mouse;
		Action.Inputdata = 0;
		Action.Flags = Flags;
		if (m_Socket) {
			SENDS(Action);
		}
		m_Mutex.unlock();
	}

	void PuppetLayer::MouseMoves(int x, int y) {
		m_Mutex.lock();
		Action.Type = ActionType::Mouse;
		Action.dx = x * (65536 / m_ImageSize.x);
		Action.dy = y * (65536 / m_ImageSize.y);
		Action.Inputdata = 0;
		Action.Flags = 0x0001 | 0x8000;
		if (m_Socket) {
			SENDS(Action);
		}
		m_Mutex.unlock();
	}

	void PuppetLayer::MouseScrolled(int offset) {
		m_Mutex.lock();
		Action.Type = ActionType::Mouse;
		Action.dx = 0;
		Action.dy = 0;
		Action.Inputdata = offset * 100;
		Action.Flags = 0x0800;
		if (m_Socket) {
			SENDS(Action);
		}
		m_Mutex.unlock();
	}

	void PuppetLayer::OnEvent(Event& event) {

	}

}
