#include "pch.h"
#include "core.h"
#include "gui/gui.h"

ocore::ocore()
{
    /*filedata = loadtrp("TROPHY.TRP");

    std::cout << "NPID (e.g., NPWR03361_00): ";

    header = get_header();
    load_entries();
    load_entries_data();
    dump_path += NPID;
    decrypt_entries(NPID);

    dump_files();*/
}

void ocore::run()
{
    gui::CreateHWindow("preise - trp dumper");
    if (!gui::CreateDevice())
        return;
    gui::CreateImGui();

    gui::notification->add(5.f, true, "WELCOME TO PREISE'S TROPHY DUMPER!");

    if (!globals::core)
        return;

    while (gui::isRunning)
    {
        gui::BeginRender();
        gui::Render();
        gui::EndRender();
    }

    gui::DestroyImGui();
    gui::DestroyDevice();
    gui::DestroyHWindow();
    o::unload();
}


bool ocore::load_trp(const std::string& filename)
{
    if (filename.empty())
    {
        return false;
    }

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
    {
        gui::notification->add(7.f, true, "FAILED TO OPEN FILE: %s", filename.c_str());
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);

    if (!file.read((char*)buffer.data(), size))
    {
        gui::notification->add(5.f, true, "FAILED TO READ FILE: %s", filename.c_str());
        return false;
    }

    file_data = buffer;

    if (!buffer.empty())
    {
        return true;
    }
    else
    {
        gui::notification->add(5.f, true, "FAILED TO LOAD FILE: %s", filename.c_str());
        return false;
    }
}
template <typename T>
bool ocore::load_into(T* address, uintptr_t offset)
{
    if (address == nullptr || offset + sizeof(T) > file_data.size())
    {
        return false;
    }

    std::memcpy(address, file_data.data() + offset, sizeof(T));
    return true;
}
bool ocore::load_header()
{
    if (!load_into(&header, 0))
        return false;

    header.magic;
    header.version = swapendian(header.version);
    header.file_size = swapendian(header.file_size);
    header.entry_num = swapendian(header.entry_num);
    header.entry_size = swapendian(header.entry_size);
    header.dev_flag = swapendian(header.dev_flag);
    header.key_index = swapendian(header.key_index);

    return true;
}
void ocore::load_entries_data()
{
    for (int i = 0; i < entries.size(); i++)
    {
        trp_entry_data entry_data;
        std::copy(std::begin(entries[i].entry_name), std::end(entries[i].entry_name), std::begin(entry_data.entry_name));
        entry_data.entry_pos = entries[i].entry_pos;
        entry_data.entry_len = entries[i].entry_len;
        entry_data.flag = entries[i].flag;

        entry_data.data = std::vector<uint8_t>(entry_data.entry_len);

        std::memcpy(entry_data.data.data(), file_data.data() + entry_data.entry_pos, entry_data.entry_len);

        entries_data.push_back(entry_data);
    }
}
void ocore::load_entries()
{
    uintptr_t offset = 0x60;
    for (int i = 0; i < header.entry_num; i++)
    {
        trp_entry entry;
        load_into(&entry, offset);

        entry.entry_pos = swapendian(entry.entry_pos);
        entry.entry_len = swapendian(entry.entry_len);
        entry.flag = swapendian(entry.flag);

        entries.push_back(entry);

        offset += 0x40;
    }
}
void ocore::decrypt_entries()
{
    for (int i = 0; i < entries_data.size(); i++)
    {
        if (entries_data[i].flag == 3)
        {
            entries_data[i].data = decrypt_data(entries_data[i].data, NPID);
        }
    }
}
std::vector<uint8_t> ocore::decrypt_data(const std::vector<uint8_t>& encrypted_data, const std::string& NPID) {
    // Define the key
    static const unsigned char Trophy_Key[16] = {
        0x21, 0xF4, 0x1A, 0x6B, 0xAD, 0x8A, 0x1D, 0x3E,
        0xCA, 0x7A, 0xD5, 0x86, 0xC1, 0x01, 0xB7, 0xA9,
    };

    // Set up the initialization vector (IV)
    unsigned char iv[AES_BLOCK_SIZE] = { 0 };

    // Prepare key for AES CBC
    unsigned char key[AES_BLOCK_SIZE];
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create EVP_CIPHER_CTX");

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, Trophy_Key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize encryption");
    }

    // Create key by encrypting NPID padded to 16 bytes
    int outlen;
    if (!NPID.empty()) {
        std::string padded_npid = NPID;
        padded_npid.resize(16, '\0');  // Pad NPID to 16 bytes
        EVP_EncryptUpdate(ctx, key, &outlen, reinterpret_cast<const unsigned char*>(padded_npid.data()), 16);
    }
    else {
        memcpy(key, Trophy_Key, 16);
    }
    EVP_CIPHER_CTX_free(ctx);

    // Decrypt using AES CBC
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create EVP_CIPHER_CTX");

    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize decryption");
    }

    std::vector<uint8_t> decrypted_data(encrypted_data.size());
    int final_len = 0;

    if (!encrypted_data.empty()) {
        if (EVP_DecryptUpdate(ctx, decrypted_data.data(), &outlen, encrypted_data.data(), encrypted_data.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Decryption failed");
        }
        final_len += outlen;

        if (EVP_DecryptFinal_ex(ctx, decrypted_data.data() + outlen, &outlen) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Decryption finalization failed");
        }
        final_len += outlen;
    }
    EVP_CIPHER_CTX_free(ctx);

    decrypted_data.resize(final_len);  // Resize vector to the actual decrypted size

    // Remove padding bytes
    auto it = std::find_if(decrypted_data.begin(), decrypted_data.end(), [](uint8_t byte) { return byte != 0x00; });
    decrypted_data.erase(decrypted_data.begin(), it);

    return decrypted_data;
}
void ocore::dump_files(bool images, bool esfm)
{
    for (int i = 0; i < entries_data.size(); i++)
    {
        if (images && entries_data[i].flag == 0)
			dump_file(i);
        if (esfm && entries_data[i].flag == 3)
            dump_file(i);
    }
}
void ocore::dump_file(int index)
{
    std::filesystem::create_directory(dump_path);

    switch (entries_data[index].flag)
    {
    case 0:
    {
        std::ofstream file(dump_path + "/" + std::string((char*)entries_data[index].entry_name), std::ios::binary);
        file.write((char*)entries_data[index].data.data(), entries_data[index].data.size());
        file.close();
        break;
    }

    case 3:
    {
        std::string filename((char*)entries_data[index].entry_name);
        filename = filename.substr(0, filename.size() - 4) + "xml";
        std::ofstream file(dump_path + "/" + filename, std::ios::binary);
        file.write((char*)entries_data[index].data.data(), entries_data[index].data.size());
        file.close();
        break;
    }

    }

    std::cout << "Dumped " << std::string((char*)entries_data[index].entry_name) << std::endl;
}

void ocore::clear_data() {
    header = trp_header();
    entries.clear();
    entries_data.clear();
    file_data.clear();

    dump_path.clear();
    NPID.clear();
}

template <typename T>
T ocore::swapendian(T value) {
    static_assert(std::is_trivially_copyable<T>::value, "Not a TriviallyCopyable type");

    std::array<char, sizeof(T)> bytes;
    std::memcpy(bytes.data(), &value, sizeof(T));
    std::reverse(bytes.begin(), bytes.end());
    std::memcpy(&value, bytes.data(), sizeof(T));

    return value;
}

namespace globals
{

#ifdef _WINDLL
    void load(HMODULE hModule)
    {

    }

    void unload(HMODULE hModule)
    {
        delete core;
        core = nullptr;
    }
#else
    void load()
    {
        core = new ocore();
        if (!core)
			return;

        core->run();
    }

    void unload()
    {
        if (gui::notification)
		{
			delete gui::notification;
			gui::notification = nullptr;
		}

        if (core)
		{
			delete core;
			core = nullptr;
		}
    }
#endif
}