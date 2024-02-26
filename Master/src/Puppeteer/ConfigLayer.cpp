#include "pch.h"
#include "ConfigLayer.h"
#include "imgui_internal.h"
#include "Store/StoreJson.h"

#include "misc/cpp/imgui_stdlib.cpp"
namespace Puppeteer
{

	ConfigLayer::ConfigLayer()	{
		if (CheckJsonTable("Puppeteer")) {
			CreateJsonTable("Puppeteer");
		}
		else {
			std::vector<std::map<std::string, std::string >> CredentialsVector = ExtractJsonTable<std::map<std::string, std::string>>("Puppeteer");
			if (CredentialsVector.size() == 0) {
				return;
			}
			std::map<std::string, std::string> Credentials = CredentialsVector[0];
			Username = Credentials["username"];
			Password = Credentials["password"];
			Domain = Credentials["domain"];
			Ip = Credentials["ip"];
			std::vector<std::map<std::string, std::string>> pcs = ParseJson<std::vector<std::map<std::string, std::string>>>(Credentials["pcs"].data());
				for (std::map<std::string, std::string> pc: pcs) {
				PcInfos.push_back(PCInfo(pc));
			}

		}
	}
	void ConfigLayer::OnAttach()
	{
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

	void ConfigLayer::OnDetach()
	{
		glDeleteTextures(1, &m_TextureID);
	}

	void ConfigLayer::OnUpdate(float dt)
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
	static int IpCompletionCallBack(ImGuiInputTextCallbackData* data) {
		if (data->EventKey == ImGuiKey_Tab) ImGui::SetKeyboardFocusHere(0);
		return 0;
	}
	void ConfigLayer::OnImGuiRender()
	{
		// Viewport
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });

		ImGui::SetNextWindowDockID(2, ImGuiCond_FirstUseEver);
		ImGui::Begin("Settings");

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

		ImGui::PushItemWidth(150);
		ImGui::LabelText("##username_label", "Username: ");
		if (ImGui::InputText("##username_input", &Username, ImGuiInputTextFlags_EnterReturnsTrue)) {
			ImGui::SetKeyboardFocusHere(0);
		}
		ImGui::LabelText("##password_label", "Password: ");
		if (ImGui::InputText("##password_input", &Password, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue)) {
			ImGui::SetKeyboardFocusHere(0);
		}
		ImGui::LabelText("##domain_label", "Domain: ");
		if (ImGui::InputText("##domain_input", &Domain, ImGuiInputTextFlags_EnterReturnsTrue)) {
			ImGui::SetKeyboardFocusHere(0);
		}
		ImGui::LabelText("##ip_label", "Ip: ");
		if (ImGui::InputText("##ip_input", &Ip, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion, IpCompletionCallBack)) {
			ImGui::SetKeyboardFocusHere(0);
		}

		
		if (ImGui::Button("Connect") || (ImGui::IsItemFocused() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeyPadEnter)))) {
			std::map<std::string, char*> Credentials = { {"username", Username.data()}, {"password", Password.data()}, {"domain", Domain.data()}};
			std::string sCredentials = WriteJson(Credentials);
			std::vector<std::map<std::string, std::string>> pcs;
			for (PCInfo pc : PcInfos) {
				pcs.push_back(pc.toMap());
			}
			std::map<std::string, std::string> Save = { {"username", Username}, {"ip", Ip}, {"domain", Domain}, {"pcs", WriteJson(pcs)} };
			OverrideJsonTable("Puppeteer", WriteJson(Save));
			app->PushLayer(new PuppetLayer(Ip.data(), sCredentials));
		}
		ImGui::PopItemWidth();
		ImGui::End();
		ImGui::PopStyleVar();
				
	}

	void ConfigLayer::OnEvent(Event& event)
	{
		//std::cout << event.ToString() << std::endl;
	}
}