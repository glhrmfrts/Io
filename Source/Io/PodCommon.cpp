#include <stdexcept>
#include <Urho3D/Core/Main.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Material.h>
#include "PodCommon.h"
#include "PodVehicle.h"


/// Given a PDBF file with encrypted data, fills a Vector of bytes with decrypted data.
/// Function was mostly copied from the C# code for PDBF by Ray Koopa.
static void Decrypt(unsigned int key, unsigned int blockSize, File& file, PODVector<unsigned char>& dest)
{
    int blockIndex = 0;
    int blockDataSize = blockSize - sizeof(unsigned int);
    int blockDataDwordCount = blockDataSize / sizeof(unsigned int);

    unsigned char* block = new unsigned char[blockSize];
    unsigned int* blockDword = reinterpret_cast<unsigned int*>(block);

    while (!file.IsEof())
    {
        // Process a block.
        file.Read(block, blockSize);//inStream.Read(block, 0, blockSize);
        int i = 0;
        unsigned int checksum = 0;
        if (blockIndex == 0 || (key != 0x00005CA8 && key != 0x0000D13F))
        {
            // First block and most keys always use the default XOR encryption.
            for (; i < blockDataDwordCount; i++)
            {
                unsigned int* value = blockDword + i;
                checksum += (*value ^ key);
                *value ^= key;
            }
        }
        else
        {
            // Starting with the second block, specific keys use a special encryption.
            unsigned int lastValue = 0;
            for (; i < blockDataDwordCount; i++)
            {
                unsigned int keyValue = 0;
                switch (lastValue >> 16 & 3)
                {
                case 0: keyValue = lastValue - 0x50A4A89D; break;
                case 1: keyValue = 0x3AF70BC4 - lastValue; break;
                case 2: keyValue = (lastValue + 0x07091971) << 1; break;
                case 3: keyValue = (0x11E67319 - lastValue) << 1; break;
                }
                unsigned int* value = blockDword + i;
                lastValue = *value;
                switch (lastValue & 3)
                {
                case 0: *value = ~(*value) ^ keyValue; break;
                case 1: *value = ~(*value) ^ ~keyValue; break;
                case 2: *value = (*value) ^ ~keyValue; break;
                case 3: *value = (*value) ^ keyValue ^ 0xFFFF; break;
                }
                checksum += *value;
            }
        }
        // Validate the checksum and write the decrypted block.
        if (checksum != blockDword[i])
            throw std::runtime_error("Invalid PBDF block checksum.");

        dest.Insert(blockIndex * blockDataSize, PODVector<unsigned char>(block, blockDataSize));
        blockIndex++;
    }
}

/// Calculates the PDBF block size given an encryption key
static unsigned int ReadBlockSize(File& file, unsigned int key)
{
    unsigned int checksum = 0;
    while (!file.IsEof())
    {
        unsigned int value = file.ReadUInt();
        if (value == checksum && (file.GetSize() % file.GetPosition() == 0))
            return file.GetPosition();

        checksum += value ^ key;
    }
    throw std::runtime_error("Could not determine PDBF block size");
}

/// Returns an offset table from the file header, the offsets are
/// adjusted to be relative to data start / header end
static PODVector<int> ReadHeader(PodBdfFile& file, unsigned int blockSize)
{
    int headerSize = 2 * sizeof(int); // FileSize + OffsetCount

    // Read header data.
    file.ReadInt(); // Read file size
    int offsetCount = file.ReadInt();
    headerSize += offsetCount * sizeof(int);

    // Read and adjust offsets to be relative to data start.
    PODVector<int> offsets;
    for (int i = 0; i < offsetCount; i++)
    {
        offsets.Push(file.ReadInt());
    }
    for (int i = 0; i < offsetCount; i++)
    {
        offsets[i] -= offsets[i] / blockSize * sizeof(unsigned int);
        offsets[i] -= headerSize;
    }
    return offsets;
}


/*
Face properties (by RayKoopa):

xxxxxxxx SSSSSSSS 00D0020B GGW0R00V

V = Visible
R = Road
W = Wall
G = Graphic level (higher means less textured if graphics low)
B = Black is transparent
2 = Two faced texture, no culling
D = Dural effect
S = Slipperyness level
  = Animated Texture Data?
  = Reflect zones 0-7?
x = not used
*/
bool FaceData::IsVisible() const
{
    return FaceProperties & 1;
}


PodBdfFile::PodBdfFile(Context* context, const String& fileName)
    : context_(context), fileName_(fileName)
{
}

bool PodBdfFile::Load()
{
    UniquePtr<File> file(new File(context_, fileName_, FILE_READ));
    if (!file->IsOpen() || file->IsEof())
        return false;

    key_ = file->ReadUInt() ^ file->GetSize();
    file->Seek(0);
    blockSize_ = ReadBlockSize(*file, key_);
    file->Seek(0);

    data_.Resize(file->GetSize() - (file->GetSize() / blockSize_ * sizeof(unsigned int)));
    Decrypt(key_, blockSize_, *file, data_);
    position_ = 0;
    size_ = data_.Size();

    // Read the header and adjusted offsets.
    offsets_ = ReadHeader(*this, blockSize_);

    headerEnd_ = position_;

    return LoadData();
}

unsigned PodBdfFile::Read(void* dest, unsigned size)
{
    unsigned int acsize = size;
    if ((position_ + size) > data_.Size())
        acsize = size - (data_.Size() - position_);
    std::memcpy(dest, data_.Buffer() + position_, acsize);
    position_ += acsize;
    return acsize;
}

unsigned PodBdfFile::Seek(unsigned position)
{
    position_ = position;
    return position_;
}

String PodBdfFile::ReadPodString()
{
    unsigned char size = ReadUByte();
    if (size == 0) return String();

    char* chars = new char[size];
    for (int i = 0; i < size; i++)
        chars[i] = (char)((byte)(ReadByte() ^ ~i));

    String result(chars, size);
    delete[] chars;
    return result;
}

unsigned int PodBdfFile::SeekOffset(unsigned int idx)
{
    return Seek(headerEnd_ + offsets_[idx]);
}

float PodBdfFile::ReadFloat()
{
    fp1616_t fp = ReadInt();
    return FloatFromFP1616(fp);
}

Vector3 PodBdfFile::ReadVector3()
{
    fp1616_t fp[3];
    Read(fp, sizeof(fp));
    return Vector3FromFP1616(fp);
}

Matrix3 PodBdfFile::ReadMatrix3()
{
    return Matrix3(
        ReadFloat(), ReadFloat(), ReadFloat(),
        ReadFloat(), ReadFloat(), ReadFloat(),
        ReadFloat(), ReadFloat(), ReadFloat()
    );
}

void PodBdfFile::ReadTextureList(TextureList& list, int width, int height)
{
    list.Width = width;
    list.Height = height;

    list.Count = ReadUInt();
    list.Flags = ReadUInt();
    list.ImageList = new ImageList[list.Count];
    list.PixelData = new TexturePixelData[list.Count];

    for (int i = 0; i < list.Count; i++)
    {
        auto imgs = list.ImageList.Get() + i;
        imgs->Count = ReadUInt();
        imgs->ImgData = new ImageData[imgs->Count];
        for (int j = 0; j < imgs->Count; j++)
        {
            auto imgptr = imgs->ImgData.Get() + j;
            Read(imgptr->FileName, 32);
            imgptr->Left = ReadUInt();
            imgptr->Top = ReadUInt();
            imgptr->Right = ReadUInt();
            imgptr->Bottom = ReadUInt();
            imgptr->ImageFlags = ReadUInt();
        }
    }
    for (int i = 0; i < list.Count; i++)
    {
        auto pixptr = list.PixelData.Get() + i;
        pixptr->Pixels = new uint16_t[width * height];
        Read(pixptr->Pixels.Get(), width * height * sizeof(uint16_t));
    }
}

void PodBdfFile::ReadFace(FaceData& face, unsigned int flags)
{
    if (flags & FLAG_NAMED_FACES)
        face.Name = ReadPodString();

    if (key_ == 0x00005CA8)
    {
        face.Indices[3] = ReadUInt();
        face.Indices[0] = ReadUInt();
        face.Vertices = ReadUInt();
        face.Indices[2] = ReadUInt();
        face.Indices[1] = ReadUInt();
    }
    else
    {
        face.Vertices = ReadUInt();
        Read(face.Indices, sizeof(face.Indices));
    }
    
    Read(face.Normal, sizeof(face.Normal));
    face.MaterialType = ReadPodString();
    face.ColorOrTexIndex = ReadUInt();

    {
        // read tex coord
        uint32_t TextureUV[4][2];   // "TEXTURE", "TEXGOU" (00..FF)
        Read(TextureUV, sizeof(TextureUV));
        for (int i = 0; i < 4; i++)
        {
            float oldu = float(TextureUV[i][0]) / 255.0f;
            float oldv = float(TextureUV[i][1]) / 255.0f;
            float u = oldu;
            float v = oldv;

            if (fileType_ == FILE_VEHICLE && (face.MaterialType == "TEXTURE" || face.MaterialType == "TEXGOU"))
            {
                //auto vehicle = static_cast<PodVehicle*>(this);
                //auto texCount = vehicle->materialData_.TexList.Count;
                // Transform tex coord based on texture count because we create only one texture with both images
                //float mul = 1.0f / float(texCount);
                //float offs = float(face.ColorOrTexIndex) / float(texCount);
                //u = (oldu * mul) + offs;
                //v = (oldv * mul) + offs;
            }
            else if (fileType_ != FILE_VEHICLE)
            {
                u = oldu;
                v = oldv;
            }
            else
            {
                u = 1.0f;
                v = 1.0f;
            }

            face.TexCoord[i][0] = u;
            face.TexCoord[i][1] = v;
        }
    }

    face.Reserved1 = ReadUInt();
    if (face.Vertices == 4)
    {
        Read(face.QuadReserved, sizeof(face.QuadReserved));
    }

    if (fileType_ == FILE_CIRCUIT && Vector3FromFP1616(face.Normal) == Vector3::ZERO)
    {
        face.Reserved2 = ReadUInt();
    }
    else
    {
        if (flags & FLAG_FACE_UNK_PROPERTY)
            face.Unknown = ReadUInt();
        face.FaceProperties = ReadUInt();
    }

    face.UseAnimTexCoord = false;
}

void PodBdfFile::ReadObject(ObjectData& obj, unsigned int flags)
{
    obj.VertexCount = ReadUInt();
    obj.VertexArray = new fp1616_t[obj.VertexCount * 3];
    Read(obj.VertexArray.Get(), obj.VertexCount * 3 * sizeof(fp1616_t));
    obj.FaceCount = ReadUInt();
    obj.TriangleCount = ReadUInt();
    obj.QuadrangleCount = ReadUInt();
    obj.FaceData = new FaceData[obj.FaceCount];
    for (int i = 0; i < obj.FaceCount; i++)
    {
        auto& face = obj.FaceData.Get()[i];
        face.Obj = &obj;
        ReadFace(face, flags);
        if (face.Vertices == 3)
            obj.TriFaces.Push(&face);
        else if (face.Vertices == 4)
            obj.QuadFaces.Push(&face);
    }

    obj.Normals = new fp1616_t[obj.VertexCount * 3];
    Read(obj.Normals.Get(), obj.VertexCount * 3 * sizeof(fp1616_t));

    obj.Unknown = ReadUInt();
    if (flags & FLAG_OBJ_HAS_PRISM)
        Read(obj.Prism, sizeof(obj.Prism));
}