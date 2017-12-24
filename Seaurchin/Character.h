#pragma once

#define SU_IF_ABILITY "Ability"

struct Character final {
    std::string Name;
    std::string Description;
    std::vector<std::string> Abilities;
};