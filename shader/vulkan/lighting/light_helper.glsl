/**
 * Copyright (c) 2017 Darius Rückert 
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

//Intensity Attenuation based on the distance to the light source.
//Used by point and spot light.
float getAttenuation(vec4 attenuation, float distance){
    float radius = attenuation.w;
    //normalize the distance, so the attenuation is independent of the radius
    float x = distance / radius;
    //make sure that we return 0 if distance > radius, otherwise we would get an hard edge
    float smoothBorder = smoothstep(1.0f,0.9f,x);
    return smoothBorder / (attenuation.x +
                    attenuation.y * x +
                    attenuation.z * x * x);
}
