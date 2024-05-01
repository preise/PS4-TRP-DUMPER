#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincrypt.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <array>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <string>
#include <filesystem>
#include <thread>
#include <mutex>
#include <regex>


#include <d3d9.h>
#include <d3dx9tex.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_stdlib.h"

#include <XAudio2.h>
#include <mpg123.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mpg123.lib")
#pragma comment(lib, "Shlwapi.lib")

#include <ShObjIdl.h>