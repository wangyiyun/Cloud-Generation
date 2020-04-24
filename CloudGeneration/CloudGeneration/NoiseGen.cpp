#include "NoiseGen.h"
#include <iostream>
#include <fstream>
using namespace std;

const int N = 128;
vec4 shapeData[N * N * N];

const int M = 32;
vec4 detailData[M * M * M];

const int W_l = 8;
const int W_m = 12;
const int W_h = 16;

vec3 Worely_low[W_l * W_l * W_l];
vec3 Worely_mid[W_m * W_m * W_m];
vec3 Worely_high[W_h * W_h * W_h];

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

bool LoadVolumeFromFile(const char* fileName, GLuint& tex, int dataSizeX, int dataSizeY, int dataSizeZ) {
	FILE* pFile = fopen(fileName, "rb");
	if (NULL == pFile) {
		return false;
	}
	int dataSize = dataSizeX * dataSizeY * dataSizeZ;
	glm::vec4 * pVolume = new glm::vec4[dataSize];
	fread(pVolume, sizeof(glm::vec4), dataSize, pFile);
	fclose(pFile);

	//load data into a 3D texture
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_3D, tex);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, dataSizeX, dataSizeY, dataSizeZ, 0, GL_RGBA, GL_FLOAT, pVolume);

	delete[] pVolume;
	return true;
}

bool WriteVolumeToFile(const char* fileName, vec4 *data, int dataSizeX, int dataSizeY, int dataSizeZ)
{
	FILE* pFile = fopen(fileName, "wb");
	if (NULL == pFile) {
		return false;
	}
	int dataSize = dataSizeX * dataSizeY * dataSizeZ;
	fwrite(data, sizeof(glm::vec4), dataSize, pFile);
	fclose(pFile);
}

void NoiseGen::GenWorleyGrid()
{
	// init
	srand(glutGet(GLUT_ELAPSED_TIME));
	memset(Worely_low, 0, sizeof(Worely_low));
	memset(Worely_mid, 0, sizeof(Worely_mid));
	memset(Worely_high, 0, sizeof(Worely_high));

	// For the Grid
	// between (0,1)
	for (int x = 0; x < W_l; x++)
	{
		for (int y = 0; y < W_l; y++)
		{
			for (int z = 0; z < W_l; z++)
			{
				if (x == 0 || x == W_l - 1 ||
					y == 0 || y == W_l - 1 ||
					z == 0 || z == W_l - 1)
				{
					Worely_low[x * W_l * W_l + y * W_l + z] = vec3(-1);
					continue;
				}
				else
				{
					Worely_low[x * W_l * W_l + y * W_l + z] = vec3(x + GetRandom(),
																	y + GetRandom(),
																	z + GetRandom()) / (float)W_l;
				}
				
			}
		}
	}

	for (int x = 0; x < W_l; x++)
	{
		for (int y = 0; y < W_l; y++)
		{
			for (int z = 0; z < W_l; z++)
			{
				if (x == 0 || x == W_l - 1 ||
					y == 0 || y == W_l - 1 ||
					z == 0 || z == W_l - 1)
				{
					int fixX, fixY, fixZ;
					float offsetX, offsetY, offsetZ;
					
					if (x == 0) { fixX = W_l - 2; offsetX = -(W_l - 2); }
					else if (x == W_l - 1) { fixX = 1; offsetX = (W_l - 2); }
					else { fixX = x; offsetX = 0; }

					if (y == 0) { fixY = W_l - 2; offsetY = -(W_l - 2); }
					else if (y == W_l - 1) { fixY = 1; offsetY = (W_l - 2); }
					else { fixY = y; offsetY = 0; }

					if (z == 0) { fixZ = W_l - 2; offsetZ = -(W_l - 2); }
					else if (z == W_l - 1) { fixZ = 1; offsetZ = (W_l - 2); }
					else { fixZ = z; offsetZ = 0; }

					Worely_low[x * W_l * W_l + y * W_l + z] = Worely_low[fixX * W_l * W_l + fixY * W_l + fixZ]+vec3(offsetX, offsetY, offsetZ) / (float)W_l;
				}
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
				if (x == 0 || x == W_m - 1 ||
					y == 0 || y == W_m - 1 ||
					z == 0 || z == W_m - 1)
				{
					Worely_mid[x * W_m * W_m + y * W_m + z] = vec3(-1);
					continue;
				}
				else
				{
					Worely_mid[x * W_m * W_m + y * W_m + z] = vec3(x + GetRandom(),
						y + GetRandom(),
						z + GetRandom()) / (float)W_m;
				}

			}
		}
	}

	for (int x = 0; x < W_m; x++)
	{
		for (int y = 0; y < W_m; y++)
		{
			for (int z = 0; z < W_m; z++)
			{
				if (x == 0 || x == W_m - 1 ||
					y == 0 || y == W_m - 1 ||
					z == 0 || z == W_m - 1)
				{
					int fixX, fixY, fixZ;
					float offsetX, offsetY, offsetZ;

					if (x == 0) { fixX = W_m - 2; offsetX = -(W_m - 2); }
					else if (x == W_m - 1) { fixX = 1; offsetX = (W_m - 2); }
					else { fixX = x; offsetX = 0; }

					if (y == 0) { fixY = W_m - 2; offsetY = -(W_m - 2); }
					else if (y == W_m - 1) { fixY = 1; offsetY = (W_m - 2); }
					else { fixY = y; offsetY = 0; }

					if (z == 0) { fixZ = W_m - 2; offsetZ = -(W_m - 2); }
					else if (z == W_m - 1) { fixZ = 1; offsetZ = (W_m - 2); }
					else { fixZ = z; offsetZ = 0; }

					Worely_mid[x * W_m * W_m + y * W_m + z] = Worely_mid[fixX * W_m * W_m + fixY * W_m + fixZ] + vec3(offsetX, offsetY, offsetZ) / (float)W_m;
				}
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
				if (x == 0 || x == W_h - 1 ||
					y == 0 || y == W_h - 1 ||
					z == 0 || z == W_h - 1)
				{
					Worely_high[x * W_h * W_h + y * W_h + z] = vec3(-1);
					continue;
				}
				else
				{
					Worely_high[x * W_h * W_h + y * W_h + z] = vec3(x + GetRandom(),
						y + GetRandom(),
						z + GetRandom()) / (float)W_h;
				}

			}
		}
	}

	for (int x = 0; x < W_h; x++)
	{
		for (int y = 0; y < W_h; y++)
		{
			for (int z = 0; z < W_h; z++)
			{
				if (x == 0 || x == W_h - 1 ||
					y == 0 || y == W_h - 1 ||
					z == 0 || z == W_h - 1)
				{
					int fixX, fixY, fixZ;
					float offsetX, offsetY, offsetZ;

					if (x == 0) { fixX = W_h - 2; offsetX = -(W_h - 2); }
					else if (x == W_h - 1) { fixX = 1; offsetX = (W_h - 2); }
					else { fixX = x; offsetX = 0; }

					if (y == 0) { fixY = W_h - 2; offsetY = -(W_h - 2); }
					else if (y == W_h - 1) { fixY = 1; offsetY = (W_h - 2); }
					else { fixY = y; offsetY = 0; }

					if (z == 0) { fixZ = W_h - 2; offsetZ = -(W_h - 2); }
					else if (z == W_h - 1) { fixZ = 1; offsetZ = (W_h - 2); }
					else { fixZ = z; offsetZ = 0; }

					Worely_high[x * W_h * W_h + y * W_h + z] = Worely_high[fixX * W_h * W_h + fixY * W_h + fixZ] + vec3(offsetX, offsetY, offsetZ) / (float)W_h;
				}
			}
		}
	}
	std::cout << "INFO: Generated high freq Worley Grid." << endl;
}

void NoiseGen::GetGloudShape(GLuint &cloud_shape)
{

	if (LoadVolumeFromFile("CloudShapeData.raw", cloud_shape, N, N, N))
	{
		std::cout << "INFO: Cloud shape texture exist!" << std::endl;
		return;
	}
	
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

				//Medium freq Worley
				G = GetWorleyVaule(pos, W_l);

				//High freq Worley
				B = GetWorleyVaule(pos, W_m);

				//Higest freq Worly
				A = GetWorleyVaule(pos, W_h);

				shapeData[x * N * N + y * N + z] = vec4(R,G,B,A);
			}
		}
	}
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &cloud_shape);
	glBindTexture(GL_TEXTURE_3D, cloud_shape);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, N, N, N, 0, GL_RGBA, GL_FLOAT, shapeData);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_3D, 0);

	WriteVolumeToFile("CloudShapeData.raw", shapeData, N, N, N);

	std::cout << "INFO: Cloud shape texture Generated!" << std::endl;
}

void NoiseGen::GetGloudDetail(GLuint& cloud_detail)
{
	if (LoadVolumeFromFile("CloudDetailData.raw", cloud_detail, M, M, M))
	{
		std::cout << "INFO: Cloud detail texture exist!" << std::endl;
		return;
	}

	GenWorleyGrid();

	for (int x = 0; x < M; x++)
	{
		for (int y = 0; y < M; y++)
		{
			for (int z = 0; z < M; z++)
			{
				float R, G, B;
				// coord (0,1) fo 3D tex
				vec3 pos = vec3(x, y, z) / float(M);

				//Low freq Perlin-Worley
				R = GetWorleyVaule(pos, W_l);
				
				//Medium freq Worley
				G = GetWorleyVaule(pos, W_m);
				
				//High freq Worley
				B = GetWorleyVaule(pos, W_h);

				detailData[x * M * M + y * M + z] = vec4(R, G, B, 0);
			}
		}
	}
	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &cloud_detail);
	glBindTexture(GL_TEXTURE_3D, cloud_detail);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, M, M, M, 0, GL_RGBA, GL_FLOAT, detailData);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_3D, 0);

	WriteVolumeToFile("CloudDetailData.raw", detailData, M, M, M);

	std::cout << "INFO: Cloud detail texture Generated!" << std::endl;

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
	
	// map to (1, freq-2)
	int x = floor(texPos.x * (freq - 2)) + 1;
	int y = floor(texPos.y * (freq - 2)) + 1;
	int z = floor(texPos.z * (freq - 2)) + 1;
	// texPos: (0,1) map to
	texPos = texPos * ((freq - 2) / (float)freq) + 1/ (float)freq;
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
				{
					int offset = (x + i) * freq * freq + (y + j) * freq + (z + k);
					float currentDist = distance(texPos, p[offset]);
					result = min(result, currentDist);
					if (result == 0) break;
				}
			}
		}
	}
	p = NULL;

	// remap t0 (0,1)
	float maxDist = (float)2 / (freq);
	result = Remap(result, 0, maxDist, 0, 0.9);
	// Inverse
	return 1 - result;
}