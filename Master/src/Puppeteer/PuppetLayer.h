#pragma once

#include "TCP/TCPClient.h"
#include "Core/Layer.h"
#include "Core/Application.h"
#include "Renderer/Framebuffer.h"

#include "Platform/Windows/DirectX11/DirectX11.h"
#include "Platform/Windows/PCInfo/PCInfo.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

#include "imgui_internal.h"
#include <imgui.h>

#include <future>
#include <thread>
#include <queue>	
#include <mutex>

#include <Windows.h>
#include <lz4.h>
#include "Store/StoreJson.h"

namespace Puppeteer
{
	typedef struct ImageData
	{
		int Width, Height;
		char* Texture;
	};

	class PuppetLayer : public Layer
	{
	public:
		PCInfo m_PCInfo;	
		GLuint m_Texture;	
		std::queue<ImageData> m_Textures;
		std::mutex m_Mutex;
		
		char* m_Ip;
		Networking::TCPClient* m_Socket;
		Credentials_T m_Credentials;

		char* m_Name;
		bool m_Initialized;

		PuppetLayer() = delete;
		PuppetLayer(char* Ip, Credentials_T Creds);
		~PuppetLayer() {
			m_Mutex.lock();
			m_UpdatingTexture = false;
			if (m_Texture) glDeleteTextures(1, &m_Texture);
			if(m_TextureID) glDeleteTextures(1, &m_TextureID);
			m_Mutex.unlock();
		}

		void UpdateInfo();
		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(float dt) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& event) override;
	private:
		void UpdateTexture();
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

	static std::map<int, int> MouseDown = {
		{ImGuiKey_MouseLeft, 0x0002},
		{ImGuiKey_MouseRight, 0x0008},
		{ImGuiKey_MouseMiddle, 0x0020},
		{ImGuiKey_MouseX1, 0x0080},
	};
	static std::map<int, int> MouseUp = {
		{ImGuiKey_MouseLeft, 0x0004}, 
		{ImGuiKey_MouseRight, 0x0010},
		{ImGuiKey_MouseMiddle, 0x0040},
		{ImGuiKey_MouseX1, 0x0100},
	};
	static std::map<int, int> ImGuiToWin = {
		{ImGuiKey_Backspace, 0x0008},
		{ImGuiKey_Tab, 0x0009},
		{ImGuiKey_Enter, 0x000D},
		{ImGuiKey_Pause, 0x0013},
		{ImGuiKey_CapsLock, 0x0014},
		{ImGuiKey_Escape, 0x001B},
		{ImGuiKey_Space, 0x0020},
		{ImGuiKey_PageUp, 0x0021},
		{ImGuiKey_PageDown, 0x0022},
		{ImGuiKey_End, 0x0023},
		{ImGuiKey_Home, 0x0024},

		{ImGuiKey_LeftArrow, 0x0025},
		{ImGuiKey_UpArrow, 0x0026},
		{ImGuiKey_RightArrow, 0x0027},
		{ImGuiKey_DownArrow, 0x0028},

		{ImGuiKey_Insert, 0x002D},
		{ImGuiKey_Delete, 0x002E},

		{ImGuiKey_0, 0x0030},
		{ImGuiKey_1, 0x0031},
		{ImGuiKey_2, 0x0032},
		{ImGuiKey_3, 0x0033},
		{ImGuiKey_4, 0x0034},
		{ImGuiKey_5, 0x0035},
		{ImGuiKey_6, 0x0036},
		{ImGuiKey_7, 0x0037},
		{ImGuiKey_8, 0x0038},
		{ImGuiKey_9, 0x0039},

		{ImGuiKey_A, 0x0041},
		{ImGuiKey_B, 0x0042},
		{ImGuiKey_C, 0x0043},
		{ImGuiKey_D, 0x0044},
		{ImGuiKey_E, 0x0045},
		{ImGuiKey_F, 0x0046},
		{ImGuiKey_G, 0x0047},
		{ImGuiKey_H, 0x0048},
		{ImGuiKey_I, 0x0049},
		{ImGuiKey_J, 0x004A},
		{ImGuiKey_K, 0x004B},
		{ImGuiKey_L, 0x004C},
		{ImGuiKey_M, 0x004D},
		{ImGuiKey_N, 0x004E},
		{ImGuiKey_O, 0x004F},
		{ImGuiKey_P, 0x0050},
		{ImGuiKey_Q, 0x0051},
		{ImGuiKey_R, 0x0052},
		{ImGuiKey_S, 0x0053},
		{ImGuiKey_T, 0x0054},
		{ImGuiKey_U, 0x0055},
		{ImGuiKey_V, 0x0056},
		{ImGuiKey_W, 0x0057},
		{ImGuiKey_X, 0x0058},
		{ImGuiKey_Y, 0x0059},
		{ImGuiKey_Z, 0x005A},

		{ImGuiKey_Keypad0, 0x0060},
		{ImGuiKey_Keypad1, 0x0061},
		{ImGuiKey_Keypad2, 0x0062},
		{ImGuiKey_Keypad3, 0x0063},
		{ImGuiKey_Keypad4, 0x0064},
		{ImGuiKey_Keypad5, 0x0065},
		{ImGuiKey_Keypad6, 0x0066},
		{ImGuiKey_Keypad7, 0x0067},
		{ImGuiKey_Keypad8, 0x0068},
		{ImGuiKey_Keypad9, 0x0069},
		{ImGuiKey_KeypadMultiply, 0x006A},
		{ImGuiKey_KeypadAdd, 0x006B},
		{ImGuiKey_KeypadSubtract, 0x006D},
		{ImGuiKey_KeypadDecimal, 0x006E},
		{ImGuiKey_KeypadDivide, 0x006F},

		{ImGuiKey_F1, 0x0070},
		{ImGuiKey_F2, 0x0071},
		{ImGuiKey_F3, 0x0072},
		{ImGuiKey_F4, 0x0073},
		{ImGuiKey_F5, 0x0074},
		{ImGuiKey_F6, 0x0075},
		{ImGuiKey_F7, 0x0076},
		{ImGuiKey_F8, 0x0077},
		{ImGuiKey_F9, 0x0078},
		{ImGuiKey_F10, 0x0079},
		{ImGuiKey_F11, 0x007A},
		{ImGuiKey_F12, 0x007B},

		{ImGuiKey_NumLock, 0x0090},
		{ImGuiKey_ScrollLock, 0x0091},

		{ImGuiKey_LeftShift, 0xA0},
		{ImGuiKey_RightShift, 0xA1},
		{ImGuiKey_LeftCtrl, 0xA2},
		{ImGuiKey_RightCtrl, 0xA3},
		{ImGuiKey_LeftAlt, 0xA4},
		{ImGuiKey_RightAlt, 0xA5},
		{ImGuiKey_LeftSuper, 0x5B},
		{ImGuiKey_RightSuper, 0x5C},
		{ImGuiKey_Menu, 0x5D},

		{ImGuiKey_Semicolon, 0xBA},
		{ImGuiKey_Equal, 0xBB},
		{ImGuiKey_Comma, 0xBC},
		{ImGuiKey_Minus, 0xBD},
		{ImGuiKey_Period, 0xBE},
		{ImGuiKey_Slash, 0xBF},
		{ImGuiKey_LeftBracket, 0xDB},
		{ImGuiKey_Backslash, 0xDC},
		{ImGuiKey_RightBracket, 0xDD},
		{ImGuiKey_Apostrophe, 0xDE},
		{ImGuiKey_GraveAccent, 0xC0},

	};
}