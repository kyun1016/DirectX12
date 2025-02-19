#ifndef LOADM3D_H
#define LOADM3D_H

#include "SkinnedData.h"
#include "GeometryGenerator.h"

class M3DLoader
{
public:
    M3DLoader() = delete;

    struct Subset
    {
        UINT Id = -1;
        UINT VertexStart = 0;
        UINT VertexCount = 0;
        UINT FaceStart = 0;
        UINT FaceCount = 0;
    };

    struct M3dMaterial
    {
        std::string Name;

        DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
        float Roughness = 0.8f;
        bool AlphaClip = false;

        std::string MaterialTypeName;
        std::string DiffuseMapName;
        std::string NormalMapName;
    };

    static bool LoadM3d(const std::string& filename,
        std::vector<GeometryGenerator::Vertex>& vertices,
        std::vector<std::uint32_t>& indices,
        std::vector<Subset>& subsets,
        std::vector<M3dMaterial>& mats);
    static bool LoadM3d(const std::string& filename,
        std::vector<GeometryGenerator::SkinnedVertex>& vertices,
        std::vector<std::uint32_t>& indices,
        std::vector<Subset>& subsets,
        std::vector<M3dMaterial>& mats,
        SkinnedData& skinInfo);

private:
    static void ReadMaterials(std::ifstream& fin, UINT numMaterials, std::vector<M3dMaterial>& mats);
    static void ReadSubsetTable(std::ifstream& fin, UINT numSubsets, std::vector<Subset>& subsets);
    static void ReadVertices(std::ifstream& fin, UINT numVertices, std::vector<GeometryGenerator::Vertex>& vertices);
    static void ReadSkinnedVertices(std::ifstream& fin, UINT numVertices, std::vector<GeometryGenerator::SkinnedVertex>& vertices);
    static void ReadTriangles(std::ifstream& fin, UINT numTriangles, std::vector<std::uint32_t>& indices);
    static void ReadBoneOffsets(std::ifstream& fin, UINT numBones, std::vector<DirectX::XMFLOAT4X4>& boneOffsets);
    static void ReadBoneHierarchy(std::ifstream& fin, UINT numBones, std::vector<int>& boneIndexToParentIndex);
    static void ReadAnimationClips(std::ifstream& fin, UINT numBones, UINT numAnimationClips, std::unordered_map<std::string, AnimationClip>& animations);
    static void ReadBoneKeyframes(std::ifstream& fin, UINT numBones, BoneAnimation& boneAnimation);
};



#endif // LOADM3D_H