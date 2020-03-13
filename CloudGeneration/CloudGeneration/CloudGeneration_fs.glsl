#version 430
layout(location = 0) uniform float time;
layout(location = 1) uniform vec2 windowSize;
layout(location = 2) uniform mat4 _CameraToWorld;
layout(location = 3) uniform mat4 _CameraInverseProjection;
layout(location = 4) uniform vec3 _boxScale;
layout(location = 5) uniform vec3 _boxPosition;
layout(location = 6) uniform float _slider;
layout(location = 7) uniform sampler3D _CloudTexture;

out vec4 fragcolor;           
in vec2 tex_coord;

vec2 uv;	// current pixel pos on screen
const int RAY_MARCHING_STEPS = 32;
const float INF = 9999;
const float EPSILON = 0.0001;
const float PI = 3.1415926;
const float MAX_DIST = 100;
const vec4 skyColor = vec4(0.2,0.3,0.6,1);

struct Ray
{
    vec3 origin;
    vec3 direction;
};


Ray CreateRay(vec3 origin, vec3 direction)
{
    Ray ray;
    ray.origin = origin;
    ray.direction = direction;
    return ray;
}

Ray CreateCameraRay(vec2 uv)
{
    // Transform the camera origin to world space
    vec3 origin = (_CameraToWorld * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
    
    // Invert the perspective projection of the view-space position
    vec3 direction = (_CameraInverseProjection * vec4(uv, 0.0f, 1.0f)).xyz;
    // Transform the direction from camera to world space and normalize
    direction = (_CameraToWorld * vec4(direction, 0.0f)).xyz;
    direction = normalize(direction);
    return CreateRay(origin, direction);
}

float SphereDistance(vec3 eye, vec3 centre, float radius) {
    return distance(eye, centre) - radius;
}

float CubeDistance(vec3 eye, vec3 centre, vec3 scale) {
    vec3 o = abs(eye - centre) - scale;
    float ud = length(max(o,0));
    float n = max(max(min(o.x,0),min(o.y,0)), min(o.z,0));
    return ud+n;
}

float distanceField(vec3 p)
{
	float cubeDist = CubeDistance(p, _boxPosition, _boxScale);
//	float sphereDist = SphereDistance(p, vec3(0), _slider);
	return cubeDist;
}

vec3 Remap(vec3 p)
{
	return (p-_boxPosition)/(_boxScale*2) + vec3(0.5);
}

void main(void)
{   
//	fragcolor = texture(diffuse_color, tex_coord);
	uv = (vec2(gl_FragCoord.xy) + vec2(0.5, 0.5))/windowSize.xy * 2.0 - 1.0;
	// Get a ray for the UVs
    Ray ray = CreateCameraRay(uv);
	vec4 result = vec4(vec3(ray.direction.y * 0.5f + 0.5f), 1.0f);
	int marchSteps = 0;
	float rayDist = 0;
	float density = 0;
	while(rayDist < MAX_DIST)
	{
		marchSteps++;
		float dist = distanceField(ray.origin);
		if(dist <= EPSILON)
		{
			vec3 localPos = Remap(ray.origin);
			result = vec4(texture(_CloudTexture, localPos).r);	
			break;
		}
		ray.origin += ray.direction * dist;
        rayDist += dist;
	}
	fragcolor = result;
}




















