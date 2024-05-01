#pragma once
#include "structs.h"

class ocore
{
public:
	ocore();
	void run();

	trp_header header;
	std::vector<trp_entry> entries;
	std::vector<trp_entry_data> entries_data;
	std::vector<uint8_t> file_data;

	std::string dump_path = "";
	std::string NPID = "";

	bool load_trp(const std::string& filename);
	template <typename T>
	bool load_into(T* address, uintptr_t offset);
	bool load_header();
	void load_entries();
	void load_entries_data();
	void decrypt_entries();
	std::vector<uint8_t> decrypt_data(const std::vector<uint8_t>& encrypted_data, const std::string& NPID);
	void dump_files(bool images, bool esfm);
	void dump_file(int index);

	void clear_data();

	template <typename T>
	T swapendian(T value);
};

namespace globals
{
	inline ocore* core;

	inline HMODULE handle;

#ifdef _WINDLL
	void load(HMODULE hModule);
	void unload(HMODULE hModule);
#else
	void load();
	void unload();
#endif
}

namespace o = globals;
using namespace globals;