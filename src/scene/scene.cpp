#include "scene.hpp"
#include "mathlib/mathlib.hpp"
#include "renderers/render.hpp"
#include "utils/cl_exception.hpp"
#include <algorithm>
#include <iostream>
#include <string>

Scene::Scene(const char* filename)
{
    LoadTriangles(filename);

}

void Scene::LoadTriangles(const char* filename)
{
    char mtlname[80];
    memset(mtlname, 0, 80);
    strncpy(mtlname, filename, strlen(filename) - 4);
    strcat(mtlname, ".mtl");

    LoadMaterials(mtlname);

    std::vector<float3> positions;
    std::vector<float3> normals;
    std::vector<float2> texcoords;

    std::cout << "Loading object file " << filename << std::endl;

    double startTime = render->GetCurtime();

    FILE* file = fopen(filename, "r");
    if (!file)
    {
        throw std::runtime_error("Failed to open scene file!");
    }
    
    unsigned int materialIndex = -1;

    while (true)
    {
        char lineHeader[128];
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF)
        {
            break;
        }
        if (strcmp(lineHeader, "v") == 0)
        {
            float3 vertex;
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
            positions.push_back(vertex);
        }
        else if (strcmp(lineHeader, "vt") == 0)
        {
            float2 uv;
            fscanf(file, "%f %f\n", &uv.x, &uv.y);
            texcoords.push_back(uv);
        }
        else if (strcmp(lineHeader, "vn") == 0)
        {
            float3 normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            normals.push_back(normal);
        }
        else if (strcmp(lineHeader, "usemtl") == 0)
        {
            char str[80];
            fscanf(file, "%s\n", str);
            for (unsigned int i = 0; i < m_MaterialNames.size(); ++i)
            {
                if (strcmp(str, m_MaterialNames[i].c_str()) == 0)
                {
                    materialIndex = i;
                    break;
                }
            }

        }
        else if (strcmp(lineHeader, "f") == 0)
        {
            unsigned int iv[3], it[3], in[3];
            int count = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &iv[0], &it[0], &in[0], &iv[1], &it[1], &in[1], &iv[2], &it[2], &in[2]);
            if (count != 9)
            {
                throw std::runtime_error("Failed to load face!");
            }
            m_Triangles.push_back(Triangle(
                Vertex(positions[iv[0] - 1], texcoords[it[0] - 1], normals[in[0] - 1]),
                Vertex(positions[iv[1] - 1], texcoords[it[1] - 1], normals[in[1] - 1]),
                Vertex(positions[iv[2] - 1], texcoords[it[2] - 1], normals[in[2] - 1]),
                materialIndex
                ));
        }
    }
    
    std::cout << "Load succesful (" << m_Triangles.size() << " triangles, " << render->GetCurtime() - startTime << "s elapsed)" << std::endl;

}

void Scene::LoadMaterials(const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        throw std::runtime_error("Failed to open material file!");
    }
    
    while (true)
    {
        char buf[128];
        int res = fscanf(file, "%s", buf);
        if (res == EOF)
        {
            break;
        }
        if (strcmp(buf, "newmtl") == 0)
        {
            char str[80];
            fscanf(file, "%s\n", str);
            m_MaterialNames.push_back(str);
            m_Materials.push_back(Material());
        }
        else if (strcmp(buf, "type") == 0)
        {
            fscanf(file, "%d\n", &m_Materials.back().type);
        }
        else if (strcmp(buf, "diff") == 0)
        {
            fscanf(file, "%f %f %f\n", &m_Materials.back().diffuse.x, &m_Materials.back().diffuse.y, &m_Materials.back().diffuse.z);
        }
        else if (strcmp(buf, "spec") == 0)
        {
            fscanf(file, "%f %f %f\n", &m_Materials.back().specular.x, &m_Materials.back().specular.y, &m_Materials.back().specular.z);
        }
        else if (strcmp(buf, "emit") == 0)
        {
            fscanf(file, "%f %f %f\n", &m_Materials.back().emission.x, &m_Materials.back().emission.y, &m_Materials.back().emission.z);
        }
        else if (strcmp(buf, "rough") == 0)
        {
            fscanf(file, "%f\n", &m_Materials.back().roughness);
        }
        else if (strcmp(buf, "ior") == 0)
        {
            fscanf(file, "%f\n", &m_Materials.back().ior);
        }
    }
    std::cout << "Material count: " << m_Materials.size() << std::endl;
}

struct BVHPrimitiveInfo
{
    BVHPrimitiveInfo() {}
    BVHPrimitiveInfo(unsigned int primitiveNumber, const Bounds3 &bounds)
        : primitiveNumber(primitiveNumber), bounds(bounds),
        centroid(bounds.min * 0.5f + bounds.max * 0.5f)
    {}

    unsigned int primitiveNumber;
    Bounds3 bounds;
    float3 centroid;

};

struct BVHBuildNode
{
    void InitLeaf(int first, int n, const Bounds3 &b)
    {
        firstPrimOffset = first;
        nPrimitives = n;
        bounds = b;
        children[0] = children[1] = nullptr;

    }

    void InitInterior(int axis, BVHBuildNode *c0, BVHBuildNode *c1)
    {
        children[0] = c0;
        children[1] = c1;
        bounds = Union(c0->bounds, c1->bounds);
        splitAxis = axis;
        nPrimitives = 0;

    }

    Bounds3 bounds;
    BVHBuildNode *children[2];
    int splitAxis, firstPrimOffset, nPrimitives;

};

struct BucketInfo
{
    int count = 0;
    Bounds3 bounds;
};

BVHScene::BVHScene(const char* filename, unsigned int maxPrimitivesInNode)
    : Scene(filename), m_MaxPrimitivesInNode(maxPrimitivesInNode)
{
    std::cout << "Building Bounding Volume Hierarchy for scene" << std::endl;

    double startTime = render->GetCurtime();
    std::vector<BVHPrimitiveInfo> primitiveInfo(m_Triangles.size());
    for (unsigned int i = 0; i < m_Triangles.size(); ++i)
    {
        primitiveInfo[i] = { i, m_Triangles[i].GetBounds() };
    }

    unsigned int totalNodes = 0;
    std::vector<Triangle> orderedTriangles;
    m_Root = RecursiveBuild(primitiveInfo, 0, m_Triangles.size(), &totalNodes, orderedTriangles);
    m_Triangles.swap(orderedTriangles);

    //primitiveInfo.resize(0);
    std::cout << "BVH created with " << totalNodes << " nodes for " << m_Triangles.size()
        << " triangles ("<< float(totalNodes * sizeof(BVHBuildNode)) / (1024.0f * 1024.0f) << " MB, " << render->GetCurtime() - startTime << "s elapsed)" << std::endl;

    // Compute representation of depth-first traversal of BVH tree
    m_Nodes.resize(totalNodes);
    unsigned int offset = 0;
    FlattenBVHTree(m_Root, &offset);
    assert(totalNodes == offset);

}

void BVHScene::SetupBuffers()
{
    cl_int errCode;

    m_TriangleBuffer = cl::Buffer(render->GetCLContext()->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, m_Triangles.size() * sizeof(Triangle), m_Triangles.data(), &errCode);
    if (errCode)
    {
        throw CLException("Failed to create scene buffer", errCode);
    }

    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::BUFFER_SCENE, &m_TriangleBuffer, sizeof(cl::Buffer));

    m_NodeBuffer = cl::Buffer(render->GetCLContext()->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, m_Nodes.size() * sizeof(LinearBVHNode), m_Nodes.data(), &errCode);
    if (errCode)
    {
        throw CLException("Failed to create BVH node buffer", errCode);
    }

    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::BUFFER_NODE, &m_NodeBuffer, sizeof(cl::Buffer));

    m_MaterialBuffer = cl::Buffer(render->GetCLContext()->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, m_Materials.size() * sizeof(Material), m_Materials.data(), &errCode);
    if (errCode)
    {
        throw CLException("Failed to create material buffer", errCode);
    }

    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::BUFFER_MATERIAL, &m_MaterialBuffer, sizeof(cl::Buffer));

    
}

BVHBuildNode* BVHScene::RecursiveBuild(
    std::vector<BVHPrimitiveInfo> &primitiveInfo,
    unsigned int start,
    unsigned int end, unsigned int *totalNodes,
    std::vector<Triangle> &orderedTriangles)
{
    assert(start <= end);

    // TODO: Add memory pool
    BVHBuildNode *node = new BVHBuildNode;
    (*totalNodes)++;

    // Compute bounds of all primitives in BVH node
    Bounds3 bounds;
    for (unsigned int i = start; i < end; ++i)
    {
        bounds = Union(bounds, primitiveInfo[i].bounds);
    }

    unsigned int nPrimitives = end - start;
    if (nPrimitives == 1)
    {
        // Create leaf
        int firstPrimOffset = orderedTriangles.size();
        for (unsigned int i = start; i < end; ++i)
        {
            int primNum = primitiveInfo[i].primitiveNumber;
            orderedTriangles.push_back(m_Triangles[primNum]);
        }
        node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
        return node;
    }
    else
    {
        // Compute bound of primitive centroids, choose split dimension
        Bounds3 centroidBounds;
        for (unsigned int i = start; i < end; ++i)
        {
            centroidBounds = Union(centroidBounds, primitiveInfo[i].centroid);
        }
        unsigned int dim = centroidBounds.MaximumExtent();

        // Partition primitives into two sets and build children
        unsigned int mid = (start + end) / 2;
        if (centroidBounds.max[dim] == centroidBounds.min[dim])
        {
            // Create leaf
            unsigned int firstPrimOffset = orderedTriangles.size();
            for (unsigned int i = start; i < end; ++i)
            {
                unsigned int primNum = primitiveInfo[i].primitiveNumber;
                orderedTriangles.push_back(m_Triangles[primNum]);
            }
            node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
            return node;
        }
        else
        {
            if (nPrimitives <= 2)
            {
                // Partition primitives into equally-sized subsets
                mid = (start + end) / 2;
                std::nth_element(&primitiveInfo[start], &primitiveInfo[mid], &primitiveInfo[end - 1] + 1,
                    [dim](const BVHPrimitiveInfo &a, const BVHPrimitiveInfo &b)
                    {
                        return a.centroid[dim] < b.centroid[dim];
                    });
            }
            else
            {
                // Partition primitives using approximate SAH
                const unsigned int nBuckets = 12;
                BucketInfo buckets[nBuckets];

                // Initialize _BucketInfo_ for SAH partition buckets
                for (unsigned int i = start; i < end; ++i)
                {
                    int b = nBuckets * centroidBounds.Offset(primitiveInfo[i].centroid)[dim];
                    if (b == nBuckets) b = nBuckets - 1;
                    assert(b >= 0 && b < nBuckets);
                    buckets[b].count++;
                    buckets[b].bounds = Union(buckets[b].bounds, primitiveInfo[i].bounds);
                }

                // Compute costs for splitting after each bucket
                float cost[nBuckets - 1];
                for (unsigned int i = 0; i < nBuckets - 1; ++i)
                {
                    Bounds3 b0, b1;
                    int count0 = 0, count1 = 0;
                    for (unsigned int j = 0; j <= i; ++j)
                    {
                        b0 = Union(b0, buckets[j].bounds);
                        count0 += buckets[j].count;
                    }
                    for (unsigned int j = i + 1; j < nBuckets; ++j)
                    {
                        b1 = Union(b1, buckets[j].bounds);
                        count1 += buckets[j].count;
                    }
                    cost[i] = 1.0f + (count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea()) / bounds.SurfaceArea();
                }

                // Find bucket to split at that minimizes SAH metric
                float minCost = cost[0];
                unsigned int minCostSplitBucket = 0;
                for (unsigned int i = 1; i < nBuckets - 1; ++i)
                {
                    if (cost[i] < minCost)
                    {
                        minCost = cost[i];
                        minCostSplitBucket = i;
                    }
                }

                // Either create leaf or split primitives at selected SAH bucket
                float leafCost = float(nPrimitives);
                if (nPrimitives > m_MaxPrimitivesInNode || minCost < leafCost)
                {
                    BVHPrimitiveInfo *pmid = std::partition( &primitiveInfo[start], &primitiveInfo[end - 1] + 1,
                        [=](const BVHPrimitiveInfo &pi)
                        {
                            int b = nBuckets * centroidBounds.Offset(pi.centroid)[dim];
                            if (b == nBuckets) b = nBuckets - 1;
                            assert(b >= 0 && b < nBuckets);
                            return b <= minCostSplitBucket;
                        });
                    mid = pmid - &primitiveInfo[0];
                }
                else
                {
                    // Create leaf
                    unsigned int firstPrimOffset = orderedTriangles.size();
                    for (unsigned int i = start; i < end; ++i)
                    {
                        unsigned int primNum = primitiveInfo[i].primitiveNumber;
                        orderedTriangles.push_back(m_Triangles[primNum]);
                    }
                    node->InitLeaf(firstPrimOffset, nPrimitives, bounds);

                    return node;
                }
            }

            node->InitInterior(dim,
                RecursiveBuild(primitiveInfo, start, mid,
                totalNodes, orderedTriangles),
                RecursiveBuild(primitiveInfo, mid, end,
                totalNodes, orderedTriangles));
        }
    }

    return node;
}

unsigned int BVHScene::FlattenBVHTree(BVHBuildNode *node, unsigned int *offset)
{
    LinearBVHNode *linearNode = &m_Nodes[*offset];
    linearNode->bounds = node->bounds;
    unsigned int myOffset = (*offset)++;
    if (node->nPrimitives > 0)
    {
        assert(!node->children[0] && !node->children[1]);
        assert(node->nPrimitives < 65536);
        linearNode->offset = node->firstPrimOffset;
        linearNode->nPrimitives = node->nPrimitives;
    }
    else
    {
        // Create interior flattened BVH node
        linearNode->axis = node->splitAxis;
        linearNode->nPrimitives = 0;
        FlattenBVHTree(node->children[0], offset);
        linearNode->offset = FlattenBVHTree(node->children[1], offset);
    }

    return myOffset;
}
