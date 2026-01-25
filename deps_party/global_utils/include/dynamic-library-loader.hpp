#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if __has_include(<windows.h>)
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>
    #undef WIN32_LEAN_AND_MEAN
    #undef NOMINMAX
using handle_t = HMODULE;
constexpr auto load_library_func = LoadLibraryW;
constexpr auto free_library_func = FreeLibrary;
using dll_directory_cookie_t = DLL_DIRECTORY_COOKIE;
constexpr auto add_dll_directory_func = AddDllDirectory;
constexpr auto remove_dll_directory_func = RemoveDllDirectory;
using proc_t = PROC;
constexpr auto get_proc_address_func = GetProcAddress;
constexpr auto get_last_error_func = GetLastError;
#else
    #error "This header is only for Windows platform."
#endif

class dynamic_library_loader
{
public:
    dynamic_library_loader() = default;
    dynamic_library_loader(const std::string& library_path);
    dynamic_library_loader(const std::string& library_path, const std::vector<std::string>& dependencies);

protected:
    std::string library_path;
    std::vector<std::string> dependencies;
    std::vector<dll_directory_cookie_t> dependencies_cookies;
    std::shared_ptr<void> library_handle;

public:
    virtual bool load() { return load_library() == 0; }
    virtual void unload() { library_handle.reset(); }

public:
    bool is_loaded() const { return library_handle != nullptr; }
    const std::string& get_library_path() const { return library_path; }
    const std::vector<std::string>& get_dependencies() const { return dependencies; }

private:
    int load_library()
    {
        if (library_handle)
            return 0; // Already loaded
        if (!dependencies.empty())
        {
            for (const auto& dep : dependencies)
            {
                std::wstring dep_w(dep.begin(), dep.end());
                auto dir_cookie = add_dll_directory_func(dep_w.c_str());
                dependencies_cookies.push_back(dir_cookie);
            }
        }
        std::wstring lib_path_w(library_path.begin(), library_path.end());
        library_handle = std::shared_ptr<void>(load_library_func(lib_path_w.c_str()), [cookies = dependencies_cookies](void* handle) {
            if (handle)
                free_library_func(static_cast<HMODULE>(handle));
            for (const auto& cookie : cookies)
                remove_dll_directory_func(cookie);
        });
        if (!library_handle)
            return static_cast<int>(get_last_error_func());
        return 0;
    }
    template <typename CFunc> void register_import_function()
    {
        auto func_name = typeid(CFunc).name();
        auto func_ptr = get_proc_address_func(static_cast<HMODULE>(library_handle.get()), func_name);
        if (!func_ptr)
            throw std::runtime_error("Failed to get function pointer for " + std::string(func_name));
        *reinterpret_cast<CFunc**>(&func_ptr) = reinterpret_cast<CFunc*>(func_ptr);
    }
    template <typename CFunc> void resolve_symbol(CFunc& func, const char* symbol_name)
    {
        if (!library_handle)
            throw std::runtime_error("Library not loaded");
        proc_t addr = get_proc_address_func(static_cast<HMODULE>(library_handle.get()), symbol_name);
        if (!addr)
            throw std::runtime_error("Failed to resolve symbol: " + std::string(symbol_name));
        func = reinterpret_cast<CFunc>(addr);
    }
    std::vector<proc_t> get_function_pointers(std::initializer_list<std::string> symbols)
    {
        std::vector<proc_t> function_pointers;
        if (!library_handle)
            return function_pointers;
        for (const auto& symbol : symbols)
            function_pointers.push_back(get_proc_address_func(static_cast<HMODULE>(library_handle.get()), symbol.c_str()));
        return function_pointers;
    }
};

#define MODULE_LOADER_NAME(ModuleName) ModuleName##Loader
