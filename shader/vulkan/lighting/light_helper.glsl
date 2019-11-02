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

vec3 reconstructPosition(float d, vec2 tc, mat4 proj){
	float z = d * 2.0 - 1.0;
    vec4 p = vec4(tc.xy * 2.0f - 1.0f,d,1);
    p = inverse(proj) * p; //TODO outsource inverse to cpu?
    return p.xyz/p.w;
}


float textureProj(vec4 shadowCoord, vec2 off, sampler2D shadowMap, float ambientIntensity)
{
        float shadow = 1.0;
        if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 )
        {
                float dist = texture( shadowMap, shadowCoord.xy + off ).r;
                if ( shadowCoord.w > 0.0 && dist * 0.5f + 0.5f < shadowCoord.z - 0.0005 )
                {
                        shadow = ambientIntensity;
                }
        }
        return shadow;
}

//classic pcf
float filterPCF(vec4 sc, sampler2D shadowMap, float ambientIntensity)
{
        ivec2 texDim = textureSize(shadowMap, 0);
        float scale = 1.0;
        float dx = scale * 1.0 / float(texDim.x);
        float dy = scale * 1.0 / float(texDim.y);

        float shadowFactor = 0.0;
        int count = 0;
        int range = 1;

        for (int x = -range; x <= range; x++)
        {
                for (int y = -range; y <= range; y++)
                {
                        shadowFactor += textureProj(sc, vec2(dx*x, dy*y), shadowMap, ambientIntensity);
                        count++;
                }

        }
        return shadowFactor / count;
}
