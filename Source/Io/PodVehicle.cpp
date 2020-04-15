#include <stdexcept>
#include <Urho3D/Core/Main.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Resource/ResourceCache.h>
#include "PodVehicle.h"
#include "PodModelGen.h"

static constexpr int TEXTURE_WIDTH = 128;
static constexpr int TEXTURE_HEIGHT = 128;

// Unknown data structures
struct PositionData
{
    uint8_t  unknown_0000[57];
    uint8_t  Unknown_0039;
    uint8_t  unknown_003A[2];
    uint32_t Unknown_003C;
    uint32_t Unknown_0040;
    uint32_t Unknown_0044;
    fp1616_t Position[3];  // [0..3].xy, -[4].z
    uint32_t Unknown_0054;
    uint8_t  unknown_0058[56];
    uint32_t Unknown_0090;
    uint32_t Unknown_0094;
    uint32_t Unknown_0098;
    uint32_t Unknown_009C;
    uint32_t Unknown_00A0;
    uint8_t  unknown_00A4[8];
    uint32_t Unknown_00AC;
    uint32_t Unknown_00B0;
    uint32_t Unknown_00B4;
    uint32_t Unknown_00B8;
    uint32_t Unknown_00BC;
    uint32_t Unknown_00C0;
    uint8_t  unknown_00C4[20];
};

struct UnknownData  // one big struct
{
    PositionData pos[5];
    uint8_t  unknown_0438[4];
    uint32_t unknown_043C;
    uint8_t  unknown_0440[8];
    uint8_t  Unknown_0448;
    uint8_t  unknown_0449[2];
    uint8_t  Unknown_044B;
    uint8_t  Unknown_044C[36];
    uint8_t  unknown_0470[4];
    uint32_t Unknown_0474;
    uint8_t  unknown_0478[4];
    uint32_t Unknown_047C;
    uint32_t Unknown_0480;
    uint32_t Unknown_0484;
    uint32_t Unknown_0488;
    uint32_t Unknown_048C;
    //POD1: this block is 4 bytes larger
    uint8_t  unknown_0490[100];
    uint32_t unknown_04F4;
    uint8_t  unknown_04F8[28];
    uint32_t Unknown_0514;
    //...
    uint32_t Unknown_0518;
    uint32_t Unknown_051C;
    uint32_t Unknown_0520;
    uint8_t  unknown_0524[4];
    uint32_t Unknown_0528;
    uint8_t  unknown_052C[12];
    uint32_t Unknown_0538;
    uint32_t Unknown_053C;
    uint32_t Unknown_0540;
    uint32_t Unknown_0544;
    uint32_t Unknown_0548;
    uint32_t Unknown_054C;  //POD1: 00000002, POD2: 00000001
    uint8_t  unknown_0550[8];

    struct List
    {
        uint32_t Count;
        UniquePtr<uint32_t> Array;
    } list;
};

static int RegionToWheelIndex(PodVehicleRegion reg)
{
    int idx = -1;
    switch (reg)
    {
    case VR_FRONT_R:
        idx = 0;
        break;
    case VR_REAR_R:
        idx = 1;
        break;
    case VR_FRONT_L:
        idx = 2;
        break;
    case VR_REAR_L:
        idx = 3;
        break;
    }
    return idx;
}

PodVehicle::PodVehicle(Context* context, const String& fileName)
    : PodBdfFile(context, fileName)
{
    fileType_ = FILE_VEHICLE;
}

bool PodVehicle::LoadData()
{
    name_ = ReadPodString();

    {
        UnknownData unknownData;
        Read(&unknownData, sizeof(UnknownData) - sizeof(UnknownData::List));
        unknownData.list.Count = ReadUInt();
        unknownData.list.Array = new uint32_t[unknownData.list.Count];
        Read(unknownData.list.Array.Get(), unknownData.list.Count * sizeof(uint32_t));

        for (int i = 0; i < 5; i++)
        {
            offsets_[i] = Vector3(
                FloatFromFP1616(unknownData.pos[i].Position[1]),
                FloatFromFP1616(unknownData.pos[i].Position[2]),
                FloatFromFP1616(unknownData.pos[i].Position[0])
            );
        }
    }

    // Read material data
    materialData_.Name = ReadPodString();
    if (materialData_.Name != "GOURAUD")
        ReadTextureList(materialData_.TexList, TEXTURE_WIDTH, TEXTURE_HEIGHT);

    // Read objects data

    objectsData_.NamedFaces = ReadUInt();
    unsigned int objflags = objectsData_.NamedFaces ? FLAG_NAMED_FACES : 0;
    objflags |= FLAG_OBJ_HAS_PRISM;

    // Read chassis objects
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            ReadObject(objectsData_.ChassisObjects[i][j], objflags);
        }
    }

    // Read wheels objects
    for (int i = 0; i < 4; i++)
    {
        ReadObject(objectsData_.WheelObjects[i], objflags);
    }

    // Read shadow objects
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            ReadObject(objectsData_.ShadowObjects[i][j], objflags &~ FLAG_OBJ_HAS_PRISM); // shadow objects have no prism
        }
    }

    // Read collision meshes
    for (int i = 0; i < 2; i++)
    {
        auto col = objectsData_.CollisionData + i;
        col->MaterialName = ReadPodString();
        col->NamedFaces = ReadUInt();
        unsigned int cflags = col->NamedFaces ? FLAG_NAMED_FACES : 0;
        cflags |= FLAG_OBJ_HAS_PRISM;
        ReadObject(col->ObjData, cflags);

        // unknown stuff
        col->Unknown_1 = ReadUInt();
        Read(col->Unknown_2, sizeof(col->Unknown_2));
        col->Unknown_3 = ReadUInt();
        col->Unknown_4 = ReadUInt();
        col->Unknown_5 = ReadUInt();
        col->Unknown_6 = new uint8_t[col->Unknown_5 * 64];
        Read(col->Unknown_6, col->Unknown_5 * 64);
    }

    // Read noise
    Read(&noiseData_, sizeof(noiseData_));

    // Read characteristics
    Read(&charsData_, sizeof(charsData_));

    // Generate models
    bodyModelGen_ = new PodModelGen(context_, materialData_.TexList);
    for (int i = 0; i < 6; i++)
        bodyModelGen_->AddObject(objectsData_.ChassisObjects[0][i]);

    for (int i = 0; i < 4; i++)
        wheelsModelGen_.Push(UniquePtr<PodModelGen>(new PodModelGen(context_, materialData_.TexList)));

    wheelsModelGen_[RegionToWheelIndex(VR_FRONT_L)]->AddObject(objectsData_.WheelObjects[RegionToWheelIndex(VR_FRONT_L)]);
    wheelsModelGen_[RegionToWheelIndex(VR_FRONT_R)]->AddObject(objectsData_.WheelObjects[RegionToWheelIndex(VR_FRONT_R)]);
    wheelsModelGen_[RegionToWheelIndex(VR_REAR_L)]->AddObject(objectsData_.WheelObjects[RegionToWheelIndex(VR_REAR_L)]);
    wheelsModelGen_[RegionToWheelIndex(VR_REAR_R)]->AddObject(objectsData_.WheelObjects[RegionToWheelIndex(VR_REAR_R)]);

    return true;
}

Model* PodVehicle::GetChassisModel(PodVehicleCondition cond)
{
    return bodyModelGen_->GetModel();
}

const Vector<SharedPtr<Material>>& PodVehicle::GetChassisMaterials(PodVehicleCondition cond)
{
    return bodyModelGen_->GetMaterials();
}

Vector3 PodVehicle::GetChassisOffset()
{
    return offsets_[4];
}

Model* PodVehicle::GetWheelModel(PodVehicleRegion reg)
{
    return wheelsModelGen_[RegionToWheelIndex(reg)]->GetModel();
}

const Vector<SharedPtr<Material>>& PodVehicle::GetWheelMaterials(PodVehicleRegion reg)
{
    return wheelsModelGen_[RegionToWheelIndex(reg)]->GetMaterials();
}

Vector3 PodVehicle::GetWheelOffset(PodVehicleRegion region)
{
    switch (region)
    {
    case VR_FRONT_R:
        return offsets_[0];
    case VR_REAR_R:
        return offsets_[1];
    case VR_FRONT_L:
        return offsets_[2];
    case VR_REAR_L:
        return offsets_[3];
    }
    return Vector3();
}