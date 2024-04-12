#include "pch.h"
#include "ConfigLayer.h"
#include "imgui_internal.h"
#include "Store/StoreJson.h"
#include "misc/cpp/imgui_stdlib.cpp"
#include <nfd.h>

namespace Puppeteer {
	ConfigLayer::ConfigLayer()	{
		m_Fps = 0;
		m_Texture = 0;
		m_TextureID = 0;

		m_LayerNumber = LayerCount;
		LayerCount++;

		if (CheckJsonTable("Puppeteer")) {
			CreateJsonTable("Puppeteer");
		}
		else {
			std::vector<std::map<std::string, std::string >> CredentialsVector = ExtractJsonTable<std::map<std::string, std::string>>("Puppeteer");
			if (CredentialsVector.size() == 0) return;
			
			std::map<std::string, std::string> ParsedCredentials = CredentialsVector[0];
			strcpy(Credentials.Username, ParsedCredentials["username"].c_str());
			strcpy(Credentials.Password, ParsedCredentials["password"].c_str());
			strcpy(Credentials.Domain, ParsedCredentials["domain"].c_str());

			std::vector<std::string> GridClientsS = ParseJson<std::vector<std::string>>(ParsedCredentials["GridClients"].data());
			for (std::string Client : GridClientsS) { GridClients.push_back(GridClient_T(Client)); }
			Ip = ParsedCredentials["ip"];
			std::vector<std::map<std::string, std::string>> pcs = ParseJson<std::vector<std::map<std::string, std::string>>>(ParsedCredentials["pcs"].data());
			for (std::map<std::string, std::string> pc: pcs) { PcInfos.push_back(PCInfo(pc)); }
		}
	}
	void ConfigLayer::OnAttach() {
		m_Texture = 0;
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

	void ConfigLayer::OnDetach() {
		glDeleteTextures(1, &m_TextureID);
	}

	void ConfigLayer::OnUpdate(float dt) {
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
	static int IpCompletionCallBack(ImGuiInputTextCallbackData* data) {
		if (data->EventKey == ImGuiKey_Tab) ImGui::SetKeyboardFocusHere(0);
		return 0;
	}

	void ConfigLayer::OnImGuiRender() {
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(400, 100), ImGuiCond_Appearing);
		if (modalOpen) {
			ImGui::OpenPopup("Error");
			modalOpen = false;
		}
		if (ImGui::BeginPopupModal("Error")) {
			ImGui::Text(Error.data());
			if (ImGui::Button("OK")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		// Viewport
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });

		ImGui::SetNextWindowDockID(2, ImGuiCond_FirstUseEver);
		ImGui::Begin("Settings");

		if (m_ViewportFocused) ActiveLayerIndex = m_LayerNumber;

		ImVec2 main = ImGui::GetMainViewport()->Pos;
		ImVec2 pos = ImGui::GetWindowPos();

		m_ViewportOffset = glm::vec2{ pos.x, pos.y } - glm::vec2{ main.x, main.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused && !m_ViewportHovered);

		if (m_First) {
			ImGui::SetKeyboardFocusHere(0);
			m_First = false;
		}
		ImGui::Columns(2);
		//Left side
		ImGui::PushItemWidth(150);
		ImGui::LabelText("##username_label", "Username: ");
		if (ImGui::InputText("##username_input", Credentials.Username, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
			SAVE();
			ImGui::SetKeyboardFocusHere(0);
		}
		ImGui::LabelText("##password_label", "Password: ");
		if (ImGui::InputText("##password_input", Credentials.Password, 256, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue)) {
			SAVE();
			ImGui::SetKeyboardFocusHere(0);
		}
		ImGui::LabelText("##domain_label", "Domain: ");
		if (ImGui::InputText("##domain_input", Credentials.Domain, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
			SAVE();
			ImGui::SetKeyboardFocusHere(0);
		}
		ImGui::LabelText("##ip_label", "Ip: ");
		if (ImGui::InputText("##ip_input", &Ip, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion, IpCompletionCallBack)) {
			SAVE();
			ImGui::SetKeyboardFocusHere(0);
		}

		if (ImGui::Button("Connect") || (ImGui::IsItemFocused() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeyPadEnter)))) {
			SAVE();
			app->PushLayer(new PuppetLayer(Ip.data(), Credentials));
		}
		ImGui::NextColumn();// End left side
		// Right side
		ImGui::Text("PC Configurations:\n");
		std::queue<int> toRemove;
		int index = 0;
		for (auto& GridClient : GridClients) {
			
			ImGui::PopItemWidth();
			ImGui::PushItemWidth(60);
			ImGui::LabelText(std::string("##pcname_label" + std::to_string(index)).data(), "Name: ");
			ImGui::PopItemWidth();
			ImGui::PushItemWidth(150);
			ImGui::SameLine();
			if (ImGui::InputText(std::string("##pcname_label" + std::to_string(index)).data(), GridClient.Name, 256)) {
				SAVE();
			}
			ImGui::SameLine();
			ImGui::PopItemWidth();
			ImGui::PushItemWidth(30);
			ImGui::LabelText(std::string("##pcip_label" + std::to_string(index)).data(), "Ip: ");
			ImGui::PopItemWidth();
			ImGui::PushItemWidth(150);
			ImGui::SameLine();
			if (ImGui::InputText(std::string("##pcip_label" + std::to_string(index)).data(), GridClient.Ip, 256)) {
				SAVE();
			}
			ImGui::SameLine();
			if (ImGui::Button(std::string("Remove##" + std::to_string(index)).data())) {
				toRemove.push(index);
			}
			index++;
		}

		while (!toRemove.empty()) {
			GridClients.erase(GridClients.begin() + toRemove.front());
			toRemove.pop();
			if (toRemove.empty()) {
				SAVE();
			}
		}
		

		// Button to add a new element
		if (ImGui::Button("Add PC")) {
			GridClient_T NewClient;
			GridClients.push_back(NewClient);
		} 
		ImGui::SameLine();
		if (ImGui::Button("Import PC list")) {
			nfdchar_t* outPath;
			nfdresult_t result = NFD_OpenDialog(&outPath, NULL, NULL, NULL);
			if (result == NFD_OKAY) {
				if (std::string(outPath).find(".json") == std::string::npos) {
					Error = "Invalid file format";
					modalOpen = true;
					ImGui::Columns(1);
					if (m_ViewportFocused) ActiveLayerIndex = m_LayerNumber;

					ImGui::PopItemWidth();
					ImGui::End();
					ImGui::PopStyleVar();
					return;
				} 
				std::ifstream PCConfig{ outPath };
				if (!PCConfig) {
					Error = "Cannot open file";
					modalOpen = true;
					ImGui::Columns(1);
					if (m_ViewportFocused) ActiveLayerIndex = m_LayerNumber;

					ImGui::PopItemWidth();
					ImGui::End();
					ImGui::PopStyleVar();
					return;
				} 
				std::string line;
				std::getline(PCConfig, line);

				if (!line._Starts_with("[\"") ) {
					Error = "Wrong file contents";
					modalOpen = true;
					ImGui::Columns(1);
					if (m_ViewportFocused) ActiveLayerIndex = m_LayerNumber;

					ImGui::PopItemWidth();
					ImGui::End();
					ImGui::PopStyleVar();
					return;
				}
				std::vector<std::string> temp = ParseJson<std::vector<std::string>>(line);
				PCConfig.close();
				GridClients.clear();
				for (std::string Client : temp) { GridClients.push_back(GridClient_T(Client)); }


				Error = "PC List Imported from " + std::string(outPath);
				NFD_FreePath(outPath);
				modalOpen = true;
			} else if (result == NFD_ERROR) {
				Error = "Error: " + std::string(NFD_GetError());
				modalOpen = true;
			}
			SAVE();
		}
		ImGui::SameLine();
		if (ImGui::Button("Export PC list")) {
			nfdchar_t* outPath = NULL;
			nfdresult_t result = NFD_PickFolder(&outPath, NULL);

			if (result == NFD_OKAY) {
				std::ofstream PCConfig{ (outPath + std::string("/PCListExport.json")) };
				if (!PCConfig) {
					Error = "Cannot open file";
					modalOpen = true;
					ImGui::Columns(1);
					if (m_ViewportFocused) ActiveLayerIndex = m_LayerNumber;

					ImGui::PopItemWidth();
					ImGui::End();
					ImGui::PopStyleVar();
					return;
				}
				std::vector<std::string> GClients = {};
				for (GridClient_T Client : GridClients) {
					GClients.push_back(Client.toString());
				}
				PCConfig << WriteJson(GClients);
				PCConfig.close();

				Error = "PC List Imported to " + std::string(outPath);
				NFD_FreePath(outPath);
				modalOpen = true;
			} else if (result == NFD_ERROR) {
				Error = "Error: " + std::string(NFD_GetError());
				modalOpen = true;
			}
		}
		// End right side
		ImGui::Columns(1);
		if (m_ViewportFocused) ActiveLayerIndex = m_LayerNumber;
		
		ImGui::PopItemWidth();
		ImGui::End();
		ImGui::PopStyleVar();	
		
	}

	void ConfigLayer::OnEvent(Event& event){
		//std::cout << event.ToString() << std::endl;
	}

}