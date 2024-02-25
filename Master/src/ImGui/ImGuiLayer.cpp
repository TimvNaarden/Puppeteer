#include "pch.h"
#include "ImGuiLayer.h"

#include "imgui.h"
#include "imgui_internal.h"

#define IMGUI_IMPL_API
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"


#include "Core/Application.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Puppeteer
{
	void ImGuiLayer::OnAttach()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans/OpenSans-Regular.ttf", 15.0f);

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		SetThemeColors();

		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");
	}

	void ImGuiLayer::OnDetach()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnEvent(Event& e)
	{
		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}

	void ImGuiLayer::Begin()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		static bool dockingspace_show = true;

		// Ensure that the docking space is taking up the entire window
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		// Window settings of the main docking space
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		// Select all of the styles of the docking space
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);                // Rounding of the main docking space
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);                // The border thickness of the main docking space
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));    // 0 Padding of the docking space relative to the window

		ImGui::Begin("Docking Space", &dockingspace_show, window_flags);

		// Pop all of the pushed style vars
		ImGui::PopStyleVar(3);

		// Submit the docking space
		dockspace_id = ImGui::GetID("DockSpace");

		//ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
		if (!ImGui::DockBuilderGetNode(dockspace_id)) {
			m_Initialized = 1;
			// Start the dock builder
			ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
			ImGui::DockBuilderAddNode(dockspace_id); // Add empty node to dockspace
			ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
			ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.15f, NULL, &dock_left_id);

			//ImGui::DockBuilderDockWindow("Settings", dock_main_id);
			//ImGui::DockBuilderDockWindow("Settings1", dock_main_id);

			// Finish building dockspace
			ImGui::DockBuilderFinish(dockspace_id);
		}
		ImGuiDockNode* ndoes = ImGui::DockBuilderGetNode(dockspace_id);
		ImGuiID id = ImGui::GetWindowDockID();
		ImGui::DockSpace(dockspace_id);
		
	
	}

	void ImGuiLayer::End()
	{
		// End the docking space
		ImGui::End();


		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	static ImVec4 FromRGB(int r, int g, int b)
	{
		return ImVec4{ r / 255.0f, g / 255.0f, b / 255.0f, 1.0f };
	}

	void ImGuiLayer::SetThemeColors()
	{
		auto& style = ImGui::GetStyle();
		style.TabRounding = 4.0f;
		style.ChildRounding = 3.0f;
		style.FrameRounding = 3.0f;
		style.GrabRounding = 3.0f;
		style.WindowRounding = 3.0f;
		style.PopupRounding = 3.0f;

		style.Colors[ImGuiCol_WindowBg] = FromRGB(30, 30, 46);
		style.Colors[ImGuiCol_MenuBarBg] = FromRGB(24, 24, 37);

		// Header
		style.Colors[ImGuiCol_Header] = FromRGB(49, 50, 68);
		style.Colors[ImGuiCol_HeaderHovered] = FromRGB(69, 71, 90);
		style.Colors[ImGuiCol_HeaderActive] = FromRGB(49, 50, 68);

		// Buttons
		style.Colors[ImGuiCol_Button] = FromRGB(49, 50, 68);
		style.Colors[ImGuiCol_ButtonHovered] = FromRGB(69, 71, 90);
		style.Colors[ImGuiCol_ButtonActive] = FromRGB(49, 50, 68);

		// Frame BG
		style.Colors[ImGuiCol_FrameBg] = FromRGB(49, 50, 68);
		style.Colors[ImGuiCol_FrameBgHovered] = FromRGB(69, 71, 90);
		style.Colors[ImGuiCol_FrameBgActive] = FromRGB(49, 50, 68);

		// Tabs
		style.Colors[ImGuiCol_Tab] = FromRGB(137, 180, 250);
		style.Colors[ImGuiCol_TabHovered] = FromRGB(137, 180, 250);
		style.Colors[ImGuiCol_TabActive] = FromRGB(243, 139, 168);
		style.Colors[ImGuiCol_TabUnfocused] = FromRGB(235, 160, 172);
		style.Colors[ImGuiCol_TabUnfocusedActive] = FromRGB(137, 180, 250);

		// Title
		style.Colors[ImGuiCol_TitleBg] = FromRGB(24, 24, 37);
		style.Colors[ImGuiCol_TitleBgActive] = FromRGB(30, 30, 46);
		style.Colors[ImGuiCol_TitleBgCollapsed] = FromRGB(127, 132, 156);
	}
}