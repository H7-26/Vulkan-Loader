/*
 * Copyright (c) 2021 The Khronos Group Inc.
 * Copyright (c) 2021 Valve Corporation
 * Copyright (c) 2021 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and/or associated documentation files (the "Materials"), to
 * deal in the Materials without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Materials, and to permit persons to whom the Materials are
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice(s) and this permission notice shall be included in
 * all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
 * USE OR OTHER DEALINGS IN THE MATERIALS.
 *
 * Author: Charles Giessen <charles@lunarg.com>
 */

#include "shim.h"

#include <random>

void PlatformShim::redirect_all_paths(fs::path const& path) {
    redirect_category(path, ManifestCategory::implicit_layer);
    redirect_category(path, ManifestCategory::explicit_layer);
    redirect_category(path, ManifestCategory::icd);
}

std::vector<std::string> parse_env_var_list(std::string const& var) {
    std::vector<std::string> items;
    size_t start = 0;
    size_t len = 0;
    for (size_t i = 0; i < var.size(); i++) {
#if defined(WIN32)
        if (var[i] == ';') {
#elif COMMON_UNIX_PLATFORMS
        if (var[i] == ':') {
#endif
            if (len != 0) {
                // only push back non empty strings
                items.push_back(var.substr(start, len));
            }
            start = i + 1;
            len = 0;
        } else {
            len++;
        }
    }
    items.push_back(var.substr(start, len));

    return items;
}

std::vector<std::string> get_folder_contents(std::vector<fs::FolderManager>* folders, std::string folder_name) noexcept {
    for (auto& folder : *folders) {
        if (folder.location() == folder_name) {
            return folder.get_files();
        }
    }
    return {};
}

#if defined(WIN32)

D3DKMT_Adapter& D3DKMT_Adapter::add_driver_manifest_path(fs::path const& src) { return add_path(src, driver_paths); }
D3DKMT_Adapter& D3DKMT_Adapter::add_implicit_layer_manifest_path(fs::path const& src) {
    return add_path(src, implicit_layer_paths);
}
D3DKMT_Adapter& D3DKMT_Adapter::add_explicit_layer_manifest_path(fs::path const& src) {
    return add_path(src, explicit_layer_paths);
}

D3DKMT_Adapter& D3DKMT_Adapter::add_path(fs::path src, std::vector<std::wstring>& dest) {
    std::wstring dest_path;
    dest_path.resize(src.size());
    MultiByteToWideChar(CP_UTF8, 0, src.c_str(), static_cast<int>(src.size()), &dest_path[0], static_cast<int>(dest_path.size()));
    dest.push_back(dest_path);
    return *this;
}

std::string category_path_name(ManifestCategory category) {
    if (category == ManifestCategory::implicit_layer) return "ImplicitLayers";
    if (category == ManifestCategory::explicit_layer)
        return "ExplicitLayers";
    else
        return "Drivers";
}

void PlatformShim::reset() {
    hkey_current_user_explicit_layers.clear();
    hkey_current_user_implicit_layers.clear();
    hkey_local_machine_explicit_layers.clear();
    hkey_local_machine_implicit_layers.clear();
    hkey_local_machine_drivers.clear();
    hkey_local_machine_settings.clear();
    hkey_current_user_settings.clear();
}

void PlatformShim::set_fake_path(ManifestCategory category, fs::path const& path) {}
void PlatformShim::add_known_path(fs::path const& path) {}

void PlatformShim::add_manifest(ManifestCategory category, fs::path const& path) {
    if (category == ManifestCategory::settings) {
        hkey_local_machine_settings.emplace_back(path.str());
    } else if (category == ManifestCategory::implicit_layer) {
        hkey_local_machine_implicit_layers.emplace_back(path.str());
    } else if (category == ManifestCategory::explicit_layer) {
        hkey_local_machine_explicit_layers.emplace_back(path.str());
    } else {
        hkey_local_machine_drivers.emplace_back(path.str());
    }
}

void PlatformShim::add_unsecured_manifest(ManifestCategory category, fs::path const& path) {
    if (category == ManifestCategory::settings) {
        hkey_current_user_settings.emplace_back(path.str());
    } else if (category == ManifestCategory::implicit_layer) {
        hkey_current_user_implicit_layers.emplace_back(path.str());
    } else if (category == ManifestCategory::explicit_layer) {
        hkey_current_user_explicit_layers.emplace_back(path.str());
    }
}

void PlatformShim::add_dxgi_adapter(GpuType gpu_preference, DXGI_ADAPTER_DESC1 desc1) {
    dxgi_adapters.push_back(DXGIAdapter(gpu_preference, desc1, next_adapter_handle++));
}

void PlatformShim::add_d3dkmt_adapter(D3DKMT_Adapter const& adapter) { d3dkmt_adapters.push_back(adapter); }

void PlatformShim::set_app_package_path(fs::path const& path) {
    app_package_path.resize(path.size());
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &app_package_path[0], static_cast<int>(app_package_path.size()));
}

// TODO:
void PlatformShim::add_CM_Device_ID(std::wstring const& id, fs::path const& icd_path, fs::path const& layer_path) {
    //     // append a null byte as separator if there is already id's in the list
    //     if (CM_device_ID_list.size() != 0) {
    //         CM_device_ID_list += L'\0';  // I'm sure this wont cause issues with std::string down the line... /s
    //     }
    //     CM_device_ID_list += id;
    //     std::string id_str(id.length(), '\0');
    //     size_t size_written{};
    //     wcstombs_s(&size_written, &id_str[0], id_str.length(), id.c_str(), id.length());

    //     std::string device_path = std::string(pnp_registry_path) + "\\" + id_str;
    //     CM_device_ID_registry_keys.push_back(device_path.c_str());
    //     add_key_value_string(id_key, "VulkanDriverName", icd_path.c_str());
    //     add_key_value_string(id_key, "VulkanLayerName", layer_path.c_str());
    //     // TODO: decide how to handle 32 bit
    //     // add_key_value_string(id_key, "VulkanDriverNameWoW", icd_path.c_str());
    //     // add_key_value_string(id_key, "VulkanLayerName", layer_path.c_str());
}

void PlatformShim::redirect_category(fs::path const& new_path, ManifestCategory search_category) {}

#elif COMMON_UNIX_PLATFORMS

#include <dirent.h>
#include <unistd.h>

std::string category_path_name(ManifestCategory category) {
    if (category == ManifestCategory::settings) return "settings.d";
    if (category == ManifestCategory::implicit_layer) return "implicit_layer.d";
    if (category == ManifestCategory::explicit_layer)
        return "explicit_layer.d";
    else
        return "icd.d";
}

void PlatformShim::reset() { redirection_map.clear(); }

bool PlatformShim::is_fake_path(fs::path const& path) { return redirection_map.count(path.str()) > 0; }
fs::path const& PlatformShim::get_real_path_from_fake_path(fs::path const& path) { return redirection_map.at(path.str()); }
void PlatformShim::redirect_path(fs::path const& path, fs::path const& new_path) { redirection_map[path.str()] = new_path; }
void PlatformShim::remove_redirect(fs::path const& path) { redirection_map.erase(path.str()); }

bool PlatformShim::is_known_path(fs::path const& path) { return known_path_set.count(path.str()) > 0; }
void PlatformShim::add_known_path(fs::path const& path) { known_path_set.insert(path.str()); }
void PlatformShim::remove_known_path(fs::path const& path) { known_path_set.erase(path.str()); }

void PlatformShim::add_manifest(ManifestCategory category, fs::path const& path) {}
void PlatformShim::add_unsecured_manifest(ManifestCategory category, fs::path const& path) {}

void parse_and_add_env_var_override(std::vector<std::string>& paths, std::string env_var_contents) {
    auto parsed_paths = parse_env_var_list(env_var_contents);
    paths.insert(paths.end(), parsed_paths.begin(), parsed_paths.end());
}

void PlatformShim::redirect_category(fs::path const& new_path, ManifestCategory category) {
    std::vector<std::string> paths;
    auto home = fs::path(get_env_var("HOME"));
    if (category == ManifestCategory::settings) {
        redirect_path(home / ".local/share/vulkan" / category_path_name(category), new_path);
        return;
    }

    if (home.size() != 0) {
        paths.push_back((home / ".config").str());
        paths.push_back((home / ".local/share").str());
    }
    // Don't report errors on apple - these env-vars are not suppose to be defined
    bool report_errors = true;
#if defined(__APPLE__)
    report_errors = false;
#endif
    parse_and_add_env_var_override(paths, get_env_var("XDG_CONFIG_HOME", report_errors));
    if (category == ManifestCategory::explicit_layer) {
        parse_and_add_env_var_override(paths, get_env_var("VK_LAYER_PATH", false));  // don't report failure
    }
    parse_and_add_env_var_override(paths, FALLBACK_DATA_DIRS);
    parse_and_add_env_var_override(paths, FALLBACK_CONFIG_DIRS);

    auto sys_conf_dir = std::string(SYSCONFDIR);
    if (!sys_conf_dir.empty()) {
        paths.push_back(sys_conf_dir);
    }
#if defined(EXTRASYSCONFDIR)
    // EXTRASYSCONFDIR default is /etc, if SYSCONFDIR wasn't defined, it will have /etc put
    // as its default. Don't want to double add it
    auto extra_sys_conf_dir = std::string(EXTRASYSCONFDIR);
    if (!extra_sys_conf_dir.empty() && sys_conf_dir != extra_sys_conf_dir) {
        paths.push_back(extra_sys_conf_dir);
    }
#endif

    for (auto& path : paths) {
        if (!path.empty()) {
            redirect_path(fs::path(path) / "vulkan" / category_path_name(category), new_path);
        }
    }
}

void PlatformShim::set_fake_path(ManifestCategory category, fs::path const& path) {
    // use /etc as the 'redirection path' by default since its always searched
    redirect_path(fs::path(SYSCONFDIR) / "vulkan" / category_path_name(category), path);
}

void PlatformShim::redirect_dlopen_name(fs::path const& filename, fs::path const& actual_path) {
    dlopen_redirection_map[filename.str()] = actual_path;
}

bool PlatformShim::is_dlopen_redirect_name(fs::path const& filename) { return dlopen_redirection_map.count(filename.str()) == 1; }

fs::path PlatformShim::query_default_redirect_path(ManifestCategory category) {
    return fs::path(SYSCONFDIR) / "vulkan" / category_path_name(category);
}
#endif
