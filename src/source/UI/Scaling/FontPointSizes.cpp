#include "UI/Scaling/UITransform.h"

namespace
{
constexpr int kNormalFontPointSize = 11;
constexpr int kMaximumNormalFontPointSize = 16;
constexpr int kBigFontPointSize = 22;
constexpr int kMaximumBigFontPointSize = 32;
constexpr int kFixedFontPointSize = 13;
constexpr int kMaximumFixedFontPointSize = 18;

struct FontPointRange
{
    int minimum;
    int maximum;
};

FontPointRange GetFontPointRange(UI::Scaling::FontRole role)
{
    if (role == UI::Scaling::FontRole::Big)
        return {kBigFontPointSize, kMaximumBigFontPointSize};
    if (role == UI::Scaling::FontRole::Fixed)
        return {kFixedFontPointSize, kMaximumFixedFontPointSize};
    return {kNormalFontPointSize, kMaximumNormalFontPointSize};
}
} // namespace

int UI::Scaling::MinimumFontPointSize(FontRole role)
{
    return GetFontPointRange(role).minimum;
}

int UI::Scaling::MaximumFontPointSize(FontRole role)
{
    return GetFontPointRange(role).maximum;
}
