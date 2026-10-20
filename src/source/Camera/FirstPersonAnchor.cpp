#include "stdafx.h"
#include "FirstPersonAnchor.h"
#include "FirstPersonSettings.h"
#include "Engine/Object/w_ObjectInfo.h"
#include "Render/Models/ZzzBMD.h"

void Camera::FirstPerson::EyePosition(const OBJECT& object, const BMD& model,
    const FirstPersonSettings& settings, vec3_t eye)
{
    VectorCopy(object.Position, eye);
    const int head = model.BoneHead;
    if (!object.EnableBoneMatrix || !object.BoneTransform || head < 0
        || head >= model.NumBones || head >= MAX_BONES || !model.Bones || model.Bones[head].Dummy)
    {
        eye[2] += settings.eyeHeight * object.Scale;
        return;
    }

    // The completed animation prepass is rotated model space, before body scale
    // and world translation. Avoid BMD::BodyOrigin/BodyScale: other instances and
    // inventory previews share that mutable model state. Riding/jumping already
    // raise this bone, so adding a separate mount height would count it twice.
    for (int axis = 0; axis < 3; ++axis)
        eye[axis] += object.BoneTransform[head][axis][3] * object.Scale;
    eye[2] += settings.headOffset * object.Scale;
}
