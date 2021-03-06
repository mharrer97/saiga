#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable


struct RayPayload {
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
	float diff;
};

layout(location = 0) rayPayloadInNV RayPayload rayPayload;
layout(location = 2) rayPayloadNV bool shadowed;

hitAttributeNV vec3 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;
layout(binding = 2, set = 0) uniform CameraProperties 
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 lightPos;
        vec4 attenuation;
        vec4 dir;
        vec4 specularCol;
        vec4 diffuseCol;
        float openingAngle;
        int time;
} cam;
layout(binding = 3, set = 0) buffer Vertices { vec4 v[]; } vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices;

struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec3 color;
  float ior;
  float diff;
  float data;
};

Vertex unpack(uint index)
{
	vec4 d0 = vertices.v[3 * index + 0];
	vec4 d1 = vertices.v[3 * index + 1];
	vec4 d2 = vertices.v[3 * index + 2];

	Vertex v;
	v.pos = d0.xyz;
	v.normal = vec3(d0.w, d1.x, d1.y);
	v.color = vec3(d1.z, d1.w, d2.x);
	v.ior = d2.y;
	v.diff = d2.z;
	v.data = 0.f;
	return v;
}

vec3 interpolateVec3(vec3 p0, vec3 p1, vec3 p2){
    return (1.f - attribs.x - attribs.y) * p0 + attribs.x * p1 + attribs.y * p2;
}

float interpolateFloat(float p0, float p1, float p2){
    return (1.f - attribs.x - attribs.y) * p0 + attribs.x * p1 + attribs.y * p2;
}
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

void main()
{
	ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);

	Vertex v0 = unpack(index.x);
	Vertex v1 = unpack(index.y);
	Vertex v2 = unpack(index.z);

	// Interpolate normal
	vec3 normal = normalize( gl_ObjectToWorldNV * vec4(normalize(interpolateVec3(v0.normal, v1.normal,v2.normal)), 0.f));

	// interpolate position
	vec3 position = gl_ObjectToWorldNV * vec4(interpolateVec3(v0.pos,v1.pos,v2.pos), 1.f);

	//interpolate color
	vec3 color = interpolateVec3(v0.color, v1.color,v2.color);

	// Basic lighting
	vec3 lightVector = (cam.lightPos.xyz-position.xyz);

	float intensity = getAttenuation(cam.attenuation, length(lightVector)) * cam.diffuseCol.w;

	//normalize now -> was used for intensity
	lightVector = normalize(lightVector);
	//reflect
	vec3 viewVec = normalize(position - vec3(cam.viewInverse[3][0], cam.viewInverse[3][1], cam.viewInverse[3][2]));
	vec3 reflected = reflect(lightVector, normal);
	
	vec3 diffuse = max(dot(normal, lightVector) * intensity, 0.f) * color * cam.diffuseCol.xyz;

	//specular still broken
	float roughness = 16.f;
	vec3 specular = pow(max(dot(reflected,viewVec), 0.f), roughness) * vec3(0.75f) * intensity * cam.specularCol.xyz;

	

	rayPayload.color = specular+diffuse;

	float angle = acos(dot(lightVector, normalize(-cam.dir.xyz)));
	float alpha = (clamp((angle/6.26) * 180.f, (cam.openingAngle/4.f) - 5.f, (cam.openingAngle/4.f))- ((cam.openingAngle/4.f) - 5.f)) / 5.f;
	rayPayload.color = mix(vec3(0.f), rayPayload.color,1.f- alpha);

	//TODO delete: ambient term
	//vec3 ambient = color * 0.1f; // TODO delete: currently the ambient term
	//rayPayload.color += ambient;
	//rayPayload.color = normal;

	rayPayload.distance = gl_RayTmaxNV;
	rayPayload.normal = normal;

	// Objects with full white vertex color are treated as reflectors
	rayPayload.reflector = interpolateFloat(v0.ior,v1.ior,v2.ior);//((v0.color.r == 1.0f) && (v0.color.g == 1.0f) && (v0.color.b == 1.0f)) ? 1.0f : 0.0f; 
	rayPayload.diff = interpolateFloat(v0.diff, v1.diff, v2.diff);
	
	
	// Shadow casting
	vec3 origin = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;
	float tmin = 0.001;
	//get distance between origin and light pos
	float dist = length(cam.lightPos.xyz - origin.xyz);
	float tmax = min(100.0, dist);
	shadowed = true;  
	// Offset indices to match shadow hit/miss index
	traceNV(topLevelAS, gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsOpaqueNV|gl_RayFlagsSkipClosestHitShaderNV, 0xFF, 1, 0, 1, origin, tmin, lightVector, tmax, 2);
	if (shadowed) {
		rayPayload.color = vec3(0.f);
	}
}
