#pragma once
#include <SDL2/SDL.h>
#include <string>

// The palette sampled from the mockups.
namespace col {

constexpr SDL_Color bg          {0x0d, 0x0f, 0x13, 255};
constexpr SDL_Color sidebar     {0x10, 0x12, 0x17, 255};
constexpr SDL_Color sideSel     {0x1e, 0x22, 0x2a, 255};
constexpr SDL_Color divider     {0x1c, 0x20, 0x28, 255};
constexpr SDL_Color card        {0x15, 0x18, 0x1e, 255};
constexpr SDL_Color cardBorder  {0x20, 0x24, 0x2c, 255};
constexpr SDL_Color cardFocus   {0x1c, 0x20, 0x28, 255};
constexpr SDL_Color rowSep      {0x22, 0x26, 0x2e, 255};
constexpr SDL_Color modal       {0x17, 0x1a, 0x21, 255};
constexpr SDL_Color modalBorder {0x25, 0x2a, 0x34, 255};
constexpr SDL_Color rowSel      {0x1f, 0x23, 0x2c, 255};
constexpr SDL_Color drawer      {0x14, 0x17, 0x1d, 255};
constexpr SDL_Color chip        {0x1e, 0x22, 0x2a, 255};
constexpr SDL_Color button      {0x26, 0x2a, 0x33, 255};

constexpr SDL_Color text        {0xf2, 0xef, 0xea, 255};
constexpr SDL_Color textDim     {0x9d, 0xa2, 0xac, 255};
constexpr SDL_Color textMuted   {0x6a, 0x70, 0x7b, 255};
constexpr SDL_Color textFaint   {0x3f, 0x44, 0x4e, 255};

constexpr SDL_Color accent      {0xff, 0x5a, 0x4e, 255};
constexpr SDL_Color danger      {0xff, 0x7a, 0x70, 255};
constexpr SDL_Color dangerBtn   {0xd6, 0x46, 0x3d, 255};
constexpr SDL_Color green       {0x5b, 0xd6, 0xa0, 255};
constexpr SDL_Color gold        {0xf6, 0xc4, 0x53, 255};
// The Alpha and Shiny Alpha icons' own colours (img/states, from pkHouse).
constexpr SDL_Color alpha       {0xe0, 0x55, 0x55, 255};
constexpr SDL_Color shinyAlpha  {0xf4, 0xe0, 0x04, 255};

constexpr SDL_Color keyCap      {0xe9, 0xe6, 0xe0, 255};
constexpr SDL_Color keyCapText  {0x0d, 0x0f, 0x13, 255};
constexpr SDL_Color toggleOff   {0x2e, 0x33, 0x3d, 255};
constexpr SDL_Color knobOff     {0x8a, 0x90, 0x9c, 255};
constexpr SDL_Color dot         {0x34, 0x3a, 0x45, 255};
constexpr SDL_Color dotGrid     {0x1a, 0x1d, 0x23, 255};

} // namespace col

// `a` moved towards `b` by `t` (0 = a, 1 = b). Alpha comes from `a`.
inline SDL_Color mix(SDL_Color a, SDL_Color b, float t) {
    auto ch = [t](Uint8 x, Uint8 y) { return static_cast<Uint8>(x + (y - x) * t + 0.5f); };
    return SDL_Color{ch(a.r, b.r), ch(a.g, b.g), ch(a.b, b.b), a.a};
}

inline SDL_Color withAlpha(SDL_Color c, Uint8 a) { return SDL_Color{c.r, c.g, c.b, a}; }

// The dot beside a type name, keyed by the English name in the data files
// (the displayed name may be translated). The games' own palette: sampled from
// the in-game type icons (pkHouse romfs/types, see its tools/gen_type_colors.py).
inline SDL_Color typeColor(const std::string& t) {
    struct Entry { const char* name; SDL_Color c; };
    static const Entry table[] = {
        {"Normal",   {0x9f, 0xa1, 0x9f, 255}}, {"Fighting", {0xff, 0x80, 0x00, 255}},
        {"Flying",   {0x81, 0xb9, 0xef, 255}}, {"Poison",   {0x91, 0x41, 0xcb, 255}},
        {"Ground",   {0x91, 0x51, 0x21, 255}}, {"Rock",     {0xaf, 0xa9, 0x81, 255}},
        {"Bug",      {0x91, 0xa1, 0x19, 255}}, {"Ghost",    {0x70, 0x41, 0x70, 255}},
        {"Steel",    {0x60, 0xa1, 0xb8, 255}}, {"Fire",     {0xe6, 0x28, 0x29, 255}},
        {"Water",    {0x29, 0x80, 0xef, 255}}, {"Grass",    {0x3f, 0xa1, 0x29, 255}},
        {"Electric", {0xfa, 0xc0, 0x00, 255}}, {"Psychic",  {0xef, 0x41, 0x79, 255}},
        {"Ice",      {0x3f, 0xd8, 0xff, 255}}, {"Dragon",   {0x50, 0x61, 0xe1, 255}},
        {"Dark",     {0x50, 0x41, 0x3f, 255}}, {"Fairy",    {0xef, 0x71, 0xef, 255}},
    };
    for (const auto& e : table)
        if (t == e.name) return e.c;
    return col::knobOff;
}
