#pragma once

#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Resource/ResourceCache.h>

using namespace Urho3D;

struct ObjectData;
struct FaceData;
struct TextureList;

/// Struct to help generate vertices and indices from a group of objects
struct PodGeometryGen
{
    struct Vertex
    {
        Vector3 pos;
        Vector3 normal;
    };

    explicit PodGeometryGen(Context* ctx);

    void GenerateDataFromFace(const FaceData& face);

    void SetUpdatingTexCoords(bool b);

    SharedPtr<Geometry> Commit();

    void GetBounds(Vector3& min, Vector3& max) { min = bmin_; max = bmax_; }

    Vector3 bmin_, bmax_;
    PODVector<VertexElement> elements_;
    SharedPtr<VertexBuffer> vertexBuffer_;
    SharedPtr<VertexBuffer> uvBuffer_;
    SharedPtr<IndexBuffer> indexBuffer_;
    SharedPtr<Geometry> geometry_;
    Vector<Vertex> vertices_;
    Vector<Vector2> uvs_;
    PODVector<unsigned short> indices_;
    unsigned int indexOffset_ = 0;
    bool updatingUv_ = false;
    unsigned int updatingUvIdx_ = 0;
};

struct PodModelGen
{
    struct MaterialFaceList
    {
        Vector<FaceData*> faces;
    };

    explicit PodModelGen(Context* ctx, const TextureList& list);

    void AddObject(const ObjectData& obj);

    void SetBounds(const Vector3& min, const Vector3& max);

    void UpdateTexCoords();

    void InvalidateTexCoords() { uvDirty_ = true; }

    SharedPtr<Model> GetModel();

    const Vector<SharedPtr<Material>>& GetMaterials() { return materials_; }

private:
    Context* context_;
    const TextureList* textureList_;
    HashMap<unsigned int, MaterialFaceList> textureMaterials_;
    HashMap<unsigned int, MaterialFaceList> colorMaterials_;
    Vector<SharedPtr<Material>> materials_;
    Vector<UniquePtr<PodGeometryGen>> textureGeometries_;
    Vector<UniquePtr<PodGeometryGen>> colorGeometries_;
    SharedPtr<Model> model_;
    Vector3 bmin_, bmax_;
    bool calcBounds_;
    bool uvDirty_ = false;
};