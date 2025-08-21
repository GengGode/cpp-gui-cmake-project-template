#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <Windows.h>

class dynamic_library_loader
{
public:
    dynamic_library_loader() = default;
    dynamic_library_loader(const std::string& library_path);
    dynamic_library_loader(const std::string& library_path, const std::vector<std::string>& dependencies);

protected:
    std::string library_path;
    std::vector<std::string> dependencies;

    std::shared_ptr<void> library_handle;

public:
    bool load();
    void unload();
    bool is_loaded() const;
    const std::string& get_library_path() const { return library_path; }
    const std::vector<std::string>& get_dependencies() const { return dependencies; }
    std::shared_ptr<void> get_library_handle() const { return library_handle; }

private:
    int load_library()
    {
        if (library_handle)
            return 0; // Already loaded
        std::vector<DLL_DIRECTORY_COOKIE> dependencies_cookies;
        if (!dependencies.empty())
        {
            for (const auto& dep : dependencies)
            {
                std::wstring dep_w(dep.begin(), dep.end());
                auto dir_cookie = AddDllDirectory(dep_w.c_str());
                if (dir_cookie == NULL)
                    return GetLastError();
                dependencies_cookies.push_back(dir_cookie);
            }
        }
        std::wstring lib_path_w(library_path.begin(), library_path.end());
        library_handle = std::shared_ptr<void>(LoadLibraryW(lib_path_w.c_str()), [](void* handle) {
            if (handle)
                FreeLibrary(static_cast<HMODULE>(handle));
        });
        if (!library_handle)
            return GetLastError();
        for (const auto& cookie : dependencies_cookies)
        {
            if (!RemoveDllDirectory(cookie))
                return GetLastError();
        }

        return 0; // Success
    }
    template <typename CFunc> void resgister_import_function()
    {
        auto func_name = typeid(CFunc).name();
        auto func_ptr = GetProcAddress(static_cast<HMODULE>(library_handle.get()), func_name);
        if (!func_ptr)
        {
            throw std::runtime_error("Failed to get function pointer for " + std::string(func_name));
        }
        *reinterpret_cast<CFunc**>(&func_ptr) = reinterpret_cast<CFunc*>(func_ptr);
    }
};