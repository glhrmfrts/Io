#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Resource/ResourceCache.h>
#include "PodModelGen.h"
#include "PodCommon.h"

PodGeometryGen::PodGeometryGen(Context* ctx)
    : vertexBuffer_(new VertexBuffer(ctx))
    , uvBuffer_(new VertexBuffer(ctx))
    , indexBuffer_(new IndexBuffer(ctx))
    , geometry_(new Geometry(ctx))
{
    elements_.Push(VertexElement(TYPE_VECTOR3, SEM_POSITION));
    elements_.Push(VertexElement(TYPE_VECTOR3, SEM_NORMAL));
    //elements.Push(VertexElement(TYPE_VECTOR2, SEM_TEXCOORD));
}

void PodGeometryGen::GenerateDataFromFace(const FaceData& face)
{
    struct FaceVertex { unsigned int idx; Vector2 uv; };
    static PODVector<FaceVertex> faceVerts;
    faceVerts.Clear();

    if (!face.IsVisible()) return;

    if (face.Vertices > 3)
    {
        // triangulate quad
        for (int j = 1; j < face.Vertices - 1; j++)
        {
            unsigned int idx1 = face.Indices[0];
            unsigned int idx2 = face.Indices[j];
            unsigned int idx3 = face.Indices[j + 1];
            Vector2 uv1, uv2, uv3;
            if (face.UseAnimTexCoord)
            {
                uv1 = face.AnimTexCoord[0];
                uv2 = face.AnimTexCoord[j];
                uv3 = face.AnimTexCoord[j+1];
            }
            else
            {
                uv1 = Vector2(face.TexCoord[0][0], face.TexCoord[0][1]);
                uv2 = Vector2(face.TexCoord[j][0], face.TexCoord[j][1]);
                uv3 = Vector2(face.TexCoord[j + 1][0], face.TexCoord[j + 1][1]);
            }
            faceVerts.Push({ idx1, uv1 });
            faceVerts.Push({ idx2, uv2 });
            faceVerts.Push({ idx3, uv3 });
        }
    }
    else
    {
        unsigned int idx1 = face.Indices[0];
        unsigned int idx2 = face.Indices[1];
        unsigned int idx3 = face.Indices[2];
        Vector2 uv1, uv2, uv3;
        if (face.UseAnimTexCoord)
        {
            uv1 = face.AnimTexCoord[0];
            uv2 = face.AnimTexCoord[1];
            uv3 = face.AnimTexCoord[2];
        }
        else
        {
            uv1 = Vector2(face.TexCoord[0][0], face.TexCoord[0][1]);
            uv2 = Vector2(face.TexCoord[1][0], face.TexCoord[1][1]);
            uv3 = Vector2(face.TexCoord[2][0], face.TexCoord[2][1]);
        }
        faceVerts.Push({ idx1, uv1 });
        faceVerts.Push({ idx2, uv2 });
        faceVerts.Push({ idx3, uv3 });
    }

    if (updatingUv_)
    {
        for (const auto& vert : faceVerts)
            uvs_.Push(vert.uv);
    }
    else
    {
        const fp1616_t* vdata = face.Obj->VertexArray.Get();
        const fp1616_t* normals = face.Obj->Normals.Get();
        for (const auto& vert : faceVerts)
        {
            // Build vertex
            Vertex v;
            v.pos.x_ = -FloatFromFP1616(vdata[vert.idx * 3 + 1]);
            v.pos.y_ = FloatFromFP1616(vdata[vert.idx * 3 + 2]);
            v.pos.z_ = FloatFromFP1616(vdata[vert.idx * 3 + 0]);
            v.normal.x_ = -FloatFromFP1616(normals[vert.idx * 3 + 1]);
            v.normal.y_ = FloatFromFP1616(normals[vert.idx * 3 + 2]);
            v.normal.z_ = FloatFromFP1616(normals[vert.idx * 3 + 0]);

            // Push our own index
            unsigned int vertIdx = vertices_.Size();
            indices_.Push(vertIdx);
            vertices_.Push(v);
            uvs_.Push(vert.uv);
        }
    }
}

void PodGeometryGen::SetUpdatingTexCoords(bool b)
{
    updatingUv_ = b;
    if (b)
    {
        updatingUvIdx_ = 0;
        uvs_.Clear();
    }
}

SharedPtr<Geometry> PodGeometryGen::Commit()
{
    if (updatingUv_)
    {
        uvBuffer_->SetData(uvs_.Buffer());
        return geometry_;
    }

    BoundingBox bounds;
    for (unsigned int i = 0; i < vertices_.Size(); i++)
    {
        const Vector3& pos = vertices_[i].pos;
        bounds.Merge(pos);
    }
    bmin_ = bounds.min_;
    bmax_ = bounds.max_;

    vertexBuffer_->SetShadowed(true);
    vertexBuffer_->SetSize(vertices_.Size(), elements_);
    vertexBuffer_->SetData(vertices_.Buffer());

    uvBuffer_->SetShadowed(true);
    uvBuffer_->SetSize(uvs_.Size(), { VertexElement(TYPE_VECTOR2, SEM_TEXCOORD) }, true);
    uvBuffer_->SetData(uvs_.Buffer());

    indexBuffer_->SetShadowed(true);
    indexBuffer_->SetSize(indices_.Size(), false);
    indexBuffer_->SetData(indices_.Buffer());

    geometry_->SetNumVertexBuffers(2);
    geometry_->SetVertexBuffer(0, vertexBuffer_);
    geometry_->SetVertexBuffer(1, uvBuffer_);
    geometry_->SetIndexBuffer(indexBuffer_);
    geometry_->SetDrawRange(TRIANGLE_LIST, 0, indices_.Size());
    return geometry_;
}


PodModelGen::PodModelGen(Context* ctx, const TextureList& tl)
    : context_(ctx)
    , textureList_(&tl)
    , calcBounds_(true)
    , bmin_(Vector3(100000.0f, 100000.0f, 100000.0f))
    , bmax_(Vector3(-100000.0f, -100000.0f, -100000.0f))
{
}

void PodModelGen::AddObject(const ObjectData& obj)
{
    for (int fi = 0; fi < obj.FaceCount; fi++)
    {
        auto& face = obj.FaceData.Get()[fi];
        if (face.MaterialType == "GOURAUD" || face.MaterialType == "FLAT")
        {
            colorMaterials_[face.ColorOrTexIndex].faces.Push(&face);
            colorGeometries_.Push(UniquePtr<PodGeometryGen>(new PodGeometryGen(context_)));
        }
        else
        {
            textureMaterials_[face.ColorOrTexIndex].faces.Push(&face);
            textureGeometries_.Push(UniquePtr<PodGeometryGen>(new PodGeometryGen(context_)));
        }
    }
}

void PodModelGen::SetBounds(const Vector3& min, const Vector3& max)
{
    calcBounds_ = false;
    bmin_ = min;
    bmax_ = max;
}

void PodModelGen::UpdateTexCoords()
{
    if (!uvDirty_) return;

    int gi = 0;
    for (const auto& pair : textureMaterials_)
    {
        auto& faces = pair.second_.faces;
        auto& gen = *textureGeometries_[gi++];
        gen.SetUpdatingTexCoords(true);
        for (int i = 0; i < faces.Size(); i++)
            gen.GenerateDataFromFace(*faces[i]);
        gen.Commit();
    }

    gi = 0;
    for (const auto& pair : colorMaterials_)
    {
        auto& faces = pair.second_.faces;
        auto& gen = *colorGeometries_[gi++];
        gen.SetUpdatingTexCoords(true);
        for (int i = 0; i < faces.Size(); i++)
            gen.GenerateDataFromFace(*faces[i]);
        gen.Commit();
    }

    uvDirty_ = false;
}

SharedPtr<Model> PodModelGen::GetModel()
{
    if (model_) return model_;

    BoundingBox bounds = { };
    Vector<SharedPtr<Geometry>> geoms;
    int gi = 0;
    for (const auto& pair : textureMaterials_)
    {
        unsigned int texIndex = pair.first_;
        auto& faces = pair.second_.faces;

        PodGeometryGen& gen = *textureGeometries_[gi++];
        gen.SetUpdatingTexCoords(false);
        for (int i = 0; i < faces.Size(); i++)
        {
            gen.GenerateDataFromFace(*faces[i]);
        }
        geoms.Push(gen.Commit());
        if (calcBounds_)
        {
            gen.GetBounds(bmin_, bmax_);
            bounds.Merge({ bmin_, bmax_ });
        }

        {
            // Create material
            auto image = new Image(context_);
            auto& pixels = textureList_->PixelData.Get()[texIndex].Pixels;
            image->SetSize(textureList_->Width, textureList_->Height, 4);
            for (int y = 0; y < textureList_->Height; y++)
            {
                for (int x = 0; x < textureList_->Width; x++)
                {
                    image->SetPixel(x, y, ColorFromRGB565(pixels.Get()[y * textureList_->Width + x]));
                }
            }
            auto texture = new Texture2D(context_);
            texture->SetData(image);

            SharedPtr<Material> mat(new Material(context_));
            mat->SetCullMode(CULL_NONE);
            mat->SetTexture(TU_DIFFUSE, texture);
            mat->SetTechnique(0, context_->GetSubsystem<ResourceCache>()->GetResource<Technique>("Techniques/Diff.xml"));
            materials_.Push(mat);
        }
    }

    gi = 0;
    for (const auto& pair : colorMaterials_)
    {
        unsigned int color = pair.first_;
        auto& faces = pair.second_.faces;

        PodGeometryGen& gen = *colorGeometries_[gi++];
        gen.SetUpdatingTexCoords(false);
        for (int i = 0; i < faces.Size(); i++)
        {
            gen.GenerateDataFromFace(*faces[i]);
        }
        geoms.Push(gen.Commit());
        if (calcBounds_)
        {
            gen.GetBounds(bmin_, bmax_);
            bounds.Merge({ bmin_, bmax_ });
        }

        {
            Color c;
            c.r_ = (color >> 16) & 0xFF;
            c.g_ = (color >> 8) & 0xFF;
            c.b_ = (color) & 0xFF;
            SharedPtr<Material> mat(new Material(context_));
            mat->SetCullMode(CULL_NONE);
            mat->SetShaderParameter("MatDiffColor", c);
            mat->SetTechnique(0, context_->GetSubsystem<ResourceCache>()->GetResource<Technique>("Techniques/NoTexture.xml"));
            materials_.Push(mat);
        }
    }

    SharedPtr<Model> model(new Model(context_));
    model->SetNumGeometries(geoms.Size());
    
    for (int i = 0; i < geoms.Size(); i++)
    {
        model->SetGeometry(i, 0, geoms[i]);
    }

    if (calcBounds_)
    {
        model->SetBoundingBox(bounds);
        bmin_ = bounds.min_;
        bmax_ = bounds.max_;
    }
    else
    {
        model->SetBoundingBox({ bmin_, bmax_ });
    }

    model_ = model;
    return model_;
}

