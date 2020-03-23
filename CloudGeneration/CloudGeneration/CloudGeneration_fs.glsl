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
const int RAY_MARCHING_STEPS = 64;
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

struct RayHit{
	vec3 position;
	float hitDist;
	float alpha;
	float entryPoint;
	float exitPoint;
};

RayHit CreateRayHit()
{
	RayHit hit;
	hit.position = vec3(0,0,0);
	hit.hitDist = INF;
	hit.alpha = 0;
	hit.entryPoint = 0;
	hit.exitPoint = INF;
	return hit;
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

vec4 AlphaBlend(vec4 a, vec4 b)
{
	vec4 result = a;
	result.xyz = a.xyz * (1 - b.w) + b.xyz * b.w;
	result.w = 1;
	return result;
}

struct Box
{
	vec3 position;
	vec3 scale;
}_box;

Box CreateBox(vec3 position, vec3 scale)
{
	Box box;
	box.position = position;
	box.scale = scale;
	return box;
}

void IntersectBox(Ray ray, inout RayHit bestHit, Box box)
{
	vec3 minBound = box.position - box.scale;
	vec3 maxBound = box.position + box.scale;

	vec3 t0 = (minBound - ray.origin) / ray.direction;
	vec3 t1 = (maxBound - ray.origin) / ray.direction;

	vec3 tsmaller = min(t0, t1);
	vec3 tbigger = max(t0, t1);

	float tmin = max(tsmaller[0], max(tsmaller[1], tsmaller[2]));
    float tmax = min(tbigger[0], min(tbigger[1], tbigger[2]));

	if(tmin > tmax) return;

	// else
	// Hit a box!
	if(tmax > 0 && tmin < bestHit.hitDist)
	{
		if(tmin < 0) tmin = 0;
		bestHit.hitDist = tmin;
		bestHit.position = ray.origin + bestHit.hitDist * ray.direction;
		bestHit.alpha = 1;
		// For volumetric rendering
		bestHit.entryPoint = tmin;
		bestHit.exitPoint = tmax;
	}
}

vec3 GetLocPos(vec3 pos)
{
	return ((pos -_box.position)/_box.scale+1)*0.5;
//	return pos * _boxScale + _slider;
}

//float SphereDistance(vec3 eye, vec3 centre, float radius) {
//    return distance(eye, centre) - radius;
//}
//
//float CubeDistance(vec3 eye, vec3 centre, vec3 scale) {
//    vec3 o = abs(eye - centre) - scale;
//    float ud = length(max(o,0));
//    float n = max(max(min(o.x,0),min(o.y,0)), min(o.z,0));
//    return ud+n;
//}
//
//float distanceField(vec3 p)
//{
//	float cubeDist = CubeDistance(p, _boxPosition, _boxScale);
////	float sphereDist = SphereDistance(p, vec3(0), _slider);
//	return cubeDist;
//}

vec3 Remap(vec3 p)
{
	return (p-_boxPosition)/(_boxScale*2) + vec3(0.5);
}

vec4 GetSkyColor(float y)
{
	 y = y*0.5 + 0.5;
	return mix(vec4(0.5,0.3,0.5,1.0),skyColor,y);
}

vec3 sunPos = vec3(5,5,5);
float lightMarch(vec3 p)
{
	float light = 0; 
	vec3 lightDir = normalize(sunPos - p);
	float stepSize = distance(sunPos, p) / 4.0;
	for(int i = 0; i < 4; i++)
	{
		float density = texture(_CloudTexture, GetLocPos(p)).r;
		if(density > _slider) light += exp(-i*stepSize);
		p += lightDir*stepSize;
	}
	return light;
}

float SampleDensity(vec3 p, float len)
{
	vec4 cloudShape =  texture(_CloudTexture, p);
	return cloudShape.r * exp(-len);
}

void Volume(Ray ray, RayHit hit, inout vec4 result)
{
	vec3 start = hit.position;
	vec3 end = hit.position + ray.direction*(hit.exitPoint-hit.entryPoint);
	float len = distance(start,end);
	float stepSize = len / float(RAY_MARCHING_STEPS);
//	int num = int(len/stepSize);

	vec3 eachStep =  stepSize * normalize(end - start);
	vec3 currentPos = start;

	 // https://www.youtube.com/watch?v=4QOcCGI6xOU
	 // exp for lighting
	vec3 localPos;
	vec3 lightColor = vec3(0.7);
	vec3 cloudColor = vec3(1);
	float totalDensity = 0;
	result.w = 0;
	 for(int i = 0; i < RAY_MARCHING_STEPS; i++)
	 {
	 	localPos = GetLocPos(currentPos);
	 	
		float opacity = SampleDensity(localPos, i*stepSize);
		if(opacity > _slider)
		{
			result.xyz = result.xyz * (1 - opacity) + cloudColor * opacity;
			result.w += opacity;
		}
		if(result.w >= 1) break;
	 	currentPos += eachStep;
	 }
	
	// Debug
//	localPos = GetLocPos(currentPos);
//	vec4 currentColor = vec4(texture(_CloudTexture,localPos+vec3(0,0,_slider)));
//	result = currentColor;
}

RayHit CastRay(Ray ray)
{
	RayHit bestHit = CreateRayHit();
	IntersectBox(ray, bestHit, _box);
	return bestHit;
}

void main(void)
{   
	// uv (-1,1)
	uv = (vec2(gl_FragCoord.xy) + vec2(0.5, 0.5))/windowSize.xy * 2.0 - 1.0;
	// Get a ray for the UVs
    Ray ray = CreateCameraRay(uv);
	vec4 result = GetSkyColor(-ray.direction.y);
	_box = CreateBox(_boxPosition, _boxScale);
	RayHit hit = CreateRayHit();
	hit = CastRay(ray);
	if(hit.alpha != 0)
	{
		Volume(ray,hit,result);
	}
	fragcolor = result;
}




















