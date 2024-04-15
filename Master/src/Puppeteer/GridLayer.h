#pragma once

namespace Puppeteer {
	typedef struct GridImage {
		ImageData data;
		char* name;
		char* ip;
		char* username;
	};
	typedef struct GridClient_T {
		char Name[256];
		char Ip[256];
		GridClient_T() {
			std::memset(Name, 0, sizeof(Name));
			Name[255] = '\0';
			std::memset(Ip, 0, sizeof(Ip));
			Ip[255] = '\0';
		};
		GridClient_T(std::string input) {
			std::memset(Name, 0, sizeof(Name));
			Name[255] = '\0';
			std::memset(Ip, 0, sizeof(Ip));
			Ip[255] = '\0';
			int sep = input.find(",");
			std::string name = input.substr(0, sep);
			std::string ip = input.substr(sep + 1);
			std::memcpy(Name, name.c_str(), name.size());
			std::memcpy(Ip, ip.c_str(), ip.size());
		};
		std::string toString() {
			return std::string(Name) + "," + std::string(Ip);
		}
	};

	class GridLayer : public Layer {
		public:
			GridLayer();
			virtual void OnAttach() override;
			virtual void OnDetach() override;
			virtual void OnUpdate(float dt) override;
			virtual void OnImGuiRender() override;

		private:
			Ref<Framebuffer> m_Framebuffer;
			uint32_t m_TextureID;
		
			std::mutex m_Mutex;

			bool m_ViewportFocused = false, m_ViewportHovered = false;
			glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
			glm::vec2 m_ViewportOffset = { 0.0f, 0.0f };
			bool m_First = true;

			int m_LayerNumber;

			std::time_t m_Lastrun;
			void GetImages();

			float m_Fps;
		
			std::map<std::string, Networking::TCPClient> m_Clients;

			std::queue<std::vector<GridImage>> m_Images;
			std::vector<GridImage> m_CurrentImages;
			GLuint* textures;

			GLuint m_BlackTexture;

	};
}