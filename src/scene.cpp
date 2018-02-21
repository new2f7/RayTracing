#include "scene.hpp"
#include "mathlib.hpp"
#include "render.hpp"
#include "exception.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>
#include <unordered_map>
#include <cctype>
#include <gl/GL.h>

class FileReader
{
public:
    FileReader(const char* filename) : m_InputFile(filename), m_VectorValue(0.0f)
    {
        if (!m_InputFile)
        {
            std::cerr << "Failed to load " << filename << std::endl;
        }
    }

    std::string GetStringValue() const
    {
        return m_StringValue;
    }

    float3 GetVectorValue() const
    {
        return m_VectorValue;
    }

    float3 GetFloatValue() const
    {
        return m_VectorValue;
    }

protected:
    float ReadFloatValue()
    {
        SkipSpaces();
        float value = 0.0f;
        bool minus = m_CurrentLine[m_CurrentChar] == '-';
        if (minus) ++m_CurrentChar;
        while (m_CurrentChar < m_CurrentLine.size() && m_CurrentLine[m_CurrentChar] >= '0' && m_CurrentLine[m_CurrentChar] <= '9')
        {
            value = value * 10.0f + (float)((int)m_CurrentLine[m_CurrentChar++] - 48);
        }
        if (m_CurrentLine[m_CurrentChar++] == '.')
        {
            size_t frac = 1;
            while (m_CurrentChar < m_CurrentLine.size() && m_CurrentLine[m_CurrentChar] >= '0' && m_CurrentLine[m_CurrentChar] <= '9')
            {
                value += (float)((int)m_CurrentLine[m_CurrentChar++] - 48) / (std::powf(10.0f, frac++));
            }
        }

        m_FloatValue = minus ? -value : value;
        return m_FloatValue;
        
    }

    int ReadIntValue()
    {
        SkipSpaces();
        int value = 0;
        bool minus = m_CurrentLine[m_CurrentChar] == '-';
        if (minus) ++m_CurrentChar;

        while (m_CurrentChar < m_CurrentLine.size() && m_CurrentLine[m_CurrentChar] >= '0' && m_CurrentLine[m_CurrentChar] <= '9')
        {
            value = value * 10 + ((int)m_CurrentLine[m_CurrentChar++] - 48);
        }
        return minus ? -value : value;
    }

    void ReadVectorValue()
    {
        m_VectorValue.x = ReadFloatValue();
        m_VectorValue.y = ReadFloatValue();
        m_VectorValue.z = ReadFloatValue();

    }

    void ReadStringValue()
    {
        SkipSpaces();
        m_StringValue.clear();

        if (m_CurrentChar < m_CurrentLine.size() && IsIdentifierStart(m_CurrentLine[m_CurrentChar]))
        {
            m_StringValue += m_CurrentLine[m_CurrentChar++];
            while (m_CurrentChar < m_CurrentLine.size() && IsIdentifierBody(m_CurrentLine[m_CurrentChar]))
            {
                m_StringValue += m_CurrentLine[m_CurrentChar++];
            }
        }

    }

    bool ReadLine()
    {
        do
        {
            if (!std::getline(m_InputFile, m_CurrentLine))
            {
                return false;
            }
        } while (m_CurrentLine.size() == 0);

        m_CurrentChar = 0;
        return true;
    }

    bool IsIdentifierStart(char symbol)
    {
        return symbol >= 'a' && symbol <= 'z' ||
            symbol >= 'A' && symbol <= 'Z' || symbol == '_';
    }

    bool IsIdentifierBody(char symbol)
    {
        return IsIdentifierStart(symbol) || symbol >= '0' && symbol <= '9';
    }

    void SkipSpaces()
    {
        while (std::isspace(m_CurrentLine[m_CurrentChar])) { m_CurrentChar++; }
    }

    void SkipSymbol(char symbol)
    {
        SkipSpaces();
        while (m_CurrentLine[m_CurrentChar] == symbol) { m_CurrentChar++; }
    }

private:
    std::ifstream m_InputFile;
    std::string m_CurrentLine;
    std::string m_StringValue;
    float3 m_VectorValue;
    float m_FloatValue;
    size_t m_CurrentChar;

};

class MtlReader : public FileReader
{
public:
    enum MtlToken_t
    {
        MTL_MTLNAME,
        MTL_DIFFUSE,
        MTL_SPECULAR,
        MTL_EMISSION,
        MTL_INVALID,
        MTL_EOF
    };

    MtlReader(const char* filename) : FileReader(filename)
    {
        std::cout << "Loading material library " << filename << std::endl;
    }

    MtlToken_t NextToken()
    {
        if (!ReadLine())
        {
            return MTL_EOF;
        }
        
        ReadStringValue();
        if (GetStringValue() == "newmtl")
        {
            ReadStringValue();
            return MTL_MTLNAME;
        }
        else if (GetStringValue() == "Kd")
        {
            ReadVectorValue();
            return MTL_DIFFUSE;
        }
        else if (GetStringValue() == "Ke")
        {
            ReadVectorValue();
            return MTL_EMISSION;
        }
        else if (GetStringValue() == "Ns")
        {
            ReadFloatValue();
            return MTL_SPECULAR;
        }
        else
        {
            return MTL_INVALID;
        }

    }
    
};

class ObjReader : public FileReader
{
public:
    enum ObjToken_t
    {
        OBJ_MTLLIB,
        OBJ_USEMTL,
        OBJ_POSITION,
        OBJ_TEXCOORD,
        OBJ_NORMAL,
        OBJ_FACE,
        OBJ_INVALID,
        OBJ_EOF
    };

    ObjReader(const char* filename) : FileReader(filename)
    {
        std::cout << "Loading object file " << filename << std::endl;
        
    }

    ObjToken_t NextToken()
    {
        if (!ReadLine())
        {
            return OBJ_EOF;
        }

        ReadStringValue();
        if (GetStringValue() == "mtllib")
        {
            ReadStringValue();
            return OBJ_MTLLIB;
        }
        else if (GetStringValue() == "usemtl")
        {
            ReadStringValue();
            return OBJ_USEMTL;
        }
        else if (GetStringValue() == "v")
        {
            ReadVectorValue();
            return OBJ_POSITION;
        }
        else if (GetStringValue() == "vn")
        {
            ReadVectorValue();
            return OBJ_NORMAL;
        }
        else if (GetStringValue() == "vt")
        {
            ReadVectorValue();
            return OBJ_TEXCOORD;
        }
        else if (GetStringValue() == "f")
        {
            for (size_t i = 0; i < 3; ++i)
            {
                m_VertexIndices[i] = ReadIntValue() - 1;
                SkipSymbol('/');
                m_TexcoordIndices[i] = ReadIntValue() - 1;
                SkipSymbol('/');
                m_NormalIndices[i] = ReadIntValue() - 1;

            }
            return OBJ_FACE;
        }
        else
        {
            return OBJ_INVALID;
        }
    }
    
    void GetFaceIndices(unsigned int** iv, unsigned int** it, unsigned int** in)
    {
        *iv = m_VertexIndices;
        *it = m_TexcoordIndices;
        *in = m_NormalIndices;
    }

private:
    // Face indices
    unsigned int m_VertexIndices[3];
    unsigned int m_TexcoordIndices[3];
    unsigned int m_NormalIndices[3];

};

Scene::Scene(const char* filename)
{
    LoadTriangles(filename);
}

const std::vector<Triangle>& Scene::GetTriangles() const
{
    return m_Triangles;
}

void Scene::LoadMtlFile(const char* filename)
{
    MtlReader mtlReader(filename);

    MtlReader::MtlToken_t token;
    Material currentMaterial;
    std::string materialName;
    while ((token = mtlReader.NextToken()) != MtlReader::MTL_EOF)
    {
        switch (token)
        {
        case MtlReader::MTL_MTLNAME:
            materialName = mtlReader.GetStringValue();
            break;
        case MtlReader::MTL_DIFFUSE:
            currentMaterial.diffuse = mtlReader.GetVectorValue();
            break;
        case MtlReader::MTL_SPECULAR:
            currentMaterial.specular = mtlReader.GetVectorValue();
            break;
        case MtlReader::MTL_EMISSION:
            currentMaterial.emission = mtlReader.GetVectorValue();
            m_Materials[materialName] = currentMaterial;
            break;
        }
    }
}

//void ComputeTangentSpace(Vertex& v1, Vertex& v2, Vertex& v3)
//{
//    const float3& v1p = v1.position;
//    const float3& v2p = v2.position;
//    const float3& v3p = v3.position;
//
//    const float2& v1t = v1.texcoord;
//    const float2& v2t = v2.texcoord;
//    const float2& v3t = v3.texcoord;
//
//    double x1 = v2p.x - v1p.x;
//    double x2 = v3p.x - v1p.x;
//    double y1 = v2p.y - v1p.y;
//    double y2 = v3p.y - v1p.y;
//    double z1 = v2p.z - v1p.z;
//    double z2 = v3p.z - v1p.z;
//
//    double s1 = v2t.x - v1t.x;
//    double s2 = v3t.x - v1t.x;
//    double t1 = v2t.y - v1t.y;
//    double t2 = v3t.y - v1t.y;
//
//    double r = 1.0 / (s1 * t2 - s2 * t1);
//    float3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
//    float3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);
//
//    v1.tangent_s += sdir;
//    v2.tangent_s += sdir;
//    v3.tangent_s += sdir;
//
//    v1.tangent_t += tdir;
//    v2.tangent_t += tdir;
//    v3.tangent_t += tdir;
//
//}

void Scene::LoadTriangles(const char* filename)
{
    ObjReader objReader(filename);

    std::vector<float3> positions;
    std::vector<float3> normals;
    std::vector<float2> texcoords;

    Material currentMaterial;

    ObjReader::ObjToken_t token;
    while ((token = objReader.NextToken()) != ObjReader::OBJ_EOF)
    {
        switch (token)
        {
        case ObjReader::OBJ_MTLLIB:
            LoadMtlFile(("meshes/" + objReader.GetStringValue() + ".mtl").c_str());
            break;
        case ObjReader::OBJ_POSITION:
            positions.push_back(objReader.GetVectorValue());
            // Union bounds
            break;
        case ObjReader::OBJ_TEXCOORD:
            texcoords.push_back(float2(objReader.GetVectorValue().x, 1.0f - objReader.GetVectorValue().y));
            break;
        case ObjReader::OBJ_NORMAL:
            normals.push_back(objReader.GetVectorValue());
            break;
        case ObjReader::OBJ_USEMTL:
            currentMaterial = m_Materials[objReader.GetStringValue()];
            break;
        case ObjReader::OBJ_FACE:
            unsigned int *iv, *it, *in;
            objReader.GetFaceIndices(&iv, &it, &in);
            m_Triangles.push_back(Triangle(
                Vertex(positions[iv[0]], texcoords[it[0]], normals[in[0]]),
                Vertex(positions[iv[1]], texcoords[it[1]], normals[in[1]]),
                Vertex(positions[iv[2]], texcoords[it[2]], normals[in[2]])
                ));
            break;
        }
    }
    
    std::cout << "Load succesful (" << m_Triangles.size() << " m_Triangles)" << std::endl;

}

UniformGridScene::UniformGridScene(const char* filename)
    : Scene(filename)
{
    CreateGrid(64);
}

void UniformGridScene::SetupBuffers()
{
    cl_int errCode;

    m_TriangleBuffer = cl::Buffer(render->GetCLContext()->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, m_Triangles.size() * sizeof(Triangle), m_Triangles.data(), &errCode);
    if (errCode)
    {
        throw CLException("Failed to create scene buffer", errCode);
    }
    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::BUFFER_SCENE, &m_TriangleBuffer, sizeof(cl::Buffer));

    m_IndexBuffer = cl::Buffer(render->GetCLContext()->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, m_Indices.size() * sizeof(unsigned int), m_Indices.data(), &errCode);
    if (errCode)
    {
        throw CLException("Failed to create index buffer", errCode);
    }
    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::BUFFER_INDEX, &m_IndexBuffer, sizeof(cl::Buffer));

    m_CellBuffer = cl::Buffer(render->GetCLContext()->GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, m_Cells.size() * sizeof(CellData), m_Cells.data(), &errCode);
    if (errCode)
    {
        throw CLException("Failed to create cell buffer", errCode);
    }
    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::BUFFER_CELL, &m_CellBuffer, sizeof(cl::Buffer));

}

void UniformGridScene::DrawDebug()
{
    // XYZ
    glColor3f(1, 0, 0);
    glLineWidth(4);
    glBegin(GL_LINE_LOOP);
    glColor3f(1, 0, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(200, 0, 0);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glColor3f(0, 1, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 200, 0);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glColor3f(0, 0, 1);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 200);
    glEnd();
    glColor3f(1, 0, 0);
    glLineWidth(2);

    glBegin(GL_LINES);
    for (unsigned int z = 0; z < 8; ++z)
    for (unsigned int x = 0; x < 8; ++x)
    {
        glVertex3f(x * 8, 0, z * 8);
        glVertex3f(x * 8, 64, z * 8);
    }

    for (unsigned int z = 0; z < 8; ++z)
        for (unsigned int y = 0; y < 8; ++y)
        {
            glVertex3f(0, y * 8, z * 8);
            glVertex3f(64, y * 8, z * 8);
        }

    for (unsigned int x = 0; x < 8; ++x)
        for (unsigned int y = 0; y < 8; ++y)
        {
            glVertex3f(x * 8, y * 8, 0);
            glVertex3f(x * 8, y * 8, 64);
        }

    glEnd();
}

void UniformGridScene::CreateGrid(unsigned int cellResolution)
{
    std::vector<unsigned int> lower_indices;
    std::vector<CellData> lower_cells;
    clock_t t = clock();
    unsigned int resolution = 2;
    Bounds3 sceneBounds(0.0f, 64.0f);
    std::cout << "Creating uniform grid" << std::endl;
    std::cout << "Scene bounding box min: " << sceneBounds.min << " max: " << sceneBounds.max << std::endl;
    std::cout << "Resolution: ";
    while (resolution <= cellResolution)
    {
        std::cout << resolution << "... ";
        float InvResolution = 1.0f / static_cast<float>(resolution);
        float3 dv = (sceneBounds.max - sceneBounds.min) / static_cast<float>(resolution);
        CellData current_cell = { 0, 0 };

        lower_indices = m_Indices;
        lower_cells = m_Cells;
        m_Indices.clear();
        m_Cells.clear();

        int totalCells = resolution * resolution * resolution;
        for (unsigned int z = 0; z < resolution; ++z)
            for (unsigned int y = 0; y < resolution; ++y)
                for (unsigned int x = 0; x < resolution; ++x)
                {
                    Bounds3 cellBound(sceneBounds.min + float3(x*dv.x, y*dv.y, z*dv.z), sceneBounds.min + float3((x + 1)*dv.x, (y + 1)*dv.y, (z + 1)*dv.z));

                    current_cell.start_index = m_Indices.size();

                    if (resolution == 2)
                    {
                        for (size_t i = 0; i < GetTriangles().size(); ++i)
                        {
                            if (cellBound.Intersects(GetTriangles()[i]))
                            {
                                m_Indices.push_back(i);
                            }
                        }
                    }
                    else
                    {
                        CellData lower_cell = lower_cells[x / 2 + (y / 2) * (resolution / 2) + (z / 2) * (resolution / 2) * (resolution / 2)];
                        for (int i = lower_cell.start_index; i < lower_cell.start_index + lower_cell.count; ++i)
                        {
                            if (cellBound.Intersects(GetTriangles()[lower_indices[i]]))
                            {
                                m_Indices.push_back(lower_indices[i]);
                            }
                        }
                    }

                    current_cell.count = m_Indices.size() - current_cell.start_index;
                    m_Cells.push_back(current_cell);
                }
        resolution *= 2;
    }

    std::cout << "Done (" << (clock() - t) << " ms elapsed)" << std::endl;

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

unsigned int g_LeafNodes = 0;
unsigned int g_InteriorNodes = 0;

struct BVHBuildNode
{
    void InitLeaf(int first, int n, const Bounds3 &b)
    {
        firstPrimOffset = first;
        nPrimitives = n;
        bounds = b;
        children[0] = children[1] = nullptr;
        ++g_LeafNodes;
//        ++totalLeafNodes;
//        totalPrimitives += n;
    }
    void InitInterior(int axis, BVHBuildNode *c0, BVHBuildNode *c1)
    {
        children[0] = c0;
        children[1] = c1;
        bounds = Union(c0->bounds, c1->bounds);
        splitAxis = axis;
        nPrimitives = 0;
        ++g_InteriorNodes;
    }

    Bounds3 bounds;
    BVHBuildNode *children[2];
    int splitAxis, firstPrimOffset, nPrimitives;

};

struct BucketInfo {
    int count = 0;
    Bounds3 bounds;
};

BVHScene::BVHScene(const char* filename, int maxPrimitivesInNode)
    : Scene(filename), m_MaxPrimitivesInNode(maxPrimitivesInNode)
{
    std::vector<BVHPrimitiveInfo> primitiveInfo(m_Triangles.size());
    for (size_t i = 0; i < m_Triangles.size(); ++i)
    {
        primitiveInfo[i] = { i, m_Triangles[i].GetBounds() };
    }

    int totalNodes = 0;
    std::vector<Triangle> orderedTriangles;
    m_Root = RecursiveBuild(primitiveInfo, 0, m_Triangles.size(), &totalNodes, orderedTriangles);
    m_Triangles.swap(orderedTriangles);

    //primitiveInfo.resize(0);
    std::cout << "BVH created with " << totalNodes << " nodes for " << (int)m_Triangles.size()
        << " triangles ("<< float(totalNodes * sizeof(BVHBuildNode)) / (1024.f * 1024.f) << " MB)" << std::endl;

    // Compute representation of depth-first traversal of BVH tree
    m_Nodes.resize(totalNodes);
    int offset = 0;
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
    
}

void DrawTree(BVHBuildNode* node, float x, float y, int depth)
{
    float size_x = 0.025;
    float size_y = 0.03;
    if (node->children[0])
    {
        DrawTree(node->children[0], x - 1.0f / y, y + 1, depth * 2);
    }

    if (node->nPrimitives == 0)
        glColor3f(1, 0, 0);
    else
        glColor3f(0, 1, 0);
    
    glRectf(x / 4 - size_x, 1 - y / 8 - size_y, x / 4 + size_x, 1 - y / 8 + size_y);

    if (node->children[1])
    {
        DrawTree(node->children[1], x + 1.0f / y, y + 1, depth * 2);
    }
}

void BVHScene::DrawDebug()
{
    // XYZ
    glColor3f(1, 0, 0);
    glLineWidth(4);
    glBegin(GL_LINE_LOOP);
    glColor3f(1, 0, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(200, 0, 0);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glColor3f(0, 1, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 200, 0);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glColor3f(0, 0, 1);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 200);
    glEnd();
    glColor3f(1, 0, 0);
    glLineWidth(1);

    glBegin(GL_LINES);
    for (unsigned int i = 0; i < m_Nodes.size(); ++i)
    {
        glVertex3f(m_Nodes[i].bounds.min.x, m_Nodes[i].bounds.min.y, m_Nodes[i].bounds.min.z);
        glVertex3f(m_Nodes[i].bounds.max.x, m_Nodes[i].bounds.min.y, m_Nodes[i].bounds.min.z);

        glVertex3f(m_Nodes[i].bounds.min.x, m_Nodes[i].bounds.min.y, m_Nodes[i].bounds.min.z);
        glVertex3f(m_Nodes[i].bounds.min.x, m_Nodes[i].bounds.max.y, m_Nodes[i].bounds.min.z);

        glVertex3f(m_Nodes[i].bounds.min.x, m_Nodes[i].bounds.min.y, m_Nodes[i].bounds.min.z);
        glVertex3f(m_Nodes[i].bounds.min.x, m_Nodes[i].bounds.min.y, m_Nodes[i].bounds.max.z);
        
        glVertex3f(m_Nodes[i].bounds.min.x, m_Nodes[i].bounds.min.y, m_Nodes[i].bounds.max.z);
        glVertex3f(m_Nodes[i].bounds.max.x, m_Nodes[i].bounds.min.y, m_Nodes[i].bounds.max.z);
                
        glVertex3f(m_Nodes[i].bounds.min.x, m_Nodes[i].bounds.min.y, m_Nodes[i].bounds.max.z);
        glVertex3f(m_Nodes[i].bounds.min.x, m_Nodes[i].bounds.max.y, m_Nodes[i].bounds.max.z);

        glVertex3f(m_Nodes[i].bounds.min.x, m_Nodes[i].bounds.max.y, m_Nodes[i].bounds.min.z);
        glVertex3f(m_Nodes[i].bounds.min.x, m_Nodes[i].bounds.max.y, m_Nodes[i].bounds.max.z);
                
        glVertex3f(m_Nodes[i].bounds.min.x, m_Nodes[i].bounds.max.y, m_Nodes[i].bounds.min.z);
        glVertex3f(m_Nodes[i].bounds.max.x, m_Nodes[i].bounds.max.y, m_Nodes[i].bounds.min.z);
        
        glVertex3f(m_Nodes[i].bounds.max.x, m_Nodes[i].bounds.min.y, m_Nodes[i].bounds.min.z);
        glVertex3f(m_Nodes[i].bounds.max.x, m_Nodes[i].bounds.max.y, m_Nodes[i].bounds.min.z);

        glVertex3f(m_Nodes[i].bounds.max.x, m_Nodes[i].bounds.min.y, m_Nodes[i].bounds.min.z);
        glVertex3f(m_Nodes[i].bounds.max.x, m_Nodes[i].bounds.min.y, m_Nodes[i].bounds.max.z);
        
        glVertex3f(m_Nodes[i].bounds.max.x, m_Nodes[i].bounds.max.y, m_Nodes[i].bounds.min.z);
        glVertex3f(m_Nodes[i].bounds.max.x, m_Nodes[i].bounds.max.y, m_Nodes[i].bounds.max.z);
        
        glVertex3f(m_Nodes[i].bounds.max.x, m_Nodes[i].bounds.min.y, m_Nodes[i].bounds.max.z);
        glVertex3f(m_Nodes[i].bounds.max.x, m_Nodes[i].bounds.max.y, m_Nodes[i].bounds.max.z);
        
        glVertex3f(m_Nodes[i].bounds.min.x, m_Nodes[i].bounds.max.y, m_Nodes[i].bounds.max.z);
        glVertex3f(m_Nodes[i].bounds.max.x, m_Nodes[i].bounds.max.y, m_Nodes[i].bounds.max.z);
    }
    glEnd();


    //DrawTree(m_Root, 0.0f, 1.0f, 1);

}

BVHBuildNode *BVHScene::RecursiveBuild(
    std::vector<BVHPrimitiveInfo> &primitiveInfo,
    int start,
    int end, int *totalNodes,
    std::vector<Triangle> &orderedTriangles)
{
    assert(start <= end);

    // TODO: Add memory pool
    BVHBuildNode *node = new BVHBuildNode;
    (*totalNodes)++;
    // Compute bounds of all primitives in BVH node
    Bounds3 bounds;
    for (int i = start; i < end; ++i)
    {
        bounds = Union(bounds, primitiveInfo[i].bounds);
    }
    int nPrimitives = end - start;
    if (nPrimitives == 1)
    {
        // Create leaf BVHBuildNode
        int firstPrimOffset = orderedTriangles.size();
        for (int i = start; i < end; ++i)
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
        for (int i = start; i < end; ++i)
        {
            centroidBounds = Union(centroidBounds, primitiveInfo[i].centroid);
        }
        int dim = centroidBounds.MaximumExtent();

        // Partition primitives into two sets and build children
        int mid = (start + end) / 2;
        if (centroidBounds.max[dim] == centroidBounds.min[dim])
        {
            // Create leaf _BVHBuildNode_
            int firstPrimOffset = orderedTriangles.size();
            for (int i = start; i < end; ++i)
            {
                int primNum = primitiveInfo[i].primitiveNumber;
                orderedTriangles.push_back(m_Triangles[primNum]);
            }
            node->InitLeaf(firstPrimOffset, nPrimitives, bounds);
            return node;
        }
        else
        {
            // Partition primitives using approximate SAH
            if (nPrimitives <= 2)
            {
                // Partition primitives into equally-sized subsets
                mid = (start + end) / 2;
                std::nth_element(&primitiveInfo[start], &primitiveInfo[mid], &primitiveInfo[end - 1] + 1,
                    [dim](const BVHPrimitiveInfo &a, const BVHPrimitiveInfo &b) {
                    return a.centroid[dim] < b.centroid[dim];
                });
            }
            else
            {
                // Allocate _BucketInfo_ for SAH partition buckets
                const int nBuckets = 12;
                BucketInfo buckets[nBuckets];

                // Initialize _BucketInfo_ for SAH partition buckets
                for (int i = start; i < end; ++i)
                {
                    int b = nBuckets * centroidBounds.Offset(primitiveInfo[i].centroid)[dim];
                    if (b == nBuckets) b = nBuckets - 1;
                    assert(b >= 0 && b < nBuckets);
                    buckets[b].count++;
                    buckets[b].bounds = Union(buckets[b].bounds, primitiveInfo[i].bounds);
                }

                // Compute costs for splitting after each bucket
                float cost[nBuckets - 1];
                for (int i = 0; i < nBuckets - 1; ++i)
                {
                    Bounds3 b0, b1;
                    int count0 = 0, count1 = 0;
                    for (int j = 0; j <= i; ++j)
                    {
                        b0 = Union(b0, buckets[j].bounds);
                        count0 += buckets[j].count;
                    }
                    for (int j = i + 1; j < nBuckets; ++j)
                    {
                        b1 = Union(b1, buckets[j].bounds);
                        count1 += buckets[j].count;
                    }
                    cost[i] = 1 + (count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea()) / bounds.SurfaceArea();
                }

                // Find bucket to split at that minimizes SAH metric
                float minCost = cost[0];
                int minCostSplitBucket = 0;
                for (int i = 1; i < nBuckets - 1; ++i)
                {
                    if (cost[i] < minCost)
                    {
                        minCost = cost[i];
                        minCostSplitBucket = i;
                    }
                }

                // Either create leaf or split primitives at selected SAH
                // bucket
                float leafCost = nPrimitives;
                if (nPrimitives > m_MaxPrimitivesInNode || minCost < leafCost)
                {
                    BVHPrimitiveInfo *pmid = std::partition(
                        &primitiveInfo[start], &primitiveInfo[end - 1] + 1,
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
                    // Create leaf _BVHBuildNode_
                    int firstPrimOffset = orderedTriangles.size();
                    for (int i = start; i < end; ++i)
                    {
                        int primNum = primitiveInfo[i].primitiveNumber;
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

int BVHScene::FlattenBVHTree(BVHBuildNode *node, int *offset)
{
    LinearBVHNode *linearNode = &m_Nodes[*offset];
    linearNode->bounds = node->bounds;
    int myOffset = (*offset)++;
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
