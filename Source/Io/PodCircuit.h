#pragma once

#include <Urho3D/Core/Context.h>
#include <string>
#include "Application.h"
#include "PodCommon.h"
#include "PodModelGen.h"

namespace Urho3D { class Model; class Texture2D; class Material; }

class PodCircuit : public PodBdfFile
{
public:
    struct Event
    {
        String Name;
        int32_t ParamSize;
        int32_t ParamCount;
        PODVector<uint8_t> ParamData;
    };

    struct Macro
    {
        uint32_t EventIndex;
        uint32_t ParamIndex;
        uint32_t NextMacro;
    };

    struct Macro1
    {
        uint32_t Value1;
    };

    struct Macro2
    {
        uint32_t Value1;
        uint32_t Value2;
    };

    struct Sector
    {
        ObjectData Object;
        UniqueArray<uint8_t> VertexGamma;
        Vector3 BoundsMin; // Z -= 2
        Vector3 BoundsMax; // Z += 10
    };

    struct Visibility
    {
        int32_t Count;
        UniqueArray<int32_t> VisibleSectorIndices;
    };

    struct DecorationContact
    {
        uint8_t Data[64];
    };

    struct Decoration
    {
        String Name;
        String ObjectName;
        TextureList Textures;
        bool HasNamedFaces;
        ObjectData Object;
        fp1616_t CollisionPrism1[3];
        uint32_t CollisionPrism2;
        fp1616_t CollisionPrism3[3];
        uint32_t Unknown1;
        uint32_t Unknown2[3];
        uint32_t Unknown3;
        uint32_t Unknown4;
        Vector<DecorationContact> Contacts;
    };

    struct DecorationInstance
    {
        int32_t Index;
        Vector<IntVector2> Vectors;
        Vector3 Position; //fp1616_t Position[3];
        Matrix3 Rotation;
    };

    struct SectorDecorationInstance
    {
        Vector<DecorationInstance> DecorationInstances;
    };

    struct EnvironmentSection
    {
        String Name;
        Vector<Macro> Macros;
        Vector<Decoration> Decorations;
        Vector<DecorationInstance> DecorationInstances;
        Vector<SectorDecorationInstance> SectorDecorationInstances;
    };

    struct Light
    {
        uint32_t Type;       // Cylinder, Cone, Sphere
        Vector3 Position;
        Matrix3 Rotation;

        uint32_t Value1;
        uint32_t Value2;
        uint32_t Diffusion;  // None, Linear, Square
        uint32_t Value3;
    };

    struct SectorLights
    {
        Vector<Light> Lights;
    };

    struct LightSection
    {
        String Name;
        uint32_t SectorCount;
        uint32_t Value1;
        uint32_t Value2[3];
        uint32_t Value3;
        uint32_t Value4;
        uint32_t Value5;
        uint32_t Value6;
        uint32_t Value7;
        uint32_t Value8;
        uint32_t Value9;
        Vector<Light> GlobalLights;
        Vector<SectorLights> SectorLights;
    };

    struct Anim1Sector
    {
        int32_t Index;
        uint32_t Unknown1;
        uint32_t Unknown2;
        uint32_t Unknown3;
        uint32_t Unknown4;
        Vector<IntVector2> Vectors;
        Vector3 Position;
        Matrix3 Rotation;
    };

    struct Anim1SectorList
    {
        uint32_t Unknown1;
        Vector<Anim1Sector> Sectors;
    };

    struct Anim1ObjectKey
    {
        Matrix3 Rotation;
        Vector3 Position;
    };

    struct Anim1ObjectFrame
    {
        Vector<Anim1ObjectKey> Keys;
    };

    struct Anim1Object
    {
        int32_t StartFrame;
        bool HasNamedFaces;
        String Name;
        Vector<ObjectData> MeshObjects;
        Vector<Anim1ObjectFrame> Frames;
    };

    struct Anim1TextureConfig
    {
        uint32_t Unknown1;
        uint32_t Unknown2;
        uint32_t Unknown3;
        uint32_t Unknown4;
        uint32_t Unknown5;
    };

    struct Anim1TextureKey
    {
        uint32_t Unknown1;
        IntVector2 Unknown2;
        IntVector2 Unknown3;
    };

    struct Anim1TextureFrame
    {
        IntVector3 Unknown;
        Vector<Anim1TextureKey> Keys;
    };  

    struct Anim1Texture
    {
        int32_t StartFrame;
        uint32_t Unknown;
        String Name;
        TextureList Textures;
        Vector<Anim1TextureConfig> Configs;
        Vector<Anim1TextureFrame> Frames;
    };

    struct Anim1
    {
        static constexpr char WrongWayName[] = "wrongway.ani";

        String Name;
        uint32_t WrongWayValue1;
        uint32_t WrongWayValue2;
        uint32_t Unknown;
        Vector<Anim1Object> Objects;
        Vector<Anim1Texture> Textures;
    };

    struct Anim1Section
    {
        String Name;
        Vector<Macro> Macros;
        Vector<Anim1> Animations;
        Anim1SectorList GlobalSector;
        Vector<Anim1SectorList> Sectors;
        uint32_t Unknown;
    };

    struct Sound
    {
        uint32_t Data[14];
    };

    struct SoundSection
    {
        String Name;
        Vector<Sound> Sounds;
    };

    struct Background
    {
        int32_t FogDistance;
        int32_t FogIntensity;
        int32_t BackDepth;
        int32_t BackBottom;
        bool Visible;
        uint32_t Color;
        String Name;
        TextureList Textures;
        int32_t YStart;
        int32_t YEnd;
    };

    struct Sky
    {
        bool Visible;
        int32_t YEffect;
        int32_t Unknown1;
        int32_t Unknown2;
        int32_t FadeAmount;
        int32_t Speed;
        String Name;
        TextureList Textures;
        TexturePixelData LensFlare;
        int32_t Unknown3;
    };

    struct Anim2AnimationKey
    {
        int32_t TextureIndex;
        Vector2 TexCoord[4];
    };

    struct Anim2AnimationFrame
    {
        float Time;
        int32_t KeyIndex;
    };

    struct Anim2Animation
    {
        String Name;
        Vector<Anim2AnimationKey> Keys;
        float TotalTime;
        Vector<Anim2AnimationFrame> Frames;
    };

    struct Anim2Face
    {
        int32_t Looping;
        int32_t FaceType; // Triangle or Quad
        int32_t FaceIndex; // Into Triangle or Quad List
    };

    struct Anim2Sector
    {
        int32_t SectorIndex;
        Vector<Anim2Face> Faces;
    };

    struct Anim2SectorAnimation
    {
        int32_t AnimIndex;
        Vector<Anim2Sector> Sectors;
    };

    /// Anim2Section (texture animations)
    struct Anim2Section
    {
        static constexpr char AnimSecteurName[] = "ANIME SECTEUR";

        String Name;
        Vector<Anim2Animation> Animations;
        uint32_t Unknown1;
        Vector<Anim2SectorAnimation> SectorAnimations;
    };

    struct RepairZone
    {
        Vector3 Positions[4];
        Vector3 CenterPos;
        float Height;
        float Delay;
    };

    struct RepairZoneSection
    {
        String Name;
        Vector<RepairZone> RepairZones;
        float Time;
    };

    struct DesignationMacro
    {
        Vector3 PlanePos1;
        Vector3 PlanePos2;
        Vector3 PlanePos3;
        Vector3 PlanePos4;
        Vector3 PlaneNormal;
        int MacroIndex1;
        int MacroIndex2;
    };

    struct DesignationValue
    {
        uint32_t Data[8];
    };

    struct DesignationMacroSection
    {
        String Name;
        Vector<Macro> Macros;
        Vector<DesignationMacro> DesignationMacros;
        Vector<DesignationValue> DesignationValues;
    };

    struct DesignationPhase
    {
        String Name;
        int MacroIndex;
    };

    struct DesignationStart
    {
        uint8_t Data[36];
    };

    struct Designation
    {
        uint32_t Value1;  // bool
        uint32_t Value2;
        uint32_t Value3;
        uint32_t Value4;  // bool
        uint32_t Value5;
        uint32_t Value6;
        uint32_t Value7;
        uint32_t Value8;
        uint32_t Value9;
        uint32_t ValueA[5];
        DesignationMacroSection MacroSection;
        Vector<Macro> PhaseMacros;
        Vector<DesignationPhase> Phases;
        Vector<DesignationStart> Starts;
    };

    struct DifficultyPoint
    {
        int Unknown1;
        int Unknown2;
        int PositionIndex;
        int Unknown3;
    };

    struct DifficultyPointList
    {
        Vector<DifficultyPoint> Points;
    };

    struct DifficultyConstraint
    {
        int DesignationIndex;
        int Unknown1;
        int ConstraintAttack;
    };

    struct DifficultyPath
    {
        String Name;
        Vector<Vector3> Positions;
        Vector<DifficultyPointList> PointLists;
        String ConstraintName;
        Vector<DifficultyConstraint> Constraints;
    };

    // Track segment
    struct LevelConfig1
    {
        Vector3 Position; // Segment position
        float RemainingLength; // Remaining track length
        int Value3;
        PODVector<uint32_t> Value4;
        PODVector<uint32_t> Value5;
    };

    struct LevelConfig2
    {
        int Value1;
        int Value2;
        PODVector<int> Value3;
        int Value4;
        Vector<float> Value5;
    };

    struct LevelConfig3
    {
        int Value1, Value2, Value3;
    };

    struct DifficultyLevel
    {
        String Name;
        float TrackLength;
        int Unknown2;
        Vector<LevelConfig1> Config1s;
        Vector<LevelConfig2> Config2s;
        Vector<LevelConfig3> Config3s;
    };

    struct Difficulty
    {
        static constexpr char PlansConstraintesName[] = "PLANS CONTRAINTES";

        String Name;
        DifficultyPath Path;
        DifficultyLevel Level;
    };

    struct Competitor
    {
        String Name;
        int Value1;
        int Value2;
        String Number;
        Vector<IntVector3> Value3;
    };

    struct CompetitorSection
    {
        String DifficultyName;
        String Name;
        Vector<Competitor> Competitors;
    };

    explicit PodCircuit(Context* context, const String& filename);

    bool LoadData() override;

    const String& GetProjectName() { return projectName_; }

    const Vector<Sector>& GetSectors() { return sectors_; }

    Sector& GetSector(int idx) { return sectors_[idx]; }

    SharedPtr<Model> GetSectorModel(int idx) { return sectorModelGens_[idx]->GetModel(); }

    const Vector<SharedPtr<Material>>& GetSectorMaterials(int idx) { return sectorModelGens_[idx]->GetMaterials(); }

    PodModelGen& GetSectorModelGen(int idx) { return *sectorModelGens_[idx]; }

    const Vector<Light>* GetSectorLights(int idx);

    const Vector<Decoration>& GetDecorations() { return envSection_.Decorations; }

    SharedPtr<Model> GetDecorationModel(int idx) { return decorationModelGens_[idx]->GetModel(); }

    const Vector<SharedPtr<Material>>& GetDecorationMaterials(int idx) { return decorationModelGens_[idx]->GetMaterials(); }

    const Vector<DecorationInstance>& GetDecorationInstances() { return envSection_.DecorationInstances; }

    const Vector<Light>& GetGlobalLights() { return lightSection_.GlobalLights; }

    const Vector<RepairZone>& GetRepairZones() { return repairZoneSection_.RepairZones; }

    const Vector<Anim2Section>& GetTextureAnimations() { return anim2Sections_; }

    Vector<Event> events_;
    Vector<Macro> macrosBase_;
    Vector<Macro1> macros_;
    Vector<Macro1> macrosInit_;
    Vector<Macro1> macrosActive_;
    Vector<Macro1> macrosInactive_;
    Vector<Macro2> macrosReplace_;
    Vector<Macro2> macrosExchange_;
    String trackName_;
    uint32_t levelOfDetail_[16];
    String projectName_;
    TextureList textureList_;
    bool hasNamedSectorFaces_;
    Vector<Sector> sectors_;
    Vector<Visibility> visibility_;
    EnvironmentSection envSection_;
    LightSection lightSection_;
    Anim1Section anim1Section_;
    SoundSection soundSection_;
    Background background_;
    Sky sky_;
    Vector<Anim2Section> anim2Sections_;
    RepairZoneSection repairZoneSection_;
    Designation designationForward_;
    Difficulty difficultyForwardEasy_;
    Difficulty difficultyForwardNormal_;
    Difficulty difficultyForwardHard_;
    Designation designationReverse_;
    Difficulty difficultyReverseEasy_;
    Difficulty difficultyReverseNormal_;
    Difficulty difficultyReverseHard_;
    CompetitorSection competitorsEasy_;
    CompetitorSection competitorsNormal_;
    CompetitorSection competitorsHard_;

    // runtime stuff
    Vector<UniquePtr<PodModelGen>> sectorModelGens_;
    Vector<UniquePtr<PodModelGen>> decorationModelGens_;
};