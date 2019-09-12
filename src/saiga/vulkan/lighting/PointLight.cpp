/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * Created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#include "saiga/vulkan/lighting/PointLight.h"

namespace Saiga
{
namespace Vulkan
{
namespace Lighting
{
PointLight::PointLight(vec3 pos, vec3 dir, float angle) : light_pos(pos)
{
    this->light_dir = normalize(dir);
    this->angle     = clamp(angle, 0.f, 180.f);
}

//!
//! \brief PointLight::setPosition sets the position of the pointlight
//! \param pos
//!
void PointLight::setPosition(vec3 pos)
{
    light_pos = pos;
}

//!
//! \brief PointLight::setDirection sets the lights direction to the normalized incoming parameter
//! \param dir
//! \return normalized direction
//!
vec3 PointLight::setDirection(vec3 dir)
{
    light_dir = normalize(dir);
    return light_dir;
}

//!
//! \brief PointLight::setAngle sets lights opening angle to the given angle alamped to 0 to 180 degrees
//! \param angle
//! \return the clamped angle
//!
float PointLight::setAngle(float angle)
{
    this->angle = clamp(angle, 0.f, 180.f);
    return this->angle;
}

//!
//! \brief PointLight::getPosition
//! \return position of the light
//!
vec3 PointLight::getPosition()
{
    return light_pos;
}

//!
//! \brief PointLight::getDirection
//! \return direction of the light
//!
vec3 PointLight::getDirection()
{
    return light_dir;
}

//!
//! \brief PointLight::getAngle
//! \return opening angle of the light
//!
float PointLight::getAngle()
{
    return angle;
}
}  // namespace Lighting
}  // namespace Vulkan
}  // namespace Saiga
