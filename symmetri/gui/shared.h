#pragma once
#include <functional>
#include <string>

#include "imgui.h"
#include "symmetri/colors.hpp"

ImU32 getColor(symmetri::Token token);

void drawColorDropdownMenu(const char* menu_name,
                           const std::vector<std::string>& colors,
                           const std::function<void(const std::string&)>& func);

void drawColorDropdownMenu(const char* menu_name,
                           const std::vector<const char*>& colors,
                           const std::function<void(const char*)>& func);
