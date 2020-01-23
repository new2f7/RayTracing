#ifndef SCENE_HPP
#define SCENE_HPP

#include "mathlib/mathlib.hpp"
#include "utils/shared_structs.hpp"
#include <CL/cl2.hpp>
#include <algorithm>
#include <vector>
#include <map>

class Scene
{
public:
    Scene(const char* filename);
    virtual void SetupBuffers() = 0;

private:
    void LoadTriangles(const char* filename);
    void LoadMaterials(const char* filename);
    std::vector<std::string> m_MaterialNames;

protected:
    std::vector<Triangle> m_Triangles;
    std::vector<Material> m_Materials;
    cl::Buffer m_TriangleBuffer;
    cl::Buffer m_MaterialBuffer;

};

struct BVHBuildNode;
struct BVHPrimitiveInfo;

class BVHScene : public Scene
{
public:
    BVHScene(const char* filename, unsigned int maxPrimitivesInNode);
    virtual void SetupBuffers();

private:
    BVHBuildNode* RecursiveBuild(
        std::vector<BVHPrimitiveInfo> &primitiveInfo,
        unsigned int start,
        unsigned int end, unsigned int *totalNodes,
        std::vector<Triangle> &orderedTriangles);

    unsigned int FlattenBVHTree(BVHBuildNode *node, unsigned int *offset);

private:
    std::vector<LinearBVHNode> m_Nodes;
    unsigned int m_MaxPrimitivesInNode;
    cl::Buffer m_NodeBuffer;
    BVHBuildNode* m_Root;

};

#endif // SCENE_HPP
