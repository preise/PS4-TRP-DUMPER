#include "pch.h"
#include "gui.h"
#include "images.h"
#include "core/core.h"

void NotificationManager::add(float duration, bool play_sound, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	int size = std::vsnprintf(nullptr, 0, format, args);
	va_end(args);

	if (size <= 0) return;

	std::vector<char> buffer(size + 1);
	va_start(args, format);
	std::vsnprintf(buffer.data(), buffer.size(), format, args);
	va_end(args);

	notifications.emplace_back(std::string(buffer.data()), duration, play_sound);
}
void NotificationManager::render(int windowWidth, int windowHeight)
{
	float yOffset = 50.0f;

	for (auto it = notifications.begin(); it != notifications.end(); )
	{
		auto& notif = *it;
		auto elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - notif.startTime).count();
		if (elapsed > notif.duration.count())
		{
			it = notifications.erase(it);
			continue;
		}
		else
		{
			++it;
		}

		if (notif.play_sound)
		{
			gui::play_sound(gui::pcm, gui::waveFormat);
			notif.play_sound = false;
		}

		float alpha = 1.0f;
		if (elapsed < notif.fadeDuration)
			alpha = elapsed / notif.fadeDuration;
		else if (notif.duration.count() - elapsed < notif.fadeDuration)
			alpha = (notif.duration.count() - elapsed) / notif.fadeDuration;

		ImVec4 messageColor = ImVec4(1.0f, 1.0f, 1.0f, alpha);
		ImVec4 backgroundColor = ImVec4(0.1f, 0.1f, 0.1f, 0.9f * alpha);

		float textWidth = ImGui::CalcTextSize(notif.message.c_str()).x;
		float textHeight = ImGui::CalcTextSize(notif.message.c_str()).y;

		ImVec2 textPosition = ImVec2(20.0f, windowHeight - yOffset);
		ImVec2 rectMin = ImVec2(textPosition.x - 5, textPosition.y - 5);
		ImVec2 rectMax = ImVec2(textPosition.x + textWidth + 5, textPosition.y + textHeight + 5);

		ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, ImGui::ColorConvertFloat4ToU32(backgroundColor), 10.0f);

		ImGui::PushStyleColor(ImGuiCol_Text, messageColor);
		ImGui::SetCursorPos(textPosition);
		ImGui::Text("%s", notif.message.c_str());
		ImGui::PopStyleColor();

		yOffset += textHeight + 25.0f;
	}
}

const WCHAR* StringToWChar(const std::string& s)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &s[0], (int)s.size(), NULL, 0);
	std::vector<WCHAR> wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &s[0], (int)s.size(), &wstrTo[0], size_needed);
	return &wstrTo[0];
}
const std::string WCharToString(const WCHAR* s)
{
	int bufferSize = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
	std::string narrow(bufferSize, 0);
	WideCharToMultiByte(CP_UTF8, 0, s, -1, &narrow[0], bufferSize, nullptr, nullptr);
	narrow.pop_back();
	return narrow;
}

IDirect3DTexture9* gui::D3D9LoadImage(IDirect3DDevice9* device, const unsigned char* imageData, size_t imageSize) {
	IDirect3DTexture9* texture = nullptr;
	HRESULT hr = D3DXCreateTextureFromFileInMemory(
		device,
		imageData,
		imageSize,
		&texture
	);

	if (FAILED(hr)) {
		// Handle error
		return nullptr;
	}

	return texture;
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter);
LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU)
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;

	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter);
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::window_size.x &&
				gui::position.y >= 0 && gui::position.y <= 30)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}

	}return 0;
	}

	return DefWindowProc(window, message, wideParameter, longParameter);
}
void gui::CreateHWindow(const char* windowName) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = StringToWChar(windowClassVar);
	windowClass.hIconSm = 0;

	RegisterClassEx(&windowClass);

	window = CreateWindowEx(
		WS_EX_LAYERED, // used to be WS_EX_LAYERED | WS_EX_TOPMOST
		StringToWChar(windowClassVar),
		StringToWChar(windowName),
		WS_POPUP,
		100,
		100,
		window_size.x,
		window_size.y,
		0,
		0,
		windowClass.hInstance,
		0
	);

	HRGN hRgn = CreateRoundRectRgn(0, 0, window_size.x, window_size.y, 15, 15);

	SetWindowRgn(window, hRgn, TRUE);

	DeleteObject(hRgn);

	SetLayeredWindowAttributes(window, 0, 255, LWA_ALPHA); // 70% opacity: SetLayeredWindowAttributes(window, 0, (255 * 90) / 100, LWA_ALPHA);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}
void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}
bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}
void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}
void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}
void gui::DestroyImGui() noexcept
{
	uninit_audio();
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.message == WM_QUIT)
		{
			isRunning = !isRunning;
			return;
		}
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}
void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, (0, 0, 0, 0), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	// Handle loss of D3D9 device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

bool gui::decode_mp3(const unsigned char* mp3Data, size_t dataSize, std::vector<short>& output, WAVEFORMATEX& waveFormat) {
	mpg123_init();
	int err;
	mpg123_handle* mh = mpg123_new(nullptr, &err);
	mpg123_open_feed(mh);

	long rate;
	int channels, encoding;
	std::vector<unsigned char> pcm;

	size_t done;
	mpg123_feed(mh, mp3Data, dataSize);
	unsigned char buffer[4096];
	int status;

	while ((status = mpg123_read(mh, buffer, sizeof(buffer), &done)) == MPG123_OK || status == MPG123_NEW_FORMAT) {
		if (status == MPG123_NEW_FORMAT) {
			mpg123_getformat(mh, &rate, &channels, &encoding);
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = channels;
			waveFormat.nSamplesPerSec = rate;
			waveFormat.wBitsPerSample = 16; // Assuming 16-bit samples
			waveFormat.nBlockAlign = channels * 2;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
			waveFormat.cbSize = 0;
		}
		pcm.insert(pcm.end(), buffer, buffer + done);
	}

	output.resize(pcm.size() / 2);
	memcpy(output.data(), pcm.data(), pcm.size());

	mpg123_close(mh);
	mpg123_delete(mh);
	mpg123_exit();

	return true;
}
void gui::init_audio() {
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	pXAudio2->CreateMasteringVoice(&pMasterVoice);
}
void gui::play_sound(const std::vector<short>& pcm, WAVEFORMATEX& waveFormat) {
	pXAudio2->CreateSourceVoice(&pSourceVoice, &waveFormat);
	XAUDIO2_BUFFER buffer = { 0 };
	buffer.AudioBytes = static_cast<UINT32>(pcm.size() * sizeof(short));
	buffer.pAudioData = reinterpret_cast<const BYTE*>(pcm.data());
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	pSourceVoice->SubmitSourceBuffer(&buffer);
	pSourceVoice->Start(0);
}
void gui::uninit_audio() {
	if (pSourceVoice) pSourceVoice->DestroyVoice();
	if (pMasterVoice) pMasterVoice->DestroyVoice();
	if (pXAudio2) pXAudio2->Release();
	CoUninitialize();
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	ImFontConfig fontconf;
	fontconf.FontDataOwnedByAtlas = false;
	font_24 = io.Fonts->AddFontFromMemoryTTF((void*)font_data, sizeof(font_data), 24, &fontconf);
	font_18 = io.Fonts->AddFontFromMemoryTTF((void*)font_data, sizeof(font_data), 18, &fontconf);
	if (font_24)
		io.FontDefault = font_18;

	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
	style.WindowRounding = 10.0f; // Set window corner rounding to 10 pixels
	style.ChildRounding = 10.0f; // Set child window corner rounding to 10 pixels
	style.FrameRounding = 10.0f; // Set frame corner rounding to 10 pixels
	style.PopupRounding = 10.0f; // Set popup corner rounding to 10 pixels
	style.GrabRounding = 2.0f; // Set grab corner rounding to 2 pixels
	style.TabRounding = 0.0; // Set tab corner rounding to 0 pixels
	style.WindowPadding = ImVec2(0.0f, 0.0f); // Add padding around the window
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f); // Center window title text
	style.WindowBorderSize = 0.0f; // Set Window Border size to 0
	style.ChildBorderSize = 0.0f; // Set Child Border size to 0
	style.FrameBorderSize = 0.0f; // Set Frame Border size to 0
	style.PopupBorderSize = 0.0f; // Set Popup Border size to 0
	style.ScrollbarRounding = 10.0f; // Set Scrollbar corner rounding to 10 pixels
	style.ScrollbarSize = 10.0f; // Set Scrollbar size to 10 pixels
	style.FramePadding = ImVec2(6.0f, 6.0f); // Add padding around the frame
	//style.FrameBorderSize = 1.0f; // Set Frame Border size to 1

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.94f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.10f, 0.10f, 0.10f, 0.94f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.10f, 0.10f, 0.10f, 0.94f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.00f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.00f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.00f, 1.00f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.00f, 0.00f, 1.00f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.00f, 0.64f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.00f, 0.64f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 1.00f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.64f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.00f, 0.64f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.00f, 0.00f, 1.00f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.00f, 0.00f, 0.64f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.00f, 0.00f, 0.64f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 1.00f, 1.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 0.00f, 0.64f, 1.00f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 0.00f, 0.64f, 1.00f);
	colors[ImGuiCol_Tab] = transparent;
	colors[ImGuiCol_TabHovered] = ImVec4(0.00f, 0.00f, 0.64f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.00f, 0.00f, 0.64f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	notification = new NotificationManager();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);

	cross_image = D3D9LoadImage(device, cross_png, sizeof(cross_png));
	folder_image = D3D9LoadImage(device, folder_png, sizeof(folder_png));
	folder_image2 = D3D9LoadImage(device, folder_png, sizeof(folder_png));
	if (cross_image == nullptr || folder_image == nullptr)
		isRunning = false;

	if (decode_mp3(sound_data, sizeof(sound_data), pcm, waveFormat))
		init_audio();
}

bool validate_npid(const std::string& npid)
{
	std::regex pattern("^[A-Za-z]{4}[0-9]{5}_00$");

	return std::regex_match(npid, pattern);
}
void select_folder(std::string* path, std::string* buf)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (SUCCEEDED(hr))
	{

		IFileDialog* pfd = NULL;
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileDialog, reinterpret_cast<void**>(&pfd));

		if (SUCCEEDED(hr))
		{

			DWORD dwOptions;
			hr = pfd->GetOptions(&dwOptions);
			if (SUCCEEDED(hr))
			{
				pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);

				if (path && !path->empty())
				{
					IShellItem* psiInitial = NULL;
					std::wstring wPath(path->begin(), path->end());
					hr = SHCreateItemFromParsingName(wPath.c_str(), NULL, IID_PPV_ARGS(&psiInitial));
					if (SUCCEEDED(hr))
					{
						pfd->SetFolder(psiInitial);
						psiInitial->Release();
					}
				}

				hr = pfd->Show(NULL);
				if (SUCCEEDED(hr))
				{

					IShellItem* psiResult;
					hr = pfd->GetResult(&psiResult);
					if (SUCCEEDED(hr))
					{
						PWSTR pszFilePath = NULL;
						hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

						if (SUCCEEDED(hr))
						{
							*path = WCharToString(pszFilePath);
							CoTaskMemFree(pszFilePath);

							if (buf)
							{
								if (!buf->empty() || !path)
									return;
								std::filesystem::path fsPath(*path);
								std::string lastFolder = fsPath.filename().string();

								if (!validate_npid(lastFolder))
								{
									gui::notification->add(5.0f, true, "Invalid NPID: %s", lastFolder.c_str());
									return;
								}

								gui::notification->add(5.0f, true, "Selected NPID: %s", lastFolder.c_str());

								*buf = lastFolder;
							}
						}
						psiResult->Release();
					}
				}
			}
			pfd->Release();
		}
		CoUninitialize();
	}
}

static int InputTextCallback(ImGuiInputTextCallbackData* data)
{
	if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter)
	{
		std::string* str = (std::string*)data->UserData;
		if (str->size() >= 255)
		{
			return 1;
		}
	}
	return 0;
}

void txt_box(ImVec2 size, const char* label, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	int fsize = std::vsnprintf(nullptr, 0, format, args);
	va_end(args);

	if (fsize <= 0) return;

	std::vector<char> buffer(fsize + 1);
	va_start(args, format);
	std::vsnprintf(buffer.data(), buffer.size(), format, args);
	va_end(args);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.10f, 0.94f));
	ImGui::BeginChild(label, size, true, ImGuiWindowFlags_NoScrollbar);

	ImVec2 textSize = ImGui::CalcTextSize(buffer.data());
	float posX = 5.f;
	float posY = (size.y - textSize.y) * 0.5f;

	ImGui::SetCursorPos(ImVec2(posX, posY));
	ImGui::Text("%s", buffer.data());

	ImGui::EndChild();
	ImGui::PopStyleColor();
}

static std::string folder_path = "";
static std::string dump_path = "";
static std::string npid = "";

static bool decrypt_files = true;
static bool dump_in_same_folder = false;
static bool dump_images = true;
static bool dump_esfm = true;

void txtbut_box(int index)
{
	if (core->entries_data.size() <= index || core->entries_data.empty())
		return;

	std::string label = "##" + std::to_string(index);

	ImVec2 size = ImVec2(530, 30);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.10f, 0.94f));
	ImGui::BeginChild(label.c_str(), size, true, ImGuiWindowFlags_NoScrollbar);

	ImVec2 textSize = ImGui::CalcTextSize((const char*)core->entries_data[index].entry_name);
	float posX = 5.f;
	float posY = (size.y - textSize.y) * 0.5f;

	ImGui::SetCursorPos(ImVec2(posX, posY));
	ImGui::Text("%s", (const char*)core->entries_data[index].entry_name);

	static ImVec2 text_size_endec = ImGui::CalcTextSize("ENCRYPTED");

	ImGui::SetCursorPos(ImVec2(530 / 2 - text_size_endec.x / 2, posY));
	ImGui::Text("%s", ((const char*)core->entries_data[index].flag ? "ENCRYPTED" : "DECRYPTED"));

	// Calculate button position to the right with padding of 10 pixels
	float buttonPosX = size.x - ImGui::CalcTextSize("DUMP").x - 30.f;
	float buttonPosY = 30.f / 2.f - 25.f / 2.f;
	ImGui::SetCursorPos(ImVec2(buttonPosX, buttonPosY));

	if (ImGui::Button("DUMP", ImVec2(60.f, 25.f)))
	{
		if (core->file_data.empty())
		{
			gui::notification->add(5.f, true, "TROPHY.TRP NOT LOADED");
		}
		else
		{
			if (npid.empty())
			{
				gui::notification->add(5.f, true, "NPID NOT SELECTED");
			}
			else
			{
				if (!validate_npid(npid))
				{
					gui::notification->add(5.f, true, "INVALID NPID");
				}
				else
				{
					core->NPID = npid;

					if (dump_in_same_folder)
						core->dump_path = std::filesystem::current_path().string() + "\\DUMP-" + npid;
					else
						if (dump_path.empty())
							gui::notification->add(5.f, true, "DUMP PATH NOT SELECTED");
						else
							core->dump_path = dump_path + "\\DUMP-" + npid;
						
					if (!core->dump_path.empty())
					{
						if (decrypt_files)
							core->decrypt_data(core->entries_data[index].data, core->NPID);

						core->dump_file(index);

						gui::notification->add(5.f, true, "SUCCESSFULY DUMPED: %s", (const char*)core->entries_data[index].entry_name);
					}
				}
			}
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	ImGui::EndChild();
	ImGui::PopStyleColor();
}

void title()
{
	ImGui::PushFont(gui::font_24);
	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec4 bgColor = ImVec4(0.1f, 0.1f, 0.1f, 0.9f);

	ImGui::GetWindowDrawList()->AddRectFilled(windowPos, ImVec2(windowPos.x + ImGui::GetWindowWidth(), windowPos.y + 30), ImGui::ColorConvertFloat4ToU32(bgColor));

	ImVec2 test_size = ImGui::CalcTextSize("TROPHY DUMPER");
	ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() / 2 - test_size.x / 2, 15 - test_size.y / 2));
	ImGui::Text("TROPHY DUMPER");

	test_size = ImGui::CalcTextSize("GITHUB.COM/PREISE");
	ImGui::SetCursorPos(ImVec2(10 , -2));
	ImGui::PushStyleColor(ImGuiCol_Button, gui::transparent);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, gui::transparent);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, gui::transparent);
	if (ImGui::Button("GITHUB.COM/PREISE"))
	{
		ShellExecute(0, 0, L"https://www.github.com/preise", 0, 0, SW_SHOW);
	}

	if (ImGui::IsItemHovered()) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 35, 0.f));
	bool cross = ImGui::ImageButton((void*)gui::cross_image, ImVec2(20, 20));

	if (ImGui::IsItemHovered()) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	ImGui::PopStyleColor(3);

	if (cross)
		gui::isRunning = false;

	ImGui::PopFont();
}
void dump_menu()
{

	static float trp_loader_pos_x = 10 + ((530 / 2) - (ImGui::CalcTextSize("TRP LOADER").x / 2));

	ImGui::SetCursorPos(ImVec2(trp_loader_pos_x, 82));
	ImGui::Text("TRP LOADER");

	ImGui::GetWindowDrawList()->AddLine(ImVec2(10, 100), ImVec2(10 + 540, 100), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));

	ImGui::SetCursorPos(ImVec2(10, 106));
	ImGui::SetNextItemWidth(500);
	if (ImGui::InputTextWithHint("##SELECT_FOLDER", "SELECT FOLDER", &folder_path, ImGuiInputTextFlags_CallbackCharFilter, InputTextCallback, &folder_path))
	{

	}

	ImGui::PushStyleColor(ImGuiCol_Button, gui::transparent);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, gui::transparent);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, gui::transparent);
	ImGui::SetCursorPos(ImVec2(510, 100));
	if (ImGui::ImageButton((void*)gui::folder_image, ImVec2(30, 30)))
	{
		std::thread folderSelector(select_folder, &folder_path, &npid);
		folderSelector.detach();
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	ImGui::SetCursorPos(ImVec2(10, 146));
	ImGui::SetNextItemWidth(500);
	if (ImGui::InputTextWithHint("##DUMP_PATH", "DUMP PATH", &dump_path, ImGuiInputTextFlags_CallbackCharFilter, InputTextCallback, &dump_path))
	{

	}

	ImGui::SetCursorPos(ImVec2(510, 140));
	if (ImGui::ImageButton((void*)gui::folder_image2, ImVec2(30, 30)))
	{
		std::thread folderSelector(select_folder, &dump_path, nullptr);
		folderSelector.detach();
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	ImGui::PopStyleColor(3);

	ImGui::SetCursorPos(ImVec2(10, 186));
	ImGui::SetNextItemWidth(500);
	if (ImGui::InputTextWithHint("##NPID", "NPID", &npid, ImGuiInputTextFlags_CallbackCharFilter, InputTextCallback, &npid))
	{

	}

	ImGui::SetCursorPos(ImVec2(10, 226));
	bool load_trp = ImGui::Button("LOAD TROPHY.TRP", ImVec2(540, 30));
	if (load_trp)
	{
		std::string file_path_var = folder_path + "\\" + "TROPHY.TRP";
		core->clear_data();
		if (core && core->load_trp(file_path_var))
		{
			if (!core->load_header())
			{
				gui::notification->add(5.0f, true, "INVALID TROPHY.TRP");
			}
			else
			{
				core->load_entries();
				core->load_entries_data();
				gui::notification->add(5.0f, true, "TROPHY.TRP LOADED SUCCESSFUL");
			}
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}
	
	// ----------------------------
	static float trp_info_pos_x = 10 + ((530 / 2) - (ImGui::CalcTextSize("TRP INFO").x / 2));

	ImGui::SetCursorPos(ImVec2(trp_info_pos_x, 282));
	ImGui::Text("TRP INFO");

	ImGui::GetWindowDrawList()->AddLine(ImVec2(10, 300), ImVec2(10 + 540, 300), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));

	ImGui::SetCursorPos(ImVec2(10, 306));
	txt_box(ImVec2(265, 30), "magic", "TRP MAGIC: 0x%s", (std::stringstream{} << std::hex << std::uppercase << core->header.magic).str().c_str());

	ImGui::SetCursorPos(ImVec2(285, 306));
	txt_box(ImVec2(265, 30), "entries", "TRP VERSION: %s", std::to_string(core->header.version).c_str());

	ImGui::SetCursorPos(ImVec2(10, 346));
	txt_box(ImVec2(265, 30), "entry_num", "TRP ENTRIE NUM: %s", std::to_string(core->header.entry_num).c_str());

	ImGui::SetCursorPos(ImVec2(285, 346));
	txt_box(ImVec2(265, 30), "entry_size", "TRP ENTRIE SIZE: %s", std::to_string(core->header.entry_size).c_str());

	ImGui::SetCursorPos(ImVec2(10, 386));
	txt_box(ImVec2(265, 30), "dev_flag", "TRP DEV FLAG: %s", std::to_string(core->header.dev_flag).c_str());

	ImGui::SetCursorPos(ImVec2(285, 386));
	txt_box(ImVec2(265, 30), "key_index", "TRP KEY INDEX: 0x%s", (std::stringstream{} << std::hex << std::uppercase << core->header.key_index).str().c_str());

	// ----------------------------

	static float dump_options_pos_x = 10 + ((530 / 2) - (ImGui::CalcTextSize("DUMP OPTIONS").x / 2));

	ImGui::SetCursorPos(ImVec2(dump_options_pos_x, 442));
	ImGui::Text("DUMP OPTIONS");

	ImGui::GetWindowDrawList()->AddLine(ImVec2(10, 460), ImVec2(10 + 540, 460), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));

	ImGui::SetCursorPos(ImVec2(10, 466));
	ImGui::Checkbox("##decrypt", &decrypt_files);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		static ImVec2 text_size = ImGui::CalcTextSize("IF CHECKED, DECRYPT ENCRYPTED FILES");
		ImGui::SetNextWindowSize(ImVec2(text_size.x + 10, text_size.y + 5));
		ImGui::BeginTooltip();
		ImGui::SetCursorPos(ImVec2(5, 2.5));
		ImGui::Text("IF CHECKED, DECRYPT ENCRYPTED FILES");
		ImGui::EndTooltip();
		ImGui::PopStyleColor(1);
	}
	ImGui::SetCursorPos(ImVec2(45, 466));
	txt_box(ImVec2(230, 30), "decrypt", "DECRYPT ENCRYPTED FILES");

	ImGui::SetCursorPos(ImVec2(285, 466));
	ImGui::Checkbox("##dump_path", &dump_in_same_folder);
	if (ImGui::IsItemHovered())
	{

		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		static ImVec2 text_size = ImGui::CalcTextSize("IF CHECKED, DUMP FILES IN FOLDER WHERE DUMPER IS IN");
		ImGui::SetNextWindowSize(ImVec2(text_size.x + 10, text_size.y + 5));
		ImGui::BeginTooltip();
		ImGui::SetCursorPos(ImVec2(5, 2.5));
		ImGui::Text("IF CHECKED, DUMP FILES IN FOLDER WHERE DUMPER IS IN");
		ImGui::EndTooltip();
		ImGui::PopStyleColor(1);
	}
	ImGui::SetCursorPos(ImVec2(320, 466));
	txt_box(ImVec2(230, 30), "dump_path", "DUMP IN SAME FOLDER");
	// ----------------------------
	ImGui::SetCursorPos(ImVec2(10, 506));
	ImGui::Checkbox("##dump_images", &dump_images);
	if (ImGui::IsItemHovered())
	{

		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		static ImVec2 text_size = ImGui::CalcTextSize("IF CHECKED, DUMP IMAGES");
		ImGui::SetNextWindowSize(ImVec2(text_size.x + 10, text_size.y + 5));
		ImGui::BeginTooltip();
		ImGui::SetCursorPos(ImVec2(5, 2.5));
		ImGui::Text("IF CHECKED, DUMP IMAGES");
		ImGui::EndTooltip();
		ImGui::PopStyleColor(1);
	}
	ImGui::SetCursorPos(ImVec2(45, 506));
	txt_box(ImVec2(230, 30), "dump_images", "DUMP IMAGES");

	ImGui::SetCursorPos(ImVec2(285, 506));
	ImGui::Checkbox("##dump_esfm", &dump_esfm);
	if (ImGui::IsItemHovered())
	{

		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		static ImVec2 text_size = ImGui::CalcTextSize("IF CHECKED, DUMP ESFM");
		ImGui::SetNextWindowSize(ImVec2(text_size.x + 10, text_size.y + 5));
		ImGui::BeginTooltip();
		ImGui::SetCursorPos(ImVec2(5, 2.5));
		ImGui::Text("IF CHECKED, DUMP ESFM");
		ImGui::EndTooltip();
		ImGui::PopStyleColor(1);
	}
	ImGui::SetCursorPos(ImVec2(320, 506));
	txt_box(ImVec2(230, 30), "dump_esfm", "DUMP ESFM");

	ImGui::SetCursorPos(ImVec2(10, 546));
	ImGui::SetNextItemWidth(540);
	bool dump = ImGui::Button("DUMP FILES", ImVec2(540, 30));
	if (dump)
	{
		if (core->file_data.empty())
		{
			gui::notification->add(5.f, true, "TROPHY.TRP NOT LOADED");
		}
		else
		{
			if (npid.empty())
			{
				gui::notification->add(5.f, true, "NPID NOT SELECTED");
			}
			else
			{
				if (!validate_npid(npid))
				{
					gui::notification->add(5.f, true, "INVALID NPID");
					return;
				}

				core->NPID = npid;

				if (dump_in_same_folder)
					core->dump_path = std::filesystem::current_path().string() + "\\DUMP-" + npid;
				else
					if (dump_path.empty())
						gui::notification->add(5.f, true, "DUMP PATH NOT SELECTED");
					else
						core->dump_path = dump_path + "\\DUMP-" + npid;

				if (!core->dump_path.empty())
				{
					if (decrypt_files)
						core->decrypt_entries();

					core->dump_files(dump_images, dump_esfm);

					core->clear_data();
					gui::notification->add(5.f, true, "SUCCESSFULY DUMPED TROPHY AND CLEARED VARIABLES");
				}
			}
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

	float x_pos = gui::window_size.x - 10 - 540;
	static float trp_explorer_pos_x = x_pos + ((530 / 2) - (ImGui::CalcTextSize("TRP EXPLORER").x / 2));

	ImGui::SetCursorPos(ImVec2(trp_explorer_pos_x, 82));
	ImGui::Text("TRP EXPLORER");

	ImGui::GetWindowDrawList()->AddLine(ImVec2(x_pos, 100), ImVec2(x_pos + 540, 100), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));


	ImGui::SetCursorPos(ImVec2(x_pos, 106));
	ImGui::BeginChild("ScrollingRegion", ImVec2(540, 470), true);
	for (int i = 0; i < core->entries_data.size(); i++)
	{
		float y_pos = (40 * i);
		ImGui::SetCursorPos(ImVec2(5.f, y_pos));
		txtbut_box(i);
	}
	ImGui::EndChild();
}

void gui::Render() noexcept
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize(window_size);
	ImGui::Begin(
		windowNameVar.c_str(),
		&isRunning,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoBackground
	);
	title();

	dump_menu();

	notification->render(window_size.x, window_size.y);

	ImGui::End();
}