#include "LoadMesh.h"
#include <fstream>
#include <algorithm>

#include <GL/glew.h>
#include "assimp/Importer.hpp"
#include "assimp/PostProcess.h"

// Create an instance of the Importer class
Assimp::Importer gImporter;

void BufferIndexedVerts(MeshData& meshdata);
void GetBoundingBox(const aiScene* scene, aiVector3D* min, aiVector3D* max);
void GetBoundingBox(const aiMesh* mesh, aiVector3D* min, aiVector3D* max);

MeshData LoadMesh(const std::string& pFile)
{
   MeshData mesh;
   mesh.mFilename = pFile;

   //check if file exists
   std::ifstream fin(pFile.c_str());
   if (!fin.fail())
   {
      fin.close();
   }
   else
   {
      printf("Couldn't open file: %s\n", pFile.c_str());
      printf("%s\n", gImporter.GetErrorString());
      return mesh;
   }

   mesh.mScene = gImporter.ReadFile(pFile, aiProcessPreset_TargetRealtime_Quality | aiProcess_PreTransformVertices); //PreTransformVertices makes multiple submeshes work

                                                                                                                     // If the import failed, report it
   if (!mesh.mScene)
   {
      printf("%s\n", gImporter.GetErrorString());
      return mesh;
   }

   // Now we can access the file's contents.
   printf("Import of scene %s succeeded.\n", pFile.c_str());

   GetBoundingBox(mesh.mScene, &mesh.mBbMin, &mesh.mBbMax);
   aiVector3D diff = mesh.mBbMax - mesh.mBbMin;
   float w = std::max(diff.x, std::max(diff.y, diff.z));

   mesh.mScaleFactor = 1.0f / w;

   BufferIndexedVerts(mesh);

   return mesh;
}

void GetBoundingBox(const aiMesh* mesh, aiVector3D* min, aiVector3D* max)
{
   min->x = min->y = min->z = 1e10f;
   max->x = max->y = max->z = -1e10f;

   for (unsigned int t = 0; t < mesh->mNumVertices; ++t)
   {
      aiVector3D tmp = mesh->mVertices[t];

      min->x = std::min(min->x, tmp.x);
      min->y = std::min(min->y, tmp.y);
      min->z = std::min(min->z, tmp.z);

      max->x = std::max(max->x, tmp.x);
      max->y = std::max(max->y, tmp.y);
      max->z = std::max(max->z, tmp.z);
   }
}


void GetBoundingBoxForNode(const aiScene* scene, const aiNode* nd, aiVector3D* min, aiVector3D* max)
{
   unsigned int n = 0, t;

   for (; n < nd->mNumMeshes; ++n)
   {
      const aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
      for (t = 0; t < mesh->mNumVertices; ++t)
      {

         aiVector3D tmp = mesh->mVertices[t];

         min->x = std::min(min->x, tmp.x);
         min->y = std::min(min->y, tmp.y);
         min->z = std::min(min->z, tmp.z);

         max->x = std::max(max->x, tmp.x);
         max->y = std::max(max->y, tmp.y);
         max->z = std::max(max->z, tmp.z);
      }
   }

   for (n = 0; n < nd->mNumChildren; ++n)
   {
      GetBoundingBoxForNode(scene, nd->mChildren[n], min, max);
   }
}


void GetBoundingBox(const aiScene* scene, aiVector3D* min, aiVector3D* max)
{
   min->x = min->y = min->z = 1e10f;
   max->x = max->y = max->z = -1e10f;
   GetBoundingBoxForNode(scene, scene->mRootNode, min, max);
}

void BufferIndexedVerts(MeshData& meshdata)
{

   if (meshdata.mVao != -1)
   {
      glDeleteVertexArrays(1, &meshdata.mVao);
   }

   if (meshdata.mIndexBuffer != -1)
   {
      glDeleteBuffers(1, &meshdata.mIndexBuffer);
   }

   if (meshdata.mVboVerts != -1)
   {
      glDeleteBuffers(1, &meshdata.mVboVerts);
   }

   if (meshdata.mVboTexCoords != -1)
   {
      glDeleteBuffers(1, &meshdata.mVboTexCoords);
   }

   if (meshdata.mVboNormals != -1)
   {
      glDeleteBuffers(1, &meshdata.mVboNormals);
   }

   GLint program = -1;
   glGetIntegerv(GL_CURRENT_PROGRAM, &program);

   //shader attrib locations
   const int pos_loc = 0;
   const int tex_coord_loc = 1;
   const int normal_loc = 2;

   glBindAttribLocation(program, pos_loc, "pos_attrib");
   glBindAttribLocation(program, tex_coord_loc, "tex_coord_attrib");
   glBindAttribLocation(program, normal_loc, "normal_attrib");

   glGenVertexArrays(1, &meshdata.mVao);
   glBindVertexArray(meshdata.mVao);

   const int numSubmeshes = meshdata.mScene->mNumMeshes;
   meshdata.mSubmesh.resize(numSubmeshes);

   int totalNumVerts = 0;
   int totalNumIndices = 0;

   for (int m = 0; m < numSubmeshes; m++)
   {
      meshdata.mSubmesh[m].mNumIndices = meshdata.mScene->mMeshes[m]->mNumFaces * 3;
      meshdata.mSubmesh[m].mBaseIndex = totalNumIndices;
      meshdata.mSubmesh[m].mBaseVertex = totalNumVerts;

      totalNumVerts += meshdata.mScene->mMeshes[m]->mNumVertices;
      totalNumIndices += meshdata.mSubmesh[m].mNumIndices;
   }

   std::vector<unsigned int> indices(totalNumIndices);

   unsigned int faceIndex = 0;
   for (int m = 0; m<numSubmeshes; m++)
   {
      int meshFaces = meshdata.mScene->mMeshes[m]->mNumFaces;
      for (unsigned int f = 0; f < meshFaces; ++f)
      {
         const aiFace* face = &meshdata.mScene->mMeshes[m]->mFaces[f];

         memcpy(&indices[faceIndex], face->mIndices, 3 * sizeof(unsigned int));
         faceIndex += 3;
      }
   }

   //Buffer indices
   glGenBuffers(1, &meshdata.mIndexBuffer);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshdata.mIndexBuffer);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * totalNumIndices, indices.data(), GL_STATIC_DRAW);


   //Buffer vertices
   {
      glGenBuffers(1, &meshdata.mVboVerts);
      glBindBuffer(GL_ARRAY_BUFFER, meshdata.mVboVerts);
      glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * totalNumVerts, 0, GL_STATIC_DRAW);

      int offset = 0;
      for (int m = 0; m<numSubmeshes; m++)
      {
         aiMesh* mesh = meshdata.mScene->mMeshes[m];
         if (mesh->HasPositions())
         {
            //TODO for animated meshes: inline aiNode* FindNode(const aiString& name), and compute transformation
            //aiNode* node = FindNode(mesh->mName);

            const int size = 3 * sizeof(float)*mesh->mNumVertices;
            const void* data = mesh->mVertices;
            glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
            offset += size;
         }
      }
      glEnableVertexAttribArray(pos_loc);
      glVertexAttribPointer(pos_loc, 3, GL_FLOAT, 0, 0, 0);
   }

   // buffer texture coordinates
   {
      glGenBuffers(1, &meshdata.mVboTexCoords);
      glBindBuffer(GL_ARRAY_BUFFER, meshdata.mVboTexCoords);
      glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * totalNumVerts, 0, GL_STATIC_DRAW);

      int offset = 0;
      for (int m = 0; m<numSubmeshes; m++)
      {
         aiMesh* mesh = meshdata.mScene->mMeshes[m];
         std::vector<float> tex_coords(2 * mesh->mNumVertices);
         if (mesh->HasTextureCoords(0))
         {
            for (unsigned int k = 0; k < mesh->mNumVertices; ++k)
            {
               tex_coords[k * 2] = mesh->mTextureCoords[0][k].x;
               tex_coords[k * 2 + 1] = mesh->mTextureCoords[0][k].y;
            }
            const int size = 2 * sizeof(float)*mesh->mNumVertices;
            const void* data = tex_coords.data();
            glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
            offset += size;
         }
      }
      glEnableVertexAttribArray(tex_coord_loc);
      glVertexAttribPointer(tex_coord_loc, 2, GL_FLOAT, 0, 0, 0);
   }

   //buffer normals
   {
      glGenBuffers(1, &meshdata.mVboNormals);
      glBindBuffer(GL_ARRAY_BUFFER, meshdata.mVboNormals);
      glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * totalNumVerts, 0, GL_STATIC_DRAW);

      int offset = 0;
      for (int m = 0; m<numSubmeshes; m++)
      {
         aiMesh* mesh = meshdata.mScene->mMeshes[m];
         std::vector<float> normals(3 * mesh->mNumVertices);
         if (mesh->HasNormals())
         {
            for (unsigned int k = 0; k < mesh->mNumVertices; ++k)
            {
               normals[k * 3] = mesh->mNormals[k].x;
               normals[k * 3 + 1] = mesh->mNormals[k].y;
               normals[k * 3 + 2] = mesh->mNormals[k].z;
            }
         }
         const int size = 3 * sizeof(float)*mesh->mNumVertices;
         const void* data = normals.data();
         glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
         offset += size;
      }
      glEnableVertexAttribArray(normal_loc);
      glVertexAttribPointer(normal_loc, 3, GL_FLOAT, 0, 0, 0);
   }

   glBindVertexArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void SubmeshData::DrawSubmesh()
{
   glDrawElementsBaseVertex(GL_TRIANGLES, mNumIndices, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int)*mBaseIndex), mBaseVertex);
}

void MeshData::DrawMesh()
{
   for (int m = 0; m < mSubmesh.size(); m++)
   {
      mSubmesh[m].DrawSubmesh();
   }
}
