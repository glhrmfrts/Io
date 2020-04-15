#pragma once

#include <Urho3D/Core/Context.h>
#include <string>
#include "Application.h"
#include "PodCommon.h"

namespace Urho3D { class Model; class Texture2D; class Material; }

class PodModelGen;

enum PodVehicleCondition
{
    VC_GOOD = 0,
    VC_DAMAGED = 1,
    VC_RUINED = 2,
};

enum PodVehicleRegion
{
    VR_FRONT_R,
    VR_SIDE_R,
    VR_REAR_R,
    VR_FRONT_L,
    VR_SIDE_L,
    VR_REAR_L,
};

class PodVehicle : public PodBdfFile
{
public:
    struct MaterialData
    {
        String Name;

        //if (Name.String != "GOURAUD") {
        TextureList TexList;
    };

    struct CollisionData
    {
        String MaterialName;
        uint32_t NamedFaces;
        ObjectData ObjData;
        uint8_t  Prism[28];
        uint32_t Unknown_1;
        uint32_t Unknown_2[3];
        uint32_t Unknown_3;
        uint32_t Unknown_4;
        uint32_t Unknown_5;
        uint8_t* Unknown_6;  // [Unknown_5] [64] ;
    };

    struct ObjectsData
    {
        uint32_t NamedFaces;
        ObjectData ChassisObjects[3][6]; // chassis[Good,Damaged,Ruined][RearR,RearL,SideR,SideL,FrontR,FrontL]
        ObjectData WheelObjects[4];     // wheel[FrontR,RearR,FrontL,RearL]
        ObjectData ShadowObjects[2][2]; // shadow[Good,Ruined][Front,Rear]
        CollisionData CollisionData[2];
    };

    struct NoiseData
    {
        uint32_t Runtime;  // MEG index
        uint16_t Unknown[15];
        uint16_t Reserved;
    };

    struct CharacteristicsData
    {
        uint32_t Acceleration;
        uint32_t Brakes;
        uint32_t Grip;
        uint32_t Handling;
        uint32_t Speed;
        // Acceleration + Brakes + Grip + Handling + Speed <= 300
    };

    explicit PodVehicle(Context* context, const String& filename);

    bool LoadData() override;

    Model* GetChassisModel(PodVehicleCondition cond);

    const Vector<SharedPtr<Material>>& GetChassisMaterials(PodVehicleCondition cond);

    Vector3 GetChassisOffset();

    Model* GetWheelModel(PodVehicleRegion region);

    const Vector<SharedPtr<Material>>& GetWheelMaterials(PodVehicleRegion reg);

    Vector3 GetWheelOffset(PodVehicleRegion region);

    const String& GetName() { return name_; }

    MaterialData materialData_;

private:
    /// Vehicle name
    String name_;
    /// Chassis and wheels offsets
    Vector3 offsets_[5];

    ObjectsData objectsData_;

    NoiseData noiseData_;
    
    CharacteristicsData charsData_;

    UniquePtr<PodModelGen> bodyModelGen_;

    Vector<UniquePtr<PodModelGen>> wheelsModelGen_;
};