#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <string>

// Correct relative paths based on your file tree
#include "memory/memory.hpp"
#include "macho/macho.hpp"
#include "scanner/scanner.hpp"
#include "roblox/offsets.hpp"

namespace fs = std::filesystem;

/** * Saves the found offset to ~/Documents/offsets.txt.
 * Checks for existing entries to prevent duplicates.
 */
void save_to_local_file(uintptr_t offset) {
    const char* home = std::getenv("HOME");
    if (!home) return;

    fs::path doc_path = fs::path(home) / "Documents" / "offsets.txt";
    std::string entry = "PRINT_FUNCTION = 0x";
    
    // Manually formatting hex for standalone compatibility
    char hex_str[20];
    snprintf(hex_str, sizeof(hex_str), "%lx", offset);
    entry += hex_str;

    if (fs::exists(doc_path)) {
        std::ifstream infile(doc_path);
        std::string line;
        while (std::getline(infile, line)) {
            if (line.find(entry) != std::string::npos) return; 
        }
    }

    std::ofstream outfile(doc_path, std::ios::app);
    if (outfile.is_open()) {
        outfile << entry << " (Found: " << __DATE__ << ")" << std::endl;
        std::cout << "[+] Offset saved to Documents/offsets.txt" << std::endl;
    }
}

/** * Scans the target task memory for the Intel x86_64 Print signature.
 */
void find_print_offset(task_t task, vm_address_t image_base) {
    // Intel x86_64 AOB for Print (push rbp; mov rbp, rsp...)
    const std::vector<uint8_t> print_signature = { 
        0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54, 0x53, 0x48, 0x81, 0xEC 
    };

    // Locate __TEXT section using provided macho utility
    auto text_section = macho::get_section(task, image_base, "__TEXT", "__text");
    if (!text_section) return;

    // Read bytes using provided memory utility
    std::vector<uint8_t> buffer(text_section->size);
    if (!memory::read_bytes(task, text_section->address, buffer.data(), text_section->size)) return;

    // Pattern match search
    for (size_t i = 0; i <= buffer.size() - print_signature.size(); ++i) {
        if (std::memcmp(&buffer[i], print_signature.data(), print_signature.size()) == 0) {
            uintptr_t offset = (text_section->address + i) - image_base;
            std::cout << "[+] Found Print Offset: 0x" << std::hex << offset << std::endl;
            save_to_local_file(offset);
            return;
        }
    }
}

int main() {
#ifdef __APPLE__
    task_t task = mach_task_self(); 
    vm_address_t base = (vm_address_t)_get_image_header(0);
    find_print_offset(task, base);
#else
    std::cout << "Compile and run this on your Intel Mac." << std::endl;
#endif
    return 0;
}