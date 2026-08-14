#pragma once

#include <filesystem>

#include "game.h"

void save_state_to_file(const State& state, const std::filesystem::path& filepath);
void load_state_from_file(State& state, const std::filesystem::path& filepath);
