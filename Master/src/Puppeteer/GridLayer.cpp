#include "pch.h"
#include "GridLayer.h"
#include <lz4.h>
#include <thread>
#include "Puppeteer/PuppetLayer.h"
#include <mutex>
#include  <nfd.h>
static GLuint CreateBlackTexture(int width, int height) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Create a black image
	unsigned char* data = new unsigned char[width * height * 3]; // 3 channels for RGB
	memset(data, 0, width * height * 3);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Upload texture data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] data;

	return textureID;
}
namespace Puppeteer {
	Action_T GridAction = {ActionType::Screen, 0, 0, 0, 0};
	GridLayer::GridLayer() {
		m_Fps = 0;
		m_TextureID = 0;

		m_LayerNumber = LayerCount;
		LayerCount++;

		m_Lastrun = std::time(0);

		blackImage = (ImTextureID)CreateBlackTexture(300, 170);
	}

	void GridLayer::OnAttach() {
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

	void GridLayer::OnDetach() {
		glDeleteTextures(1, &m_TextureID);
	}

	void GridLayer::OnUpdate(float dt) {
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
	ImVec2 CalculateImageSize(ImVec2 imageSize, ImVec2 screenSize) {
		if (screenSize.x >= imageSize.x && screenSize.y >= imageSize.y) return imageSize;
		else if (screenSize.x < imageSize.x && screenSize.y >= imageSize.y) {
			float m_ImageRatio = screenSize.x / imageSize.x;
			return ImVec2(screenSize.x, imageSize.y * m_ImageRatio);
		}
		else if (screenSize.x >= imageSize.x && screenSize.y < imageSize.y) {
			float m_ImageRatio = screenSize.y / imageSize.y;
			return ImVec2(imageSize.x * m_ImageRatio, screenSize.y);
		}
		else {
			float xRatio = screenSize.x / imageSize.x;
			float yRatio = screenSize.y / imageSize.y;
			float m_ImageRatio = (xRatio < yRatio) ? xRatio : yRatio;
			return ImVec2(imageSize.x * m_ImageRatio, imageSize.y * m_ImageRatio);
		}
		return screenSize;

	} 
	void GridLayer::OnImGuiRender() {
		// Viewport
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });

		ImGui::SetNextWindowDockID(2, ImGuiCond_FirstUseEver);
		ImGui::Begin("Clients");

		if (m_ViewportFocused) ActiveLayerIndex = m_LayerNumber;

		ImVec2 main = ImGui::GetMainViewport()->Pos;
		ImVec2 pos = ImGui::GetWindowPos();

		m_ViewportOffset = glm::vec2{ pos.x, pos.y } - glm::vec2{ main.x, main.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused && !m_ViewportHovered);
		if (m_First || (std::time(0) - m_Lastrun) >= 5) {
			m_First = false;
			std::thread(&GridLayer::GetImages, this).detach();
			m_Lastrun = std::time(0);
		}
		float WindowWidth = ImGui::GetWindowWidth();
		ImGui::SetCursorPosX(10);
		ImGui::SetCursorPosY(25);
		if (m_CurrentImages.size() == 0 && m_Images.empty()) {
			for (GridClient_T client : GridClients) {
				ImGui::BeginGroup();
				ImGui::Image(blackImage, ImVec2(300, 170));
				float textx = ImGui::CalcTextSize(client.Name).x;
				ImGui::SetCursorPosX((300 - textx) / 2);
				ImGui::TextWrapped(client.Name);
				ImGui::EndGroup();
				
				WindowWidth -= 310;
				if(WindowWidth > 320) ImGui::SameLine(0, 10);
				else {
					WindowWidth = ImGui::GetWindowWidth();
					ImGui::SetCursorPosX(10);
				}
			}
			ImGui::End();
			ImGui::PopStyleVar();
			return;
		}
		if (!m_Images.empty()) {
			m_CurrentImages.clear();
			m_CurrentImages = m_Images.front();
			m_Images.pop();
		}
		for (GridClient_T client : GridClients) {
			if (m_Clients.count(client.Name) == 0) {
				ImGui::BeginGroup();
				ImGui::Image(blackImage, ImVec2(300, 170));
				float textx = ImGui::CalcTextSize(client.Name).x;
				ImGui::SetCursorPosX((300 - textx) / 2);
				ImGui::TextWrapped(client.Name);
				ImGui::EndGroup();

				WindowWidth -= 310;
				if (WindowWidth > 320) ImGui::SameLine(0, 10);
				else {
					WindowWidth = ImGui::GetWindowWidth();
					ImGui::SetCursorPosX(10);
				}
			}
		}
		for (GridImage image : m_CurrentImages) {
			if (m_Clients.count(image.name) == 0) {
				ImGui::End();
				ImGui::PopStyleVar();
				return;
			}
			ImGui::BeginGroup();
			GLuint Texture = 0;
			glBindTexture(GL_TEXTURE_2D, Texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.data.Width, image.data.Height, 0, GL_BGRA, GL_UNSIGNED_BYTE, image.data.Texture);
			glGenerateMipmap(GL_TEXTURE_2D);
			ImGui::Image((ImTextureID)Texture, CalculateImageSize({(float)image.data.Width, (float)image.data.Height}, { 300, 300 }), ImVec2{ 0, -1 }, ImVec2{ 1, 0 });
			if (ImGui::IsItemHovered()) {
				if (ImGui::IsMouseDoubleClicked(0)) {
					app->PushLayer(new PuppetLayer(m_Clients[image.name], image.ip, Credentials, &m_Mutex));
				}
			}
			glDeleteTextures(1, &Texture);
			float textx = ImGui::CalcTextSize(image.name).x;
			ImGui::SetCursorPosX((300 - textx) / 2);
			ImGui::TextWrapped(image.name);
			ImGui::EndGroup();

			WindowWidth -= 310;
			if (WindowWidth > 320) ImGui::SameLine(0, 10);
			else {
				WindowWidth = ImGui::GetWindowWidth();
				ImGui::SetCursorPosX(10);
			}
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void GridLayer::GetImages() {
		std::vector<GridImage> ForQue = {};
		for (GridClient_T client : GridClients) {
			GridImage ForVec = { {}, new char[255], new char[255]};
			if (m_Clients.count(client.Name) == 0) {
				m_Mutex.lock();
				Networking::TCPClient s(Networking::IPV4, 54000, client.Ip, 1);
				if (s.m_Connected == 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}
				if (s.Send((char*)&Credentials, sizeof(Credentials)) != 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}
				char* response = 0;
				if (s.Receive(response) != 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}
				if (strcmp(response, "Not Authenticated") == 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}
				GridAction.Type = ActionType::ReqPCInfo;
				if (s.Send((char*)&GridAction, sizeof(GridAction)) != 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}
				if (s.Receive(response) != 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}
				if (response[0] < 32 || response[0] > 126 || response[1] < 32 || response[1] > 126) {
					m_Mutex.unlock();
					return;
				}
				int inPCSvec = 0;
				PCInfo info = PCInfo(ParseJson<std::map<std::string, std::string>>(response));
				for (PCInfo pc : PcInfos) {
					if (pc.m_Systemname == info.m_Systemname) {
						inPCSvec = 1;
						break;
					}
				}
				if (!inPCSvec) PcInfos.push_back(info);
				SAVE();

				GridAction.Type = ActionType::Screen;
				if (s.Send((char*)&GridAction, sizeof(GridAction)) != 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}	
				if (s.Receive(response) != 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}

				std::map<std::string, int> ResponseMap = ParseJson<std::map<std::string, int>>(response);
				ForVec.data = { ResponseMap["width"], ResponseMap["height"], new char[ResponseMap["size"]]};

				if (s.Receive(response) != 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}
				m_Mutex.unlock();
				LZ4_decompress_safe(response, ForVec.data.Texture, ResponseMap["csize"], ResponseMap["size"]);
				delete[] response;
				strcpy(ForVec.name, client.Name);
				strcpy(ForVec.ip, client.Ip);
				ForQue.emplace_back(ForVec);
				m_Clients.emplace(client.Name, s);
			}
			else {
				char* response = 0;
				Networking::TCPClient *s = &m_Clients[client.Name];
				GridAction.Type = ActionType::ReqPCInfo;
				if (s->Send((char*)&GridAction, sizeof(GridAction)) != 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}
				if (s->Receive(response) != 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}
				if (response[0] < 32 || response[0] > 126 || response[1] < 32 || response[1] > 126) {
					m_Mutex.unlock();
					return;
				}
				int inPCSvec = 0;
				PCInfo info = PCInfo(ParseJson<std::map<std::string, std::string>>(response));
				for (PCInfo pc : PcInfos) {
					if (pc.m_Systemname == info.m_Systemname) {
						inPCSvec = 1;
						break;
					}
				}
				if (!inPCSvec) PcInfos.push_back(info);
				SAVE();

				GridAction.Type = ActionType::Screen;
				m_Mutex.lock();
				if (s->Send((char*)&GridAction, sizeof(GridAction)) != 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}
				if (s->Receive(response) != 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}
				if (response[0] < 32 || response[0] > 126 || response[1] < 32 || response[1] > 126) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}
				std::map<std::string, int> ResponseMap = ParseJson<std::map<std::string, int>>(response);
				ForVec.data = { ResponseMap["width"], ResponseMap["height"], new char[ResponseMap["size"]] };

				if (s->Receive(response) != 0) {
					m_Clients.erase(client.Name);
					m_Mutex.unlock();
					return;
				}
				m_Mutex.unlock();
				LZ4_decompress_safe(response, ForVec.data.Texture, ResponseMap["csize"], ResponseMap["size"]);
				delete[] response;
				strcpy(ForVec.name, client.Name);
				strcpy(ForVec.ip, client.Ip);
				ForQue.emplace_back(ForVec);
			}
		} 
		m_Images.emplace(ForQue);
	}
}