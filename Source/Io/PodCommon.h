#pragma once

#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/Deserializer.h>
#include <string>
#include "Application.h"
#include "Utils.h"

enum PodFilesOffset
{
    PodBv4Key = 0x00000F2E,
    PodBv4Len = 0x00001000
};

enum PodFileType
{
    FILE_VEHICLE,
    FILE_CIRCUIT,
};

enum PodFileFlags
{
    FLAG_NAMED_FACES = 1,
    FLAG_FACE_UNK_PROPERTY = (1 << 1),
    FLAG_OBJ_HAS_PRISM = (1 << 2),
};

typedef int fp1616_t;

struct ImageData
{
    char FileName[32];
    uint32_t Left;
    uint32_t Top;
    uint32_t Right;
    uint32_t Bottom;
    uint32_t ImageFlags; // 00000001 = Non-zero pixel(s)?
};

struct ImageList
{
    uint32_t Count;
    UniqueArray<ImageData> ImgData;
};

struct TexturePixelData
{
    UniqueArray<uint16_t> Pixels;  // RGB565
};

struct TextureList
{
    int Width, Height; // runtime

    uint32_t Count;
    uint32_t Flags;

    UniqueArray<ImageList> ImageList;
    UniqueArray<TexturePixelData> PixelData;
};

struct ObjectData;
struct FaceData;

struct FaceData
{
    bool IsVisible() const;
    
    String Name; // if (NamedFaces)
    uint32_t Vertices;  // 3..4
    uint32_t Indices[4];
    fp1616_t Normal[3];
    String MaterialType;
    uint32_t ColorOrTexIndex;
    float TexCoord[4][2];
    uint32_t Reserved1;
    uint32_t Reserved2; // Circuit only, if (Normal == VectorZero)
    fp1616_t QuadReserved[3]; // if (Vertices == 4)
    uint32_t Unknown; // (if HasUnk)
    uint32_t FaceProperties;
    ObjectData* Obj; // ptr to owner obj

    Vector2 AnimTexCoord[4];
    bool UseAnimTexCoord;
};

struct ObjectData
{
    uint32_t VertexCount;                   // uint16_t
    UniqueArray<fp1616_t> VertexArray;      // [VertexCount][3]
    uint32_t FaceCount;
    uint32_t TriangleCount;                 // used for alloc
    uint32_t QuadrangleCount;               // used for alloc
    UniqueArray<FaceData> FaceData;                     // [FaceCount]
    UniqueArray<fp1616_t> Normals;           // [VertexCount] [3]
    uint32_t Unknown;
    uint8_t Prism[28]; // Vehicle only

    PODVector<::FaceData*> TriFaces;
    PODVector<::FaceData*> QuadFaces;
};

class PodBdfFile : public Deserializer
{
public:
    explicit PodBdfFile(Context* context, const String& fileName);

    bool Load();

    virtual bool LoadData() = 0;

    /// Read bytes from the stream. Return number of bytes actually read.
    unsigned Read(void* dest, unsigned size) override;
    /// Set position from the beginning of the stream. Return actual new position.
    unsigned Seek(unsigned position) override;
    /// Read an encrypted pod string
    String ReadPodString();
    /// Seek an offset at the offset table. Return actual new position
    unsigned int SeekOffset(unsigned int idx);

    float ReadFloat();

    Vector3 ReadVector3();

    Matrix3 ReadMatrix3();

    void ReadTextureList(TextureList& list, int width, int height);

    void ReadFace(FaceData& face, unsigned int flags);

    void ReadObject(ObjectData& obj, unsigned int flags);

    const String& GetFileName() { return fileName_; }

    template<typename T, typename Functor, typename... Args>
    void ReadArray(Vector<T>& arr, Functor f, Args&&... args)
    {
        arr.Resize(ReadUInt());
        for (uint32_t i = 0; i < arr.Size(); i++)
            f(*this, arr[i], std::forward<Args>(args)...);
    }

    template<typename T, typename Functor, typename... Args>
    void ReadArrayN(Vector<T>& arr, int count, Functor f, Args&&... args)
    {
        arr.Resize(count);
        for (uint32_t i = 0; i < arr.Size(); i++)
            f(*this, arr[i], std::forward<Args>(args)...);
    }

    Context* context_;

    PodFileType fileType_;

private:
    String fileName_;
    /// Decrypted data
    PODVector<unsigned char> data_;
    /// Offsets relative to data start
    PODVector<int> offsets_;
    /// Header end offset
    unsigned int headerEnd_;
    /// Encryption key
    unsigned int key_;
    /// Block size
    unsigned int blockSize_;
};

static float FloatFromFP1616(fp1616_t fp)
{
    return fp / float(1 << 16);
}

static Vector3 Vector3FromFP1616(fp1616_t* fp)
{
    return Vector3(FloatFromFP1616(fp[0]), FloatFromFP1616(fp[1]), FloatFromFP1616(fp[2]));
}

static Color ColorFromRGB565(uint16_t pix)
{
    uint8_t r5 = (pix >> 11) & 0x001f;
    uint8_t g6 = (pix & 0x07e0) >> 5;
    uint8_t b5 = (pix & 0x001f);

    uint8_t r = (r5 * 527 + 23) >> 6;
    uint8_t g = (g6 * 259 + 33) >> 6;
    uint8_t b = (b5 * 527 + 23) >> 6;

    return Color(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, 1.0f);
}

static Vector3 PodTransform(const Vector3& v)
{
    return Vector3(-v.y_, v.z_, v.x_);
}