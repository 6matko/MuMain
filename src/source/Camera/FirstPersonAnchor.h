#pragma once

#include "Core/Globals/_types.h"

class OBJECT;
class BMD;
struct FirstPersonSettings;

namespace Camera::FirstPerson
{
void EyePosition(const OBJECT& object, const BMD& model, const FirstPersonSettings& settings, vec3_t eye);
}
