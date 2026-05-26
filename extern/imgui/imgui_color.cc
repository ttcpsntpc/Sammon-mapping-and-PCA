#include "imgui_color.h"

namespace imgui
{
namespace color
{
ImColor const kWhite = {1.0f, 1.0f, 1.0f, 1.0f};
ImColor const kRed = {1.0f, 0.0f, 0.0f, 1.0f};
ImColor const kGreen = {0.0f, 1.0f, 0.0f, 1.0f};
ImColor const kBlue = {0.0f, 0.0f, 1.0f, 1.0f};
ImColor const kGray = { 0.5f, 0.5f, 0.5f, 1.0f };
std::array<ImColor, 4> const kPrimary = {kRed, kGreen, kBlue, kGray};

ImColor
SetAlpha(ImColor color, float alpha)
{
  if (alpha < 0.0f || alpha > 1.0f) {
    throw std::invalid_argument("Alpha value must be between 0 and 1.");
  }
  return {color.Value.x, color.Value.y, color.Value.z, alpha};
}

}
}
