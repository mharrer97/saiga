/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#version 450


layout(location=0) in vec4 inPosition;
layout(location=1) in vec4 inNormal;
layout(location=2) in vec4 inColor;
layout(location=3) in vec4 inData;


out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location=0) out VertexData
{
    vec2 tc;
} outData;

void main() 
{
    outData.tc = inPosition.xy * 0.5f + 0.5f;
    gl_Position = vec4(inPosition.x,inPosition.y,0,1);


    //gl_Position.z = 0.5;
}
