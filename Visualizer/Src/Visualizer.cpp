// InputVisualizer.cpp
//
// A simple OpenGL/ImGui app for testing the controller input system.
//
// Copyright (c) 2025 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#ifdef PLATFORM_WINDOWS
#include <dwmapi.h>
#include <locale.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>				// Include glfw3.h after our OpenGL declarations.
#ifdef PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif
#include <Foundation/tVersion.cmake.h>
//#include <Foundation/tHash.h>
//#include <System/tCmdLine.h>
//#include <Image/tPicture.h>
#include <Image/tImageICO.h>
//#include <Image/tImageTGA.h>		// For paste from clipboard.
#include <Image/tImagePNG.h>		// For paste from clipboard.
//#include <Image/tImageWEBP.h>		// For paste from clipboard.
//#include <Image/tImageQOI.h>		// For paste from clipboard.
//#include <Image/tImageBMP.h>		// For paste from clipboard.
//#include <Image/tImageTIFF.h>		// For paste from clipboard.
#include <System/tFile.h>
#include <System/tTime.h>
#include <System/tScript.h>
#include <System/tMachine.h>
#include <Math/tVector2.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "imgui_internal.h"			// For ImGuiWindow.

#if 0
#include "TacentView.h"
#include "GuiUtil.h"
#include "Image.h"
#include "ColourDialogs.h"
#include "ImportRaw.h"
#include "Dialogs.h"
#include "Details.h"
#include "Preferences.h"
#include "Properties.h"
#include "ContactSheet.h"
#include "MultiFrame.h"
#include "ThumbnailView.h"
#include "Crop.h"
#include "Quantize.h"
#include "Resize.h"
#include "Rotate.h"
#include "OpenSaveDialogs.h"
#include "Config.h"
#include "InputBindings.h"
#include "Command.h"
#endif
#include "RobotoFontBase85.cpp"
//#include "Version.cmake.h"

using namespace tStd;
using namespace tSystem;
using namespace tMath;


namespace Visualizer
{
	enum ErrorCode
	{
		ErrorCode_Success					= 0,
		ErrorCode_GUI_FailGLFWInit			= 10,
		ErrorCode_GUI_FailGLFWWindow		= 20,
		ErrorCode_GUI_FailGLADInit			= 30,
		ErrorCode_GUI_FailAssetDirMissing	= 40,
		ErrorCode_GUI_FailConfigDirMissing	= 50,
		ErrorCode_GUI_FailCacheDirMissing	= 60,

		ErrorCode_CLI_FailUnknown			= 100,
		ErrorCode_CLI_FailImageLoad			= 110,
		ErrorCode_CLI_FailImageProcess		= 120,
		ErrorCode_CLI_FailEarlyExit			= 130,
		ErrorCode_CLI_FailImageSave			= 140,
	};

	GLFWwindow* Window								= nullptr;
	bool WindowIconified							= false;

//	OutputLog OutLog;

#if 0

	bool LMBDown									= false;
	bool RMBDown									= false;
	int DragAnchorX									= 0;
	int DragAnchorY									= 0;

	// CursorX/Y are the real position. CursorMouseX/Y are updated when mouse clicked and update CursorX/Y.
	int CursorX										= 0;
	int CursorY										= 0;
	float CursorMouseX								= -1.0f;
	float CursorMouseY								= -1.0f;
#endif
	int DispW										= 1;
	int DispH										= 1;
#if 0
	int PanOffsetX									= 0;
	int PanOffsetY									= 0;
	int PanDragDownOffsetX							= 0;
	int PanDragDownOffsetY							= 0;
	tColour4b PixelColour							= tColour4b::black;

	const tVector4 ColourEnabledTint				= tVector4(1.00f, 1.00f, 1.00f, 1.00f);
	const tVector4 ColourDisabledTint				= tVector4(0.54f, 0.54f, 0.54f, 1.00f);
	const tVector4 ColourBG							= tVector4(0.00f, 0.00f, 0.00f, 0.00f);
	const tVector4 ColourPressedBG					= tVector4(0.21f, 0.45f, 0.21f, 1.00f);

	#endif
	const tVector4 ColourClear						= tVector4(0.10f, 0.10f, 0.12f, 1.00f);

	#if 0

	// UI scaling and HighDPI support. Scale is driven by decision on min/max font sizes.
	const float MinFontPointSize					= 14.0f;
	const float MaxFontPointSize					= 36.4f;
	const float MinUIScale							= 1.0f;
	const float MaxUIScale							= MaxFontPointSize / MinFontPointSize;
	const float FontStepPointSize					= (MaxFontPointSize - MinFontPointSize) / (int(Config::ProfileData::UISizeEnum::NumSizes) - 1);
	Config::ProfileData::UISizeEnum FontUISizeAdded	= Config::ProfileData::UISizeEnum::Invalid;
	Config::ProfileData::UISizeEnum CurrentUISize	= Config::ProfileData::UISizeEnum::Invalid;
	Config::ProfileData::UISizeEnum DesiredUISize	= Config::ProfileData::UISizeEnum::Invalid;

	uint64 FrameNumber								= 0;

	// This function expects a valid (non-auto) UI size. It updates the style and ImGui font that are in use.
	void SetUISize(Viewer::Config::ProfileData::UISizeEnum);

	void DrawBackground(float l, float r, float b, float t, float drawW, float drawH);
	#endif

	void SetWindowIcon(const tString& icoFile);

	void PrintRedirectCallback(const char* text, int numChars);

	void GlfwErrorCallback(int error, const char* description)															{ tPrintf("Glfw Error %d: %s\n", error, description); }

	#if 0
	enum class MoveDir { Right, Left, Up, Down };
	inline void OnPixelMove(MoveDir);
	inline void OnUISizeInc(bool inc);
	inline void OnZoomIn();
	inline void OnZoomOut();
	inline void OnZoomFit();
	inline void OnZoomDownscaleOnly();
	inline void OnZoomOneToOne();
	inline void OnZoomPerImageToggle();
	inline void OnResetPan();
	inline void OnMenuBar();
	inline void OnNavBar();
	inline void OnDebugLog();
	inline void OnEscape();
	inline void OnEscapeWithQuit();
	inline void OnQuit();

	void ApplyZoomDelta(float zoomDelta);
	void AutoPropertyWindow();

	tString FindImagesInImageToLoadDir(tList<tSystem::tFileInfo>& foundFiles);		// Returns the image folder.
	tuint256 ComputeImagesHash(const tList<tSystem::tFileInfo>& files);
	int RemoveOldCacheFiles(const tString& cacheDir);								// Returns num removed.

	CursorMove RequestCursorMove = CursorMove_None;
	bool IgnoreNextCursorPosCallback = false;
	#endif

	void SetStyleScaleAndFontSize();
	void Update(GLFWwindow* window, double dt, bool dopoll = true);
//	int  DoMainMenuBar();																								// Returns height.
//	int  GetNavBarHeight();
//	void DoNavBar(int dispWidth, int dispHeight, int barHeight);
	void WindowRefreshFun(GLFWwindow* window)
	{
		// WIP
		/////////////Update(window, 0.0, false);
	}
	void KeyCallback(GLFWwindow*, int key, int scancode, int action, int modifiers);
	void MouseButtonCallback(GLFWwindow*, int mouseButton, int x, int y);
	void CursorPosCallback(GLFWwindow*, double x, double y);
	void ScrollWheelCallback(GLFWwindow*, double x, double y);
	void FileDropCallback(GLFWwindow*, int count, const char** paths);
	void FocusCallback(GLFWwindow*, int gotFocus);
	void IconifyCallback(GLFWwindow*, int iconified);
	// void ChangProfile(Viewer::Profile);
}


void Visualizer::PrintRedirectCallback(const char* text, int numChars)
{
//	OutLog.AddLog("%s", text);
	
	#ifdef PLATFORM_LINUX
	// We have a terminal in Linux so use it.
	printf("%s", text);
	#endif
}


#if 0

void Viewer::OnEscapeWithQuit()
{
	Config::ProfileData& profile = Config::GetProfileData();
	if (profile.FullscreenMode)
		ChangeScreenMode(false);
	else if ((Config::GetProfile() == Profile::Basic) || (Config::GetProfile() == Profile::Kiosk))
		ChangProfile(Profile::Main);
	else
		Viewer::Request_Quit = true;
}


void Viewer::OnQuit()				{ Request_Quit = true; }


void Viewer::DrawBackground(float l, float r, float b, float t, float drawW, float drawH)
{
		case Config::ProfileData::BackgroundStyleEnum::SolidColour:
		{
			tColour4b bgCol = profile.BackgroundColour;
			if (overrideBG)
				bgCol = CurrImage->BackgroundColourOverride;
			glColor4ubv(bgCol.E);
			glBegin(GL_QUADS);
			glVertex2f(l, b);
			glVertex2f(l, t);
			glVertex2f(r, t);
			glVertex2f(r, b);
			glEnd();
			break;
		}
	}
}
#endif


void Visualizer::Update(GLFWwindow* window, double dt, bool dopoll)
{
	// Poll and handle events like inputs, window resize, etc. You can read the io.WantCaptureMouse,
	// io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
	//
	// When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
	// When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
	//
	// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those
	// two flags.
	if (dopoll)
		glfwPollEvents();

	glClearColor(ColourClear.x, ColourClear.y, ColourClear.z, ColourClear.w);
	glClear(GL_COLOR_BUFFER_BIT);

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	#ifdef CONFIG_DEBUG
	ImGuiStackSizes stackSizes;
	stackSizes.SetToCurrentState();
	#endif

	int dispw, disph;
	glfwGetFramebufferSize(window, &dispw, &disph);
	if ((dispw != DispW) || (disph != DispH))
	{
		DispW = dispw;
		DispH = disph;
	}

	//
	// Step 1 - Draw and process the main (top) menu bar and remember its height.
	//
	int topUIHeight = 0;//DoMainMenuBar();

	//
	// Step 2 - Draw and process the bottom nav bar and remember its height.
	//
	int bottomUIHeight = 0;//GetNavBarHeight();
//	DoNavBar(DispW, DispH, bottomUIHeight);

	int workAreaW = DispW;
	int workAreaH = DispH - bottomUIHeight - topUIHeight;
	float workAreaAspect = float(workAreaW)/float(workAreaH);

	glViewport(0, bottomUIHeight, workAreaW, workAreaH);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, workAreaW, 0, workAreaH, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	float draww		= 1.0f;		float drawh		= 1.0f;
	float iw		= 1.0f;		float ih		= 1.0f;
	float left		= 0.0f;
	float right		= 0.0f;
	float top		= 0.0f;
	float bottom	= 0.0f;
	float uoff		= 0.0f;
	float voff		= 0.0f;
	float panX		= 0.0f;
	float panY		= 0.0f;

	double mouseXd, mouseYd;
	glfwGetCursorPos(window, &mouseXd, &mouseYd);

	// Make origin lower-left.
	float workH = float(DispH - bottomUIHeight);
	float mouseX = float(mouseXd);
	float mouseY = workH - float(mouseYd);
	int mouseXi = int(mouseX);
	int mouseYi = int(mouseY);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, tVector2(0.0f, 0.0f));
	ImGui::SetNextWindowPos(tVector2(20.0f, 10.0f));

	static float fps = 0.0f;
	float instFps = dt > tMath::Epsilon ? 1.0f/dt : 0.0f;
	fps = 0.05f*instFps + 0.95f*fps;

	ImGuiWindowFlags flagsImgButton =
		ImGuiWindowFlags_NoTitleBar		|	ImGuiWindowFlags_NoScrollbar	|	ImGuiWindowFlags_NoMove			| ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse		|	ImGuiWindowFlags_NoNav			|	ImGuiWindowFlags_NoBackground	| ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::Begin("FPSTextID", nullptr, flagsImgButton);
	ImGui::Text("FPS:%04.1f", fps);
	ImGui::End();
	ImGui::PopStyleVar();

	// Show the big demo window. You can browse its code to learn more about Dear ImGui.
	static bool showDemoWindow = true;
	//static bool showDemoWindow = true;
	if (showDemoWindow)
		ImGui::ShowDemoWindow(&showDemoWindow);

//	if (!ImGui::GetIO().WantCaptureMouse)
//		DisappearCountdown -= dt;
	tVector2 mousePos(mouseX, mouseY);

	// This calls ImGui::EndFrame for us.
	ImGui::Render();
	glViewport(0, 0, dispw, disph);
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	glfwMakeContextCurrent(window);
	glfwSwapBuffers(window);
//	FrameNumber++;
}


void Visualizer::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int modifiers)
{
	if ((action != GLFW_PRESS) && (action != GLFW_REPEAT))
		return;

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantTextInput)
		return;

	// Don't let key repeats starve the update loop. Ignore repeats if there hasn't
	// been a frame between them.
	/*
	static uint64 lastRepeatFrameNum = 0;
	if (action == GLFW_REPEAT)
	{
		if (lastRepeatFrameNum == FrameNumber)
			return;
		lastRepeatFrameNum = FrameNumber;
	}
	*/

	// Convert key codes to support non-US keyboards. Since the glfwGetKeyName function works on
	// printable characters, it ends up converting the numpad keys KP_* to their printable counterparts.
	// ex. KEY_KP_9 -> KEY_9. This is why we skip the translation for these keys.
	if (!((key >= GLFW_KEY_KP_0) && (key <= GLFW_KEY_KP_EQUAL)))
	{
		const char* keyName = glfwGetKeyName(key, 0);
		if (keyName && *keyName)
			key = tStd::tChrupr(keyName[0]);
	}

	// Process key.

	// Now we need to query the key-binding system to find out what operation is associated
	// with the received key. The current bindings are stored in the current profile.
//	Config::ProfileData& profile = Config::GetProfileData();
//	uint32 viewerModifiers = Bindings::TranslateModifiers(modifiers);
//	Bindings::Operation operation = profile.InputBindings.GetOperation(key, viewerModifiers);
}


void Visualizer::MouseButtonCallback(GLFWwindow* window, int mouseButton, int press, int mods)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	double xposd, yposd;
	glfwGetCursorPos(window, &xposd, &yposd);
	bool down = press ? true : false;
	switch (mouseButton)
	{
		// Left mouse button.
		case 0:
		{
			break;
		}

		// Right mouse button.
		case 1:
		{
			break;
		}
	}
}


void Visualizer::CursorPosCallback(GLFWwindow* window, double x, double y)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;
}


void Visualizer::ScrollWheelCallback(GLFWwindow* window, double x, double y)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;
}


void Visualizer::FileDropCallback(GLFWwindow* window, int count, const char** files)
{
	if (count < 1)
		return;

	tString file = tString(files[0]);
}


void Visualizer::FocusCallback(GLFWwindow* window, int gotFocus)
{
	if (!gotFocus)
		return;

	// If we got focus do stuff.
}


void Visualizer::IconifyCallback(GLFWwindow* window, int iconified)
{
}


void Visualizer::SetWindowIcon(const tString& icoFile)
{
	#ifdef PLATFORM_LINUX
	tImage::tImageICO icon(icoFile);
	if (!icon.IsValid())
		return;

	const int maxImages = 16;
	GLFWimage* imageTable[maxImages];
	GLFWimage images[maxImages];
	int numImages = tMath::tMin(icon.GetNumFrames(), maxImages);
	for (int i = 0; i < numImages; i++)
	{
		imageTable[i] = &images[i];
		tImage::tFrame* frame = icon.GetFrame(i);
		frame->ReverseRows();
		images[i].width = frame->Width;
		images[i].height = frame->Height;
		images[i].pixels = (uint8*)frame->Pixels;
	}

	// This copies the pixel data out so we can let the tImageICO clean itself up afterwards afterwards.
	glfwSetWindowIcon(Visualizer::Window, numImages, *imageTable);
	#endif
}


void Visualizer::SetStyleScaleAndFontSize()
{
	ImGuiIO& io = ImGui::GetIO();

	ImVector<ImWchar> ranges;
	ImFontGlyphRangesBuilder builder;
	builder.AddChar(0x2026);                               // Adds ellipsis.
	builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic()); // Default plus Cyrillic.
	builder.BuildRanges(&ranges);

	// Calling destroy is safe if no texture is currently bound.
	ImGui_ImplOpenGL2_DestroyFontsTexture();
	float fontSizePixels = 22.0f;
	io.Fonts->AddFontFromMemoryCompressedBase85TTF
	(
		RobotoFont_compressed_data_base85,
		fontSizePixels,
		0, ranges.Data
	);

	// This will call build on the font atlas for us.
	ImGui_ImplOpenGL2_CreateFontsTexture();

	// Update the font.
	int fontIndex = 0;
	ImFont* font = io.Fonts->Fonts[fontIndex];
	io.FontDefault = font;

	// Update the style scale.
	float uiSizeScale = 1.5f;
	ImGuiStyle scaledStyle;
	scaledStyle.ScaleAllSizes(uiSizeScale);
	ImGui::GetStyle() = scaledStyle;
}


#ifdef TACENT_UTF16_API_CALLS
int wmain(int argc, wchar_t** argv)
#else
int main(int argc, char** argv)
#endif
{
	#ifdef PLATFORM_WINDOWS
	setlocale(LC_ALL, ".UTF8");
	#endif

	tPrintf("Input Visualizer\n");


	tSystem::tSetSupplementaryDebuggerOutput();
	tSystem::tSetStdoutRedirectCallback(Visualizer::PrintRedirectCallback);

	// These three must get set. They depend on the platform and packaging.
	tString assetsDir;		// Must already exist and be populated with things like the icons that are needed for tacentview.
	tString configDir;		// Directory will be created if needed. Contains the per-user viewer config file.
	tString cacheDir;		// Directory will be created if needed. Contains cache information that is not required (but can) persist between releases.

	// The portable layout is also what should be set while developing -- Everything relative
	// to the program executable with separate sub-directories for Assets, Config, and Cache.
	// This keeps portable/dev out of the way of any installed packages. Windows currently only
	// supports the portable layout.
	tString progDir	= tSystem::tGetProgramDir();
	assetsDir		= progDir + "Assets/";
	configDir		= progDir + "Config/";
	tAssert(assetsDir.IsValid());
	tAssert(configDir.IsValid());
	tPrintf("LocInfo: assetsDir : %s\n", assetsDir.Chr());
	tPrintf("LocInfo; configDir : %s\n", configDir.Chr());

	// assetDir is supposed to already exist and, well, have the assets in it.
	// configDir needs to be created if it don't already exist.
	bool assetsDirExists = tSystem::tDirExists(assetsDir);
	bool configDirExists = tSystem::tDirExists(configDir);
	if (!configDirExists)
		configDirExists = tSystem::tCreateDirs(configDir);

	// Setup window
	glfwSetErrorCallback(Visualizer::GlfwErrorCallback);
	if (!glfwInit())
		return Visualizer::ErrorCode_GUI_FailGLFWInit;

	int glfwMajor = 0; int glfwMinor = 0; int glfwRev = 0;
	glfwGetVersion(&glfwMajor, &glfwMinor, &glfwRev);
	tPrintf("Exe %s\n", tSystem::tGetProgramPath().Chr());
//	tPrintf("Tacent View V %d.%d.%d\n", ViewerVersion::Major, ViewerVersion::Minor, ViewerVersion::Revision);
	tPrintf("Tacent Library V %d.%d.%d\n", tVersion::Major, tVersion::Minor, tVersion::Revision);
	tPrintf("Dear ImGui V %s\n", IMGUI_VERSION);
	tPrintf("GLFW V %d.%d.%d\n", glfwMajor, glfwMinor, glfwRev);

	// We start with window invisible. For windows DwmSetWindowAttribute won't redraw properly otherwise.
	// For all plats, we want to position the window before displaying it.
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	#if defined(PLATFORM_LINUX)
	glfwWindowHintString(GLFW_X11_CLASS_NAME, "visualizer");
	#endif

	int windowHintFramebufferBitsPerComponent = 8;
	if (windowHintFramebufferBitsPerComponent != 0)
	{
		glfwWindowHint(GLFW_RED_BITS, windowHintFramebufferBitsPerComponent);
		glfwWindowHint(GLFW_GREEN_BITS, windowHintFramebufferBitsPerComponent);
		glfwWindowHint(GLFW_BLUE_BITS, windowHintFramebufferBitsPerComponent);
	}

	// The title here seems to override the Linux hint above. When we create with the title string "visualizer",
	// glfw makes it the X11 WM_CLASS. This is needed so that the Ubuntu can map the same name in the .desktop file
	// to find things like the correct dock icon to display. The SetWindowTitle afterwards does not mod the WM_CLASS.
	Visualizer::Window = glfwCreateWindow(1024, 576, "visualizer", nullptr, nullptr);
	if (!Visualizer::Window)
	{
		glfwTerminate();
		return Visualizer::ErrorCode_GUI_FailGLFWWindow;
	}

	if (assetsDirExists)
		Visualizer::SetWindowIcon(assetsDir + "Visualizer.ico");
	glfwSetWindowTitle(Visualizer::Window, "Visualizer");
	glfwSetWindowPos(Visualizer::Window, 100, 80);

	#ifdef PLATFORM_WINDOWS
	// Make the window title bar show up in black.
	HWND hwnd = glfwGetWin32Window(Visualizer::Window);
	const int DWMWA_USE_IMMERSIVE_DARK_MODE_A = 19;
	const int DWMWA_USE_IMMERSIVE_DARK_MODE_B = 20;
	BOOL isDarkMode = 1;
	DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_A, &isDarkMode, sizeof(isDarkMode));
	DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_B, &isDarkMode, sizeof(isDarkMode));
	#endif

//	tSystem::tSleep(10000);
//	glfwTerminate();
//	return 0;

	glfwMakeContextCurrent(Visualizer::Window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		tPrintf("Failed to initialize GLAD\n");
		glfwDestroyWindow(Visualizer::Window);
		glfwTerminate();
		return Visualizer::ErrorCode_GUI_FailGLADInit;
	}
	tPrintf("GLAD V %s\n", glGetString(GL_VERSION));

	glfwSwapInterval(1); // Enable vsync

	// Needed when window system knows a refresh is needed.
	glfwSetWindowRefreshCallback(Visualizer::Window, Visualizer::WindowRefreshFun);
	glfwSetKeyCallback(Visualizer::Window, Visualizer::KeyCallback);
	glfwSetMouseButtonCallback(Visualizer::Window, Visualizer::MouseButtonCallback);
	glfwSetCursorPosCallback(Visualizer::Window, Visualizer::CursorPosCallback);
	glfwSetScrollCallback(Visualizer::Window, Visualizer::ScrollWheelCallback);
	glfwSetDropCallback(Visualizer::Window, Visualizer::FileDropCallback);
	glfwSetWindowFocusCallback(Visualizer::Window, Visualizer::FocusCallback);
	glfwSetWindowIconifyCallback(Visualizer::Window, Visualizer::IconifyCallback);

	// Setup Dear ImGui context.
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags = 0;
	// io.NavActive = false;
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	// Setup Dear ImGui style.
	ImGui::StyleColorsDark();

	// Setup platform/renderer bindings.
	ImGui_ImplGlfw_InitForOpenGL(Visualizer::Window, true);
	ImGui_ImplOpenGL2_Init();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	#if 0

	// Before we load the single font we currently need in the update loop, we need to know what the desired UI size is.
	Viewer::UpdateDesiredUISize();

	Viewer::LoadAppImages(assetsDir);
	Viewer::PopulateImages();

	// SetCurrentImage deals with ImageToLoad being empty.
	Viewer::SetCurrentImage(Viewer::ImageToLoad);

	#endif

	glClearColor(Visualizer::ColourClear.x, Visualizer::ColourClear.y, Visualizer::ColourClear.z, Visualizer::ColourClear.w);
	glClear(GL_COLOR_BUFFER_BIT);
	int dispw, disph;
	glfwGetFramebufferSize(Visualizer::Window, &dispw, &disph);
	glViewport(0, 0, dispw, disph);

	Visualizer::SetStyleScaleAndFontSize();

	// Show the window. Can this just be the glfw call for all platforms?
	#ifdef PLATFORM_WINDOWS
	ShowWindow(hwnd, SW_SHOW);
	#elif defined(PLATFORM_LINUX)
	glfwShowWindow(Visualizer::Window);
	#endif

	// I don't seem to be able to get Linux to v-sync.
	// glfwSwapInterval(1);
	glfwMakeContextCurrent(Visualizer::Window);
	glfwSwapBuffers(Visualizer::Window);

//	Viewer::Config::ProfileData& profile = *Viewer::Config::Current;
//	if (profile.FullscreenMode)
//		Viewer::ChangeScreenMode(true, true);

	int redBits		= 0;	glGetIntegerv(GL_RED_BITS,	&redBits);
	int greenBits	= 0;	glGetIntegerv(GL_GREEN_BITS,&greenBits);
	int blueBits	= 0;	glGetIntegerv(GL_BLUE_BITS,	&blueBits);
	int alphaBits	= 0;	glGetIntegerv(GL_ALPHA_BITS,&alphaBits);
	tPrintf("Framebuffer BPC (RGB): (%d,%d,%d)\n", redBits, blueBits, greenBits);

	// Main loop.
	static double lastUpdateTime = glfwGetTime();
	while (!glfwWindowShouldClose(Visualizer::Window))// && !Visualizer::Request_Quit)
	{
		double currUpdateTime = glfwGetTime();
		double elapsed = tMath::tMin(currUpdateTime - lastUpdateTime, 1.0/30.0);

		Visualizer::Update(Visualizer::Window, elapsed);

		int sleepms = 0;

		// I don't seem to be able to get Linux to v-sync. This stops it using all the CPU.
		#ifdef PLATFORM_LINUX
		sleepms = 16;
		#endif

		if (Visualizer::WindowIconified)
			sleepms = 100;
		if (sleepms)
			tSystem::tSleep(sleepms);

		lastUpdateTime = currUpdateTime;
	}

	//Viewer::UnloadAppImages();

	// Cleanup.
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(Visualizer::Window);
	glfwTerminate();

	return Visualizer::ErrorCode_Success;

//	#endif // if 0
}
