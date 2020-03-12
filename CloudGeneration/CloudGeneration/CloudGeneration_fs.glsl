#version 430
layout(location = 0) uniform float time;
layout(location = 1) uniform vec2 windowSize;
layout(location = 2) uniform mat4 _CameraToWorld;
layout(location = 3) uniform mat4 _CameraInverseProjection;
layout(location = 4) uniform vec3 _boxScale;
layout(location = 5) uniform vec3 _boxPosition;

out vec4 fragcolor;           
in vec2 tex_coord;

vec2 uv;	// current pixel pos on screen
const int RAY_MARCHING_STEPS = 32;
const float INF = 9999;

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

float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

vec4 AlphaBlend(vec4 a, vec4 b, float alpha)
{
//	if(b.w <= 0) return a;
//	a.xyz += b.xyz * b.w * alpha;
//	return a;

	vec4 result = a;
	result.xyz = a.xyz * (1 - b.w) + b.xyz * b.w;
	result.w = 1;
	return result;
}

void Volume(Ray ray, RayHit hit, inout vec4 result)
{
//	int stepNum = RAY_MARCHING_STEPS;
//	vec3 start = hit.position;
//	vec3 end = hit.position + ray.direction*(hit.exitPoint-hit.entryPoint);
//	float len = distance(end,start);
//	float stepSize = len/stepNum;
	
	vec3 start = hit.position;
	vec3 end = hit.position + ray.direction*(hit.exitPoint-hit.entryPoint);
	float len = distance(end,start);
	float stepSize = 0.01;
	int stepNum = int(len/stepSize);
	vec3 step =  stepSize * normalize(end - start);

	vec3 currentPos = start;

	vec3 localPos;


	 for(int i = 0; i < stepNum; i++)
	 {
	 	result = AlphaBlend(result, vec4(1,1,1,0.02), 0.5);

	 	currentPos += step;
	 }
}

RayHit CastRay(Ray ray)
{
	RayHit bestHit = CreateRayHit();
	IntersectBox(ray, bestHit, _box);
	return bestHit;
}

void main(void)
{   
//	fragcolor = texture(diffuse_color, tex_coord);
	uv = (vec2(gl_FragCoord.xy) + vec2(0.5, 0.5))/windowSize.xy * 2.0 - 1.0;
	// Get a ray for the UVs
    Ray ray = CreateCameraRay(uv);
	RayHit hit = CreateRayHit();
	_box = CreateBox(_boxPosition, _boxScale);
	vec4 result = vec4(ray.direction * 0.5 + 0.5,1);
	hit = CastRay(ray);
	if(hit.alpha != 0)
	{
		Volume(ray,hit,result);
	}
	fragcolor = result;
}




















