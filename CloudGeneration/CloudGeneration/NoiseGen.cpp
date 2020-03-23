#include "NoiseGen.h"
#include <iostream>
#include <fstream>
using namespace std;

const int N = 64;
vec4 cloud[N * N * N];

const int W_l = 8;
const int W_m = 16;
const int W_h = 24;
const int W_hst = 32;

vec3 Worely_low[W_l * W_l * W_l];
vec3 Worely_mid[W_m * W_m * W_m];
vec3 Worely_high[W_h * W_h * W_h];
vec3 Worely_highest[W_hst * W_hst * W_hst];

NoiseGen::NoiseGen()
{
	
}

float GetRandom()
{
	return float(rand() % 10) / 10;
}

vec3 random3(vec3 st) {
	st = vec3(dot(st, vec3(127.1, 311.7,235.4)),
		dot(st, vec3(269.5, 183.3,421.8)), dot(st, vec3(133.6,462.5,248.1)));
	return -1.0f + 2.0f * fract(sin(st) * 43758.5453123f);
}

float Remap(float v, float l0, float h0, float ln, float hn)
{
	return ln + ((v - l0) * (hn - ln)) / (h0 - l0);
}

void NoiseGen::GenWorleyGrid()
{
	// For the Grid
	// between (0,1)
	for (int x = 0; x < W_l; x++)
	{
		for (int y = 0; y < W_l; y++)
		{
			for (int z = 0; z < W_l; z++)
			{
				Worely_low[x * W_l * W_l + y * W_l + z] = vec3(x + GetRandom(),
					y + GetRandom(),
					z + GetRandom()) / (float)W_l;
			}
		}
	}
	std::cout << "INFO: Generated low freq Worley Grid." << endl;

	for (int x = 0; x < W_m; x++)
	{
		for (int y = 0; y < W_m; y++)
		{
			for (int z = 0; z < W_m; z++)
			{
				Worely_mid[x * W_m * W_m + y * W_m + z] = vec3(x + GetRandom(),
					y + GetRandom(),
					z + GetRandom()) / (float)W_m;
			}
		}
	}
	std::cout << "INFO: Generated mid freq Worley Grid." << endl;

	for (int x = 0; x < W_h; x++)
	{
		for (int y = 0; y < W_h; y++)
		{
			for (int z = 0; z < W_h; z++)
			{
				Worely_high[x * W_h * W_h + y * W_h + z] = vec3(x + GetRandom(),
					y + GetRandom(),
					z + GetRandom()) / (float)W_h;
			}
		}
	}
	std::cout << "INFO: Generated high freq Worley Grid." << endl;

	for (int x = 0; x < W_hst; x++)
	{
		for (int y = 0; y < W_hst; y++)
		{
			for (int z = 0; z < W_hst; z++)
			{
				Worely_highest[x * W_hst * W_hst + y * W_hst + z] = vec3(x + GetRandom(),
													y + GetRandom(),
													z + GetRandom())/(float)W_hst;
			}
		}
	}
	std::cout << "INFO: Generated highest freq Worley Grid." << endl;
}

void NoiseGen::GenCloudTexture(GLuint &cloud_texture)
{
	// init
	srand(glutGet(GLUT_ELAPSED_TIME));
	GenWorleyGrid();
	std::cout << "INFO: Texture generating, stay tuned..." << std::endl;
	for (int x = 0; x < N; x++)
	{
		for (int y = 0; y < N; y++)
		{
			for (int z = 0; z < N; z++)
			{
				float R, G, B, A;
				// coord (0,1) fo 3D tex
				vec3 pos = vec3(x, y, z) / float(N);

				//Low freq Perlin-Worley
				R = (GetPerlinValue(pos, W_l));
				////Medium freq Worley
				//G = GetWorleyVaule(pos, W_l);

				////High freq Worley
				//B = GetWorleyVaule(pos, W_m);

				////Higest freq Worly
				//A = GetWorleyVaule(pos, W_h);

				if(distance(pos,vec3(0.5)) < 0.5) cloud[x * N * N + y * N + z] = vec4(R,0,0,0);
				else cloud[x * N * N + y * N + z] = vec4(0);

				// Debug
				//if (distance(vec3(x, y, z), vec3(N/2, N/2, N/2)) < N/2)
				//{
				//	//cout << distance(vec3(x, y, z), vec3(0, 0, 0)) << endl;
				//	cloud[x * N * N + y * N + z] = vec4(1,1,1,GetRandom());
				//}
				//else cloud[x * N * N + y * N + z] = vec4(0);

				/*cout << cloud[x * N * N + y * N + z].x <<
					cloud[x * N * N + y * N + z].y <<
					cloud[x * N * N + y * N + z].z <<
					cloud[x * N * N + y * N + z].w << endl;*/
			}
		}
	}
	glGenTextures(1, &cloud_texture);
	glBindTexture(GL_TEXTURE_3D, cloud_texture);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, N, N, N, 0, GL_RGBA, GL_FLOAT, cloud);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_3D, 0);

	std::cout << "INFO: Texture Generated!" << std::endl;
}

float NoiseGen::GetPerlinValue(vec3 texPos, int freq)
{
	// texPos: (0,1)

	vec3 i = floor(texPos * (float)freq);
	vec3 f = fract(texPos * (float)freq);

	vec3 u = f * f * (3.0f - 2.0f * f);

	float result = mix(mix(mix(dot(random3(i + vec3(0.0, 0.0, 0.0)), f - vec3(0.0, 0.0, 0.0)),
		dot(random3(i + vec3(1.0, 0.0, 0.0)), f - vec3(1.0, 0.0, 0.0)), u.x),
		mix(dot(random3(i + vec3(0.0, 1.0, 0.0)), f - vec3(0.0, 1.0, 0.0)),
			dot(random3(i + vec3(1.0, 1.0, 0.0)), f - vec3(1.0, 1.0, 0.0)), u.x), u.y),
		mix(mix(dot(random3(i + vec3(0.0, 0.0, 1.0)), f - vec3(0.0, 0.0, 1.0)),
			dot(random3(i + vec3(1.0, 0.0, 1.0)), f - vec3(1.0, 0.0, 1.0)), u.x),
			mix(dot(random3(i + vec3(0.0, 1.0, 1.0)), f - vec3(0.0, 1.0, 1.0)),
				dot(random3(i + vec3(1.0, 1.0, 1.0)), f - vec3(1.0, 1.0, 1.0)), u.x), u.y),u.z);
	result = Remap(result, -1, 1, 0, 1);
	return result;
}

float NoiseGen::GetWorleyVaule(vec3 texPos, int freq)
{
	// texPos: (0,1)
	int x = floor(texPos.x * freq);
	int y = floor(texPos.y * freq);
	int z = floor(texPos.z * freq);
	float result = 1;

	vec3 *p = NULL;

	switch (freq)
	{
	case W_l:
		p = Worely_low;
		break;
	case W_m:
		p = Worely_mid;
		break;
	case W_h:
		p = Worely_high;
		break;
	case W_hst:
		p = Worely_highest;
		break;
	default:
		p = NULL;
		std::cout << "ERROR: Send unknown freq in Worely Generation." << endl;
		break;
	}

	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			for (int k = -1; k <= 1; k++)
			{
				if (x + i >= 0 && x + i < freq &&
					y + j >= 0 && y + j < freq &&
					z + k >= 0 && z + k < freq)
				{
					int offest = (x + i) * freq * freq + (y + j) * freq + (z + k);
					float currentDist = distance(texPos, *(p+offest));
					result = min(result, currentDist);
				}
			}
		}
	}
	p = NULL;

	// remap t0 (0,1)
	float maxDist = (float)2 / freq;
	result = Remap(result, 0, maxDist, 0, 1);
	//cout << result << endl;
	// Inverse? 1- result
	return 1-result;
}
