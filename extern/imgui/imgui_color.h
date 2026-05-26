#pragma once
#include <imgui.h>
#include <array>
#include <stdexcept>

namespace imgui
{
namespace color
{
extern ImColor const kWhite;
extern ImColor const kRed;
extern ImColor const kGreen;
extern ImColor const kBlue;
extern ImColor const kGray;
extern std::array<ImColor, 4> const kPrimary;

extern ImColor
SetAlpha(ImColor color, float alpha);
}
}
