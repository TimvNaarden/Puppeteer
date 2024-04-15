#include "pch.h"
#include "InfoLayer.h"


namespace Puppeteer {

	InfoLayer::InfoLayer() {	
		m_ActiveIndex = 0;
		m_TextureID = 0;

		m_LayerNumber = LayerCount;
		LayerCount++;
	}

	void InfoLayer::OnAttach() {
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


	void InfoLayer::OnDetach() {
		glDeleteTextures(1, &m_TextureID);
	}

	void InfoLayer::OnUpdate(float dt) {
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
	static std::string VecToString(std::vector<std::string> vec) {
		std::string out = "";
		for (std::string s : vec) {
			out += s + ", ";
		}
		return out;
	}
	static std::string VecToString(std::vector<UINT64> vec) {
		std::string out = "";
		for (UINT64 s : vec) {
			out += std::to_string(s) + ", ";
		}
		return out;
	}

	static std::string VecToString(std::vector<UINT32> vec) {
		std::string out = "";
		for (UINT32 s : vec) {
			out += std::to_string(s) + ", ";
		}
		return out;
	}

	void InfoLayer::OnImGuiRender() 	{
		if (PcInfos.size() == 0) return;
		
		ImGui::SetNextWindowDockID(2, ImGuiCond_FirstUseEver);
		ImGui::Begin("Info");

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

		std::stringstream ss;
		ss << PcInfos[m_ActiveIndex];
		ImGui::Text(ss.str().c_str());
		int focused = ImGui::IsWindowFocused();

		if (ImGui::Button("Delete PC Info")) {
			PcInfos.erase(PcInfos.begin() + m_ActiveIndex);
			m_ActiveIndex--;
			SAVE();
		}
		ImGui::SameLine(0, 10);

		if (ImGui::Button("Export PC Info")) {
			NFD_Init();
			nfdchar_t* outPath;
			nfdresult_t result = NFD_PickFolder(&outPath, NULL);
			if (result == NFD_OKAY) {
				OpenXLSX::XLDocument doc;
				doc.create(outPath + std::string("\\PCInfoExport.xlsx"));
				OpenXLSX::XLWorksheet sheet = doc.workbook().worksheet("Sheet1");
				sheet.cell("A1").value() = "Systemname";
				sheet.cell("B1").value() = "Processor";
				sheet.cell("C1").value() = "Cores";
				sheet.cell("D1").value() = "Threads";
				sheet.cell("E1").value() = "Base Clock Speed";
				sheet.cell("F1").value() = "Motherboard";
				sheet.cell("G1").value() = "Memory Type";
				sheet.cell("H1").value() = "Memory Speed";
				sheet.cell("I1").value() = "Memory Dimms";
				sheet.cell("J1").value() = "Memory Size";
				sheet.cell("K1").value() = "Graphics Card";
				sheet.cell("L1").value() = "Virtual Memory";
				sheet.cell("M1").value() = "Storage Type";
				sheet.cell("N1").value() = "Storage Size";
				sheet.cell("O1").value() = "Storage Names";
				sheet.cell("P1").value() = "MAC Addresses";
				int index = 1;
				for (PCInfo info : PcInfos) {
					index++;
					sheet.cell("A" + std::to_string(index)).value() = info.m_Systemname;
					sheet.cell("B" + std::to_string(index)).value() = info.m_CPUName;
					sheet.cell("C" + std::to_string(index)).value() = info.m_Cores;
					sheet.cell("D" + std::to_string(index)).value() = info.m_ThreadCount;
					sheet.cell("E" + std::to_string(index)).value() = info.m_BaseClockSpeed;
					sheet.cell("F" + std::to_string(index)).value() = info.m_Mob;
					sheet.cell("G" + std::to_string(index)).value() = info.m_MemoryType;
					sheet.cell("H" + std::to_string(index)).value() = info.m_MemorySpeed;
					sheet.cell("I" + std::to_string(index)).value() = info.m_MemoryDimms;
					sheet.cell("J" + std::to_string(index)).value() = info.m_MemoryCapacity;
					sheet.cell("K" + std::to_string(index)).value() = VecToString(info.m_VideoName);
					sheet.cell("L" + std::to_string(index)).value() = VecToString(info.m_Vram);
					sheet.cell("M" + std::to_string(index)).value() = VecToString(info.m_DiskType);
					sheet.cell("N" + std::to_string(index)).value() = VecToString(info.m_DiskSize);
					sheet.cell("O" + std::to_string(index)).value() = VecToString(info.m_StorageNames);
					sheet.cell("P" + std::to_string(index)).value() = VecToString(info.m_MACAddres);
				}

				doc.save();

				Error = "Exported PC Info to " + std::string(outPath);
				NFD_FreePath(outPath);
				modalOpen = true;
				
			} else if (result == NFD_ERROR) {
				Error = "Error: " + std::string(NFD_GetError());
				modalOpen = true;
			}
			NFD_Quit();
		}

		ImGui::End();
		if (ActiveLayerIndex != m_LayerNumber) return; 
		
		ImGui::SetNextWindowDockID(1); // ImGuiCond_FirstUseEver);
		ImGui::Begin("Select PC:");

		int index = 0;
		for (PCInfo info : PcInfos) {
			if (ImGui::Button(info.m_Systemname.c_str())) {
				m_ActiveIndex = index;
			}
			index++;
		}

		ImGui::End();
	}
	void InfoLayer::OnEvent(Event& event) {

	}
}