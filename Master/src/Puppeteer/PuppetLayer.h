#pragma once

namespace Puppeteer {
	typedef struct ImageData {
		int Width, Height;
		char* Texture;
	};

	typedef struct Credentials_T {
		char Username[256];
		char Password[256];
		char Domain[256];

		Credentials_T() {
			std::memset(Username, 0, sizeof(Username));
			Username[255] = '\0';
			std::memset(Password, 0, sizeof(Password));
			Password[255] = '\0';
			std::memset(Domain, 0, sizeof(Domain));
			Domain[255] = '\0';
		}
	};

	class PuppetLayer : public Layer {
		public:
			PCInfo m_PCInfo;	
			GLuint m_Texture;	
			std::queue<ImageData> m_Textures;
			std::mutex m_Mutex;
			std::mutex* m_MutexPtr = &m_Mutex;
		
			char* m_Ip;
			Networking::TCPClient m_SocketNormal;
			Networking::TCPClient* m_Socket = &m_SocketNormal;
			Credentials_T m_Credentials;

			char* m_Name;
			bool m_Initialized;

			int m_keepSocketAlive = 0;

			PuppetLayer() = delete;
			PuppetLayer(char* Ip, Credentials_T Creds);
			PuppetLayer(Networking::TCPClient s, char* ip, Credentials_T Creds, std::mutex* mut);
			~PuppetLayer() {
				m_MutexPtr->lock();
				m_UpdatingTexture = false;
				if (m_Texture) glDeleteTextures(1, &m_Texture);
				if(m_TextureID) glDeleteTextures(1, &m_TextureID);
				m_MutexPtr->unlock();
			}
			virtual void OnAttach() override;
			virtual void OnDetach() override;
			virtual void OnUpdate(float dt) override;
			virtual void OnImGuiRender() override;
			virtual void OnEvent(Event& event) override;
		private:
			void UpdateTexture(Networking::TCPClient s);
			bool m_UpdatingTexture;
			bool m_Input;
			bool m_UserInput;

			ImVec2 m_ImageSize;
			float m_ImageRatio;
			void CalculateImageSize(ImVec2 ScreenSize, ImVec2 imageSize);

			Ref<Framebuffer> m_Framebuffer;
			uint32_t m_TextureID;

			bool m_ViewportFocused = false, m_ViewportHovered = false;
			glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
			glm::vec2 m_ViewportOffset = { 0.0f, 0.0f };

			float m_Fps;
			int m_LayerNumber;

			void KeyPressed(int key);
			void KeyReleased(int key);

			void MouseButton(int flags);
			void MouseMoves(int x, int y);
			void MouseScrolled(int offset);
			ImVec2 m_LastMousePos;
	};
}