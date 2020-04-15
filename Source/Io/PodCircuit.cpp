#include <stdexcept>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Resource/ResourceCache.h>
#include "PodCircuit.h"
#include "PodModelGen.h"

enum Offset
{
    Default,
    DifficultyForwardNormal,
    DifficultyForwardHard,
    DesignationReverse,
    DifficultyReverseNormal,
    DifficultyReverseHard,
    CompetitorsEasy,
    CompetitorsNormal,
    CompetitorsHard
};

static constexpr int TEXTURE_WIDTH = 256;
static constexpr int TEXTURE_HEIGHT = 256;

static Matrix3 ReadRotation(PodBdfFile& file)
{
    Vector3 rot[3];
    for (int i = 0; i < 3; i++)
        rot[i] = file.ReadVector3();

    return Matrix3(
        rot[1].y_, -rot[1].z_, -rot[1].x_,
        -rot[2].y_, rot[2].z_, rot[2].x_,
        -rot[0].y_, rot[0].z_, rot[0].x_
    );
}

static void ReadEvent(PodBdfFile& file, PodCircuit::Event& ev)
{
    ev.Name = file.ReadPodString();
    ev.ParamSize = file.ReadInt();
    ev.ParamCount = file.ReadInt();
    ev.ParamData.Resize(ev.ParamSize * ev.ParamCount);
    file.Read(ev.ParamData.Buffer(), ev.ParamSize * ev.ParamCount);
}

static void ReadMacro(PodBdfFile& file, PodCircuit::Macro& ma)
{
    ma.EventIndex = file.ReadUInt();
    ma.ParamIndex = file.ReadUInt();
    ma.NextMacro  = file.ReadUInt();
}

static void ReadMacro1(PodBdfFile& file, PodCircuit::Macro1& ma)
{
    ma.Value1 = file.ReadUInt();
}

static void ReadMacro2(PodBdfFile& file, PodCircuit::Macro2& ma)
{
    ma.Value1 = file.ReadUInt();
    ma.Value2 = file.ReadUInt();
}

static void ReadSector(PodBdfFile& file, PodCircuit::Sector& sec)
{
    unsigned int objflags = FLAG_FACE_UNK_PROPERTY;
    if (static_cast<PodCircuit*>(&file)->hasNamedSectorFaces_)
        objflags |= FLAG_NAMED_FACES;

    file.ReadObject(sec.Object, objflags);

    sec.VertexGamma = new uint8_t[sec.Object.VertexCount];
    file.Read(sec.VertexGamma.Get(), sec.Object.VertexCount);
    sec.BoundsMin = PodTransform(file.ReadVector3());
    sec.BoundsMax = PodTransform(file.ReadVector3());
}

static void ReadVisibility(PodBdfFile& file, PodCircuit::Visibility& vis)
{
    vis.Count = file.ReadInt();
    if (vis.Count > -1)
    {
        vis.VisibleSectorIndices = new int32_t[vis.Count];
        file.Read(vis.VisibleSectorIndices.Get(), vis.Count * sizeof(int32_t));
    }
}

static void ReadDecorationContact(PodBdfFile& file, PodCircuit::DecorationContact& c)
{
    file.Read(c.Data, sizeof(c.Data));
}

static void ReadDecoration(PodBdfFile& file, PodCircuit::Decoration& dec)
{
    dec.Name = file.ReadPodString();
    dec.ObjectName = file.ReadPodString();
    file.ReadTextureList(dec.Textures, 128, 128);
    dec.HasNamedFaces = (bool)file.ReadUInt();

    unsigned int objflags = dec.HasNamedFaces ? FLAG_NAMED_FACES : 0;
    file.ReadObject(dec.Object, objflags);

    file.Read(dec.CollisionPrism1, sizeof(dec.CollisionPrism1));
    dec.CollisionPrism2 = file.ReadUInt();
    file.Read(dec.CollisionPrism3, sizeof(dec.CollisionPrism3));

    dec.Unknown1 = file.ReadUInt();
    file.Read(dec.Unknown2, sizeof(dec.Unknown2));
    dec.Unknown3 = file.ReadUInt();
    dec.Unknown4 = file.ReadUInt();

    file.ReadArray(dec.Contacts, ReadDecorationContact);
}

static void ReadIntVector2(PodBdfFile& file, IntVector2& v)
{
    v.x_ = file.ReadInt();
    v.y_ = file.ReadInt();
}

static void ReadDecorationInstance(PodBdfFile& file, PodCircuit::DecorationInstance& di)
{
    di.Index = file.ReadInt();
    file.ReadArray(di.Vectors, ReadIntVector2);

    di.Position = PodTransform(file.ReadVector3());
    di.Rotation = ReadRotation(file);
}

static void ReadSectorDecorationInstance(PodBdfFile& file, PodCircuit::SectorDecorationInstance& sd)
{    
    file.ReadArray(sd.DecorationInstances, ReadDecorationInstance);
}

static void ReadLight(PodBdfFile& file, PodCircuit::Light& l)
{
    l.Type = file.ReadUInt();
    //file.Read(l.Data, sizeof(l.Data));
    l.Position = PodTransform(file.ReadVector3());
    l.Rotation = ReadRotation(file);

    l.Value1 = file.ReadUInt();
    l.Value2 = file.ReadUInt();
    l.Diffusion = file.ReadUInt();
    l.Value3 = file.ReadUInt();
}

static void ReadAnim1ObjectKey(PodBdfFile& file, PodCircuit::Anim1ObjectKey& k)
{
    k.Rotation = ReadRotation(file);
    k.Position = PodTransform(file.ReadVector3());
}

static void ReadAnim1ObjectFrame(PodBdfFile& file, PodCircuit::Anim1ObjectFrame& f, int meshCount)
{
    f.Keys.Resize(meshCount);
    for (int i = 0; i < meshCount; i++)
    {
        if ((bool)file.ReadUInt())
            ReadAnim1ObjectKey(file, f.Keys[i]);
    }
}

static void ReadAnim1Object(PodBdfFile& file, PodCircuit::Anim1Object& obj)
{
    obj.StartFrame = file.ReadInt();
    int frameCount = file.ReadInt();
    obj.HasNamedFaces = (bool)file.ReadUInt();
    int meshCount = file.ReadInt();
    obj.Name = file.ReadPodString();

    unsigned int objflags = obj.HasNamedFaces ? FLAG_NAMED_FACES : 0;
    obj.MeshObjects.Resize(meshCount);
    for (int i = 0; i < meshCount; i++)
        file.ReadObject(obj.MeshObjects[i], objflags);

    file.ReadArrayN(obj.Frames, frameCount, ReadAnim1ObjectFrame, meshCount);
}

static void ReadAnim1TextureConfig(PodBdfFile& file, PodCircuit::Anim1TextureConfig& cfg)
{
    cfg.Unknown1 = file.ReadUInt();
    cfg.Unknown2 = file.ReadUInt();
    cfg.Unknown3 = file.ReadUInt();
    cfg.Unknown4 = file.ReadUInt();
    cfg.Unknown5 = file.ReadUInt();
}

static void ReadAnim1TextureKey(PodBdfFile& file, PodCircuit::Anim1TextureKey& k)
{
    k.Unknown1 = file.ReadUInt();
    file.Read(&k.Unknown2, sizeof(k.Unknown2));
    file.Read(&k.Unknown3, sizeof(k.Unknown3));
}

static void ReadAnim1TextureFrame(PodBdfFile& file, PodCircuit::Anim1TextureFrame& f)
{
    file.Read(&f.Unknown, sizeof(f.Unknown));
    file.ReadArray(f.Keys, ReadAnim1TextureKey);
}

static void ReadAnim1Texture(PodBdfFile& file, PodCircuit::Anim1Texture& t)
{
    t.StartFrame = file.ReadInt();
    int frameCount = file.ReadInt();
    int configCount = file.ReadInt();
    t.Unknown = file.ReadUInt();
    t.Name = file.ReadPodString();
    file.ReadTextureList(t.Textures, 256, 256);

    file.ReadArrayN(t.Configs, configCount, ReadAnim1TextureConfig);
    file.ReadArrayN(t.Frames, frameCount, ReadAnim1TextureFrame);
}

static void ReadAnim1(PodBdfFile& file, PodCircuit::Anim1& anim)
{
    anim.Name = file.ReadPodString();
    if (anim.Name == String(PodCircuit::Anim1::WrongWayName))
    {
        anim.WrongWayValue1 = file.ReadUInt();
        anim.WrongWayValue2 = file.ReadUInt();
    }
    int texturesCount = file.ReadInt();
    int objectCount = file.ReadInt();
    anim.Unknown = file.ReadUInt();

    file.ReadArrayN(anim.Objects, objectCount, ReadAnim1Object);
    file.ReadArrayN(anim.Textures, texturesCount, ReadAnim1Texture);
}

static void ReadAnim1Sector(PodBdfFile& file, PodCircuit::Anim1Sector& sec)
{
    sec.Index = file.ReadInt();
    sec.Unknown1 = file.ReadUInt();
    sec.Unknown2 = file.ReadUInt();
    sec.Unknown3 = file.ReadUInt();
    sec.Unknown4 = file.ReadUInt();
    file.ReadArray(sec.Vectors, ReadIntVector2);
    sec.Position = PodTransform(file.ReadVector3());
    sec.Rotation = ReadRotation(file);
}

static void ReadAnim1SectorList(PodBdfFile& file, PodCircuit::Anim1SectorList& list)
{
    int count = file.ReadInt();
    list.Unknown1 = file.ReadUInt();
    file.ReadArrayN(list.Sectors, count, ReadAnim1Sector);
}

static void ReadSound(PodBdfFile& file, PodCircuit::Sound& sound)
{
    file.Read(sound.Data, sizeof(sound.Data));
}

static void ReadAnim2AnimationKey(PodBdfFile& file, PodCircuit::Anim2AnimationKey& k)
{
    k.TextureIndex = file.ReadInt();

    // read tex coord
    uint32_t TextureUV[4][2];
    file.Read(TextureUV, sizeof(TextureUV));
    for (int i = 0; i < 4; i++)
    {
        float u = float(TextureUV[i][0]) / 255.0f;
        float v = float(TextureUV[i][1]) / 255.0f;
        k.TexCoord[i] = Vector2(u, v);
    }
}

static void ReadAnim2AnimationFrame(PodBdfFile& file, PodCircuit::Anim2AnimationFrame& f)
{
    f.Time = file.ReadFloat();
    f.KeyIndex = file.ReadInt();
}

static void ReadAnim2Animation(PodBdfFile& file, PodCircuit::Anim2Animation& a)
{
    a.Name = file.ReadPodString();
    file.ReadArray(a.Keys, ReadAnim2AnimationKey);
    a.TotalTime = file.ReadFloat();
    file.ReadArray(a.Frames, ReadAnim2AnimationFrame);
}

static void ReadAnim2Face(PodBdfFile& file, PodCircuit::Anim2Face& f)
{
    f.Looping = file.ReadInt();
    f.FaceType = file.ReadInt();
    f.FaceIndex = file.ReadInt();
}

static void ReadAnim2Sector(PodBdfFile& file, PodCircuit::Anim2Sector& sec)
{
    sec.SectorIndex = file.ReadInt();
    file.ReadArray(sec.Faces, ReadAnim2Face);
}

static void ReadAnim2SectorAnimation(PodBdfFile& file, PodCircuit::Anim2SectorAnimation& ani)
{
    ani.AnimIndex = file.ReadInt();
    file.ReadArray(ani.Sectors, ReadAnim2Sector);
}

static void ReadAnim2Section(PodBdfFile& file, PodCircuit::Anim2Section& sec)
{
    sec.Name = file.ReadPodString();
    if (sec.Name.Empty() || sec.Name == "NEANT")
        return;

    if (sec.Name == String(PodCircuit::Anim2Section::AnimSecteurName))
        throw std::runtime_error("anime secteur not implemented");

    file.ReadArray(sec.Animations, ReadAnim2Animation);
    sec.Unknown1 = file.ReadUInt();
    file.ReadArray(sec.SectorAnimations, ReadAnim2SectorAnimation);
}

static void ReadRepairZone(PodBdfFile& file, PodCircuit::RepairZone& zone)
{
    for (int i = 0; i < 4; i++)
        zone.Positions[i] = PodTransform(file.ReadVector3());

    zone.CenterPos = PodTransform(file.ReadVector3());
    zone.Height = file.ReadFloat();
    zone.Delay = file.ReadFloat();
}

static void ReadDesignationMacro(PodBdfFile& file, PodCircuit::DesignationMacro& m)
{
    m.PlanePos1 = PodTransform(file.ReadVector3());
    m.PlanePos2 = PodTransform(file.ReadVector3());
    m.PlanePos3 = PodTransform(file.ReadVector3());
    m.PlanePos4 = PodTransform(file.ReadVector3());
    m.PlaneNormal = PodTransform(file.ReadVector3());
    m.MacroIndex1 = file.ReadInt();
    m.MacroIndex2 = file.ReadInt();
}

static void ReadDesignationValue(PodBdfFile& file, PodCircuit::DesignationValue& v)
{
    file.Read(v.Data, sizeof(v.Data));
}

static void ReadDesignationPhase(PodBdfFile& file, PodCircuit::DesignationPhase& p)
{
    p.Name = file.ReadPodString();
    p.MacroIndex = file.ReadInt();
}

static void ReadDesignationStart(PodBdfFile& file, PodCircuit::DesignationStart& s)
{
    file.Read(s.Data, sizeof(s.Data));
}

static void ReadDesignation(PodBdfFile& file, PodCircuit::Designation& d)
{
    d.Value1 = file.ReadUInt();
    d.Value2 = file.ReadUInt();
    d.Value3 = file.ReadUInt();
    d.Value4 = file.ReadUInt();
    d.Value5 = file.ReadUInt();
    d.Value6 = file.ReadUInt();
    d.Value7 = file.ReadUInt();
    d.Value8 = file.ReadUInt();
    d.Value9 = file.ReadUInt();
    file.Read(d.ValueA, sizeof(d.ValueA));

    d.MacroSection.Name = file.ReadPodString();
    if (!d.MacroSection.Name.Empty() && d.MacroSection.Name != "NEANT")
    {
        file.ReadArray(d.MacroSection.Macros, ReadMacro);
        file.ReadArray(d.MacroSection.DesignationMacros, ReadDesignationMacro);
        file.ReadArray(d.MacroSection.DesignationValues, ReadDesignationValue);
    }

    file.ReadArray(d.PhaseMacros, ReadMacro);
    file.ReadArray(d.Phases, ReadDesignationPhase);
    file.ReadArray(d.Starts, ReadDesignationStart);
}

static void ReadDifficulty(PodBdfFile& file, PodCircuit::Difficulty& d)
{
    d.Name = file.ReadPodString();
    {
        d.Path.Name = file.ReadPodString();
        if (!d.Path.Name.Empty() && d.Path.Name != "NEANT")
        {
            file.ReadArray(d.Path.Positions, [](PodBdfFile& file, Vector3& v) { v = PodTransform(file.ReadVector3()); });
            file.ReadArray(d.Path.PointLists, [](PodBdfFile& file, PodCircuit::DifficultyPointList& l)
            {
                file.ReadArray(l.Points, [](PodBdfFile& file, PodCircuit::DifficultyPoint& p)
                {
                    p.Unknown1 = file.ReadInt();
                    p.Unknown2 = file.ReadInt();
                    p.PositionIndex = file.ReadInt();
                    p.Unknown3 = file.ReadInt();
                });
            });
            d.Path.ConstraintName = file.ReadPodString();
            if (!d.Path.ConstraintName.Empty() && d.Path.ConstraintName != "NEANT")
            {
                file.ReadArray(d.Path.Constraints, [](PodBdfFile& file, PodCircuit::DifficultyConstraint& c)
                {
                    c.DesignationIndex = file.ReadInt();
                    c.Unknown1 = file.ReadInt();
                    c.ConstraintAttack = file.ReadInt();
                });
            }
        }
    }
    {
        d.Level.Name = file.ReadPodString();
        if (!d.Level.Name.Empty() && d.Level.Name != "NEANT")
        {
            d.Level.TrackLength = file.ReadFloat();
            int levelConfig1Count = file.ReadInt();
            int levelConfig2Count = file.ReadInt();
            int levelConfig3Count = file.ReadInt();
            d.Level.Unknown2 = file.ReadInt();
            file.ReadArrayN(d.Level.Config1s, levelConfig1Count, [](PodBdfFile& file, PodCircuit::LevelConfig1& c)
            {
                c.Position = PodTransform(file.ReadVector3());
                c.RemainingLength = file.ReadFloat();
                c.Value3 = file.ReadInt();
                c.Value4.Resize(file.ReadInt()); file.Read(c.Value4.Buffer(), sizeof(uint32_t) * c.Value4.Size());
                c.Value5.Resize(file.ReadInt()); file.Read(c.Value5.Buffer(), sizeof(uint32_t) * c.Value5.Size());
            });
            file.ReadArrayN(d.Level.Config2s, levelConfig2Count, [](PodBdfFile& file, PodCircuit::LevelConfig2& c)
            {
                c.Value1 = file.ReadInt();
                c.Value2 = file.ReadInt();
                c.Value3.Resize(file.ReadInt()); file.Read(c.Value3.Buffer(), sizeof(uint32_t) * c.Value3.Size());
                c.Value4 = file.ReadInt();
                //c.Value5.Resize(file.ReadInt()); file.Read(c.Value5.Buffer(), sizeof(uint32_t) * c.Value5.Size());
                file.ReadArray(c.Value5, [](PodBdfFile& file, float& f) {
                    f = file.ReadFloat();
                });
            });
            file.ReadArrayN(d.Level.Config3s, levelConfig3Count, [](PodBdfFile& file, PodCircuit::LevelConfig3& c)
            {
                c.Value1 = file.ReadInt();
                c.Value2 = file.ReadInt();
                c.Value3 = file.ReadInt();
            });
        }
    }
}

static void ReadCompetitorSection(PodBdfFile& file, PodCircuit::CompetitorSection& cs)
{
    cs.DifficultyName = file.ReadPodString();
    cs.Name = file.ReadPodString();
    file.ReadArray(cs.Competitors, [](PodBdfFile& file, PodCircuit::Competitor& c)
    {
        c.Name = file.ReadPodString();
        c.Value1 = file.ReadInt();
        c.Value2 = file.ReadInt();
        c.Number = file.ReadPodString();
        c.Value3.Resize(file.ReadInt()); file.Read(c.Value3.Buffer(), c.Value3.Size() * sizeof(IntVector3));
    });
}

PodCircuit::PodCircuit(Context* context, const String& filename)
    : PodBdfFile(context, filename)
{
    fileType_ = FILE_CIRCUIT;
}

bool PodCircuit::LoadData()
{
    SeekOffset(Offset::Default);

    uint32_t checksum = ReadUInt(); // must be 3
    uint32_t reserved = ReadUInt(); // not used
    
    // Read events and macros
    int32_t eventCount = ReadInt();
    events_.Resize(eventCount);

    uint32_t bufferSize = ReadUInt();
    for (int i = 0; i < eventCount; i++)
        ReadEvent(*this, events_[i]);

    ReadArray(macrosBase_, ReadMacro);
    ReadArray(macros_, ReadMacro1);
    ReadArray(macrosInit_, ReadMacro1);
    ReadArray(macrosActive_, ReadMacro1);
    ReadArray(macrosInactive_, ReadMacro1);
    ReadArray(macrosReplace_, ReadMacro2);
    ReadArray(macrosExchange_, ReadMacro2);

    // Read track info
    trackName_ = ReadPodString();
    Read(levelOfDetail_, sizeof(levelOfDetail_));
    projectName_ = ReadPodString();
    ReadTextureList(textureList_, TEXTURE_WIDTH, TEXTURE_HEIGHT);
    hasNamedSectorFaces_ = (bool)ReadUInt();
    ReadArray(sectors_, ReadSector);
    ReadArray(visibility_, ReadVisibility);

    // Read environment section
    envSection_.Name = ReadPodString();
    if (!envSection_.Name.Empty() && envSection_.Name != "NEANT")
    {
        ReadArray(envSection_.Macros, ReadMacro);
        ReadArray(envSection_.Decorations, ReadDecoration);
        ReadArray(envSection_.DecorationInstances, ReadDecorationInstance);
        ReadArrayN(envSection_.SectorDecorationInstances, sectors_.Size(), ReadSectorDecorationInstance);
    }

    // Read light section
    lightSection_.Name = ReadPodString();
    if (!lightSection_.Name.Empty() && lightSection_.Name != "NEANT")
    {
        lightSection_.SectorCount = ReadUInt();
        lightSection_.Value1 = ReadUInt();
        Read(lightSection_.Value2, sizeof(lightSection_.Value2));
        lightSection_.Value3 = ReadUInt();
        lightSection_.Value4 = ReadUInt();
        lightSection_.Value5 = ReadUInt();
        lightSection_.Value6 = ReadUInt();
        lightSection_.Value7 = ReadUInt();
        lightSection_.Value8 = ReadUInt();
        lightSection_.Value9 = ReadUInt();
        if (lightSection_.SectorCount > 0)
            ReadArray(lightSection_.GlobalLights, ReadLight);

        lightSection_.SectorLights.Resize(lightSection_.SectorCount);
        for (unsigned int i = 0; i < lightSection_.SectorCount; i++)
            ReadArray(lightSection_.SectorLights[i].Lights, ReadLight);
    }

    // Read Animation1 Section
    anim1Section_.Name = ReadPodString();
    if (!anim1Section_.Name.Empty() && anim1Section_.Name != "NEANT")
    {
        ReadArray(anim1Section_.Macros, ReadMacro);
        ReadArray(anim1Section_.Animations, ReadAnim1);
        anim1Section_.Unknown = ReadUInt();
        if (sectors_.Size() > 0)
            ReadAnim1SectorList(*this, anim1Section_.GlobalSector);
        ReadArrayN(anim1Section_.Sectors, sectors_.Size(), ReadAnim1SectorList);
    }

    // Read sound section
    soundSection_.Name = ReadPodString();
    if (!soundSection_.Name.Empty() && soundSection_.Name != "NEANT")
        ReadArray(soundSection_.Sounds, ReadSound);

    // Read background/sky data
    background_.FogDistance = ReadInt();
    background_.FogIntensity = ReadInt();
    background_.BackDepth = ReadInt();
    background_.BackBottom = ReadInt();
    background_.Visible = (bool)ReadUInt();
    background_.Color = ReadUInt();
    background_.Name = ReadPodString();
    ReadTextureList(background_.Textures, 256, 256);
    background_.YStart = ReadInt();
    background_.YEnd = ReadInt();

    sky_.Visible = (bool)ReadUInt();
    sky_.YEffect = ReadInt();
    sky_.Unknown1 = ReadInt();
    sky_.Unknown2 = ReadInt();
    sky_.FadeAmount = ReadInt();
    sky_.Speed = ReadInt();
    sky_.Name = ReadPodString();
    ReadTextureList(sky_.Textures, 128, 128);
    sky_.LensFlare.Pixels = new uint16_t[128 * 128];
    Read(sky_.LensFlare.Pixels.Get(), 128 * 128 * sizeof(uint16_t));
    sky_.Unknown3 = ReadInt();

    // Read Animation2 section
    unsigned int anim2Count = (ReadUInt()+1) / 2;
    ReadArrayN(anim2Sections_, anim2Count, ReadAnim2Section);

    // Read repair zones section
    repairZoneSection_.Name = ReadPodString();
    if (!repairZoneSection_.Name.Empty() && repairZoneSection_.Name != "NEANT")
    {
        ReadArray(repairZoneSection_.RepairZones, ReadRepairZone);
        repairZoneSection_.Time = ReadFloat();
    }

    // Read designations and difficulty
    ReadDesignation(*this, designationForward_);
    ReadDifficulty(*this, difficultyForwardEasy_);

    SeekOffset(Offset::DifficultyForwardNormal);
    ReadDifficulty(*this, difficultyForwardNormal_);

    SeekOffset(Offset::DifficultyForwardHard);
    ReadDifficulty(*this, difficultyForwardHard_);

    SeekOffset(Offset::DesignationReverse);
    ReadDesignation(*this, designationReverse_);
    ReadDifficulty(*this, difficultyReverseEasy_);

    SeekOffset(Offset::DifficultyReverseNormal);
    ReadDifficulty(*this, difficultyReverseNormal_);

    SeekOffset(Offset::DifficultyReverseHard);
    ReadDifficulty(*this, difficultyReverseHard_);

    // Read competitors
    SeekOffset(Offset::CompetitorsEasy);
    ReadCompetitorSection(*this, competitorsEasy_);

    SeekOffset(Offset::CompetitorsNormal);
    ReadCompetitorSection(*this, competitorsNormal_);

    SeekOffset(Offset::CompetitorsHard);
    ReadCompetitorSection(*this, competitorsHard_);

    // Create main model, process sectors to create geometry
    for (int i = 0; i < sectors_.Size(); i++)
    {
        auto& sec = sectors_[i];
        auto modelGen = new PodModelGen(context_, textureList_);
        modelGen->AddObject(sec.Object);
        modelGen->SetBounds(sec.BoundsMin, sec.BoundsMax);
        sectorModelGens_.Push(UniquePtr<PodModelGen>(modelGen));
    }

    // Process decoration objects
    for (int i = 0; i < envSection_.Decorations.Size(); i++)
    {
        auto& dec = envSection_.Decorations[i];
        auto modelGen = new PodModelGen(context_, dec.Textures);
        modelGen->AddObject(dec.Object);
        decorationModelGens_.Push(UniquePtr<PodModelGen>(modelGen));
    }

    return true;
}

const Vector<PodCircuit::Light>* PodCircuit::GetSectorLights(int idx)
{
    if (idx < lightSection_.SectorLights.Size())
        return &lightSection_.SectorLights[idx].Lights;

    return nullptr;
}
