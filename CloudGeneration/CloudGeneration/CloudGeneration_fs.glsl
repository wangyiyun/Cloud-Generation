#version 430
layout(location = 0) uniform float time;
layout(location = 1) uniform vec2 windowSize;
layout(location = 2) uniform mat4 _CameraToWorld;
layout(location = 3) uniform mat4 _CameraInverseProjection;
layout(location = 4) uniform vec3 _boxScale;
layout(location = 5) uniform vec3 _boxPosition;
layout(location = 6) uniform vec4 _FBM;
layout(location = 7) uniform sampler3D _CloudShape;
layout(location = 8) uniform sampler3D _CloudDetail;
layout(location = 9) uniform vec4 _slider;

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

float Remap(float v, float l0, float h0, float ln, float hn)
{
	return ln + ((v - l0) * (hn - ln)) / (h0 - l0);
}

float SAT(float v)
{
	if(v > 1) return 1;
	if(v < 0) return 0;
	return v;
}

// need new pos calculation
vec3 GetLocPos(vec3 pos)
{
	return ((pos -_box.position)/(_box.scale)+1)*0.5;
}

vec4 GetSkyColor(float y)
{
	 y = y*0.5 + 0.5;
	return mix(skyColor,vec4(0.7,0.5,0.8,1.0),y);
}

float SampleDensity(vec3 p)
{	
	// p is world pos
	vec3 uvw = GetLocPos(p);
	vec4 cloudShape =  texture(_CloudShape, uvw);
	
	// calculate base shape density
	// Refer: Sebastian Lague @ Code Adventure
	float boxBottom = _box.position.y - _box.scale.y;
	float heightPercent = (p.y - boxBottom) / (_box.scale.y);
	float heightGradient = SAT(Remap(heightPercent, 0.0, 0.2,0,1)) * SAT(Remap(heightPercent, 1, 0.7, 0,1));
	float shapeFBM = (cloudShape.r*_FBM.x + cloudShape.g*_FBM.y + cloudShape.b*_FBM.z + cloudShape.a*_FBM.w)* heightGradient;
	
	if(shapeFBM <= 0)
	{

		vec4 cloudDetail =  texture(_CloudDetail, uvw);
		float detailFBM = (cloudDetail.r*_slider.x + cloudDetail.g*_slider.y + cloudDetail.b*_slider.z);
		float density = max(0, shapeFBM - 0.5)*1.2*50;
		return density;
	}

	return 0;
}

vec3 sunDir = vec3(0,1,0);
const int LIGHT_MARCH_NUM = 10;
float lightMarch(vec3 p)
{
	float totalDensity = 0; 
	vec3 lightDir = normalize(sunDir);
	Ray lightRay = CreateRay(p, lightDir);
	RayHit lightHit = CreateRayHit();
	IntersectBox(lightRay, lightHit,_box);

	float distInBox = abs(lightHit.exitPoint - lightHit.entryPoint);

//	return distInBox;

	float stepSize = distInBox / float(LIGHT_MARCH_NUM);
	for(int i = 0; i < LIGHT_MARCH_NUM; i++)
	{
		p += lightDir*stepSize;
		totalDensity += max(0.0, SampleDensity(p)*stepSize);
	}
	float transmittance = exp(-totalDensity*1.5);
	float darknessThreshold = 0.7;
	return darknessThreshold + transmittance*(1-darknessThreshold);
}

vec3 lightColor = vec3(0.9);

void Volume(Ray ray, RayHit hit, inout vec4 result)
{
	vec3 start = hit.position;
	vec3 end = hit.position + ray.direction*abs(hit.exitPoint-hit.entryPoint);
	float len = distance(start,end);
	float stepSize = len / float(RAY_MARCHING_STEPS);

	vec3 eachStep =  stepSize * normalize(end - start);
	vec3 currentPos = start;

	// debug
	vec3 localPos = GetLocPos(currentPos) + vec3(0,0,_slider.y);
	vec4 shape = texture(_CloudShape, localPos);
	vec4 detail = texture(_CloudDetail, localPos);
	if(localPos.y > _slider.x) result.xyz = vec3(shape.x);
	else result.xyz = vec3(detail.x);

//	 // exp for lighting
//	vec3 lightEnergy = vec3(0.0);
//	float transmittance = 1;
//	 for(int i = 0; i < RAY_MARCHING_STEPS; i++)
//	 {
//		float density = SampleDensity(currentPos);
//		if(density > 0)
//		{
//			float lightTransmittance = lightMarch(currentPos);
//			lightEnergy += density*stepSize*transmittance*lightTransmittance;
//			transmittance *= exp(-density*stepSize);
//
//			if(transmittance < 0.01)
//			{
//				break;
//			}
//
//		}
//		currentPos += eachStep;	 	
//	 }
//
//	 vec3 cloudColor = lightEnergy * lightColor;
//	 result.xyz = result.xyz*transmittance + cloudColor;

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




















