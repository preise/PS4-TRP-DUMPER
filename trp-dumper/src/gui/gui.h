#pragma once

struct Notification
{
	std::string message;
	std::chrono::steady_clock::time_point startTime;
	std::chrono::duration<float> duration;
	float fadeDuration = 1.0f;
	bool play_sound = false;

	Notification(const std::string& msg, float dur, bool play = false)
		: message(msg), startTime(std::chrono::steady_clock::now()), duration(std::chrono::seconds(static_cast<long long>(dur))), play_sound(play)
	{
	}
};

class NotificationManager
{
private:
	std::vector<Notification> notifications;

public:
	void add(float duration, bool play_sound, const char* format, ...);

	void render(int windowWidth, int windowHeight);
};

namespace gui
{
	inline NotificationManager* notification = nullptr;

	inline IXAudio2* pXAudio2 = nullptr;
	inline IXAudio2SourceVoice* pSourceVoice = nullptr;
	inline IXAudio2MasteringVoice* pMasterVoice = nullptr;
	inline std::vector<short> pcm;
	inline WAVEFORMATEX waveFormat = { 0 };
	bool decode_mp3(const unsigned char* mp3Data, size_t dataSize, std::vector<short>& output, WAVEFORMATEX& waveFormat);
	void init_audio();
	void play_sound(const std::vector<short>& pcm, WAVEFORMATEX& waveFormat);
	void uninit_audio();

	inline std::string windowNameVar = "PR-Window";
	inline std::string windowClassVar = "PR-Class";

	const ImVec4 transparent = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	inline ImVec2 window_size = ImVec2(1200, 650); // ImVec2(700, 500);

	inline ImFont* font_24 = nullptr;
	inline ImFont* font_18 = nullptr;

	inline bool isRunning = true;

	inline HWND window = nullptr;
	inline WNDCLASSEX windowClass = { };

	inline POINTS position = { };

	inline PDIRECT3DTEXTURE9 icon;
	inline int icon_width, icon_height = 100;

	inline PDIRECT3D9 d3d = nullptr;
	inline LPDIRECT3DDEVICE9 device = nullptr;
	inline D3DPRESENT_PARAMETERS presentParameters = { };

	inline IDirect3DTexture9* cross_image = nullptr;
	inline IDirect3DTexture9* folder_image = nullptr;
	inline IDirect3DTexture9* folder_image2 = nullptr;

	IDirect3DTexture9* D3D9LoadImage(IDirect3DDevice9* device, const unsigned char* imageData, size_t imageSize);

	void CreateHWindow(const char* windowName) noexcept;
	void DestroyHWindow() noexcept;

	bool CreateDevice() noexcept;
	void ResetDevice() noexcept;
	void DestroyDevice() noexcept;

	void CreateImGui() noexcept;
	void DestroyImGui() noexcept;

	void BeginRender() noexcept;
	void EndRender() noexcept;
	void Render() noexcept;
}