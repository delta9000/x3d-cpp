#ifndef X3D_ENUMS_HPP
#define X3D_ENUMS_HPP

// Auto-generated C++ 'enum class' definitions for the bounded X3D
// SimpleType enumerations (closed vocabularies) in the UOM spec.

#include <string>

// AccessTypeChoices: accessTypeChoices are strictly allowed enumeration values for accessType, used when defining field declarations within a Script node or ProtoDeclare/ExternProtoDeclare statements. Original accessType name values in VRML97 were eventIn, eventOut, field, exposedField respectively. This list is bounded, no additional values are allowed.
enum class AccessTypeChoices {
    INITIALIZEONLY,  // "initializeOnly"
    INPUTONLY,  // "inputOnly"
    OUTPUTONLY,  // "outputOnly"
    INPUTOUTPUT,  // "inputOutput"
};

// AlphaModeChoices: Permitted values for Appearance alphaMode field. This list is bounded, no additional values are allowed.
enum class AlphaModeChoices {
    AUTO,  // "AUTO"
    OPAQUE,  // "OPAQUE"
    MASK,  // "MASK"
    BLEND,  // "BLEND"
};

// AppliedParametersChoices: Default global parameters for collision outputs of rigid body physics system. Contact node can override parent CollisionCollection node. Multiple optional values, but appliedParameters enumerations cannot be extended. This list is bounded, no additional values are allowed.
enum class AppliedParametersChoices {
    BOUNCE,  // ""BOUNCE""
    USER_FRICTION,  // ""USER_FRICTION""
    FRICTION_COEFFICIENT_2,  // ""FRICTION_COEFFICIENT-2""
    ERROR_REDUCTION,  // ""ERROR_REDUCTION""
    CONSTANT_FORCE,  // ""CONSTANT_FORCE""
    SPEED_1,  // ""SPEED-1""
    SPEED_2,  // ""SPEED-2""
    SLIP_1,  // ""SLIP-1""
    SLIP_2,  // ""SLIP-2""
};

// BiquadTypeFilterChoices: Permitted values for BiquadFilter type. X3D enumeration naming conventions are capitalized versions of Web Audio API enumerations, also changing hyphens to underscores. This list is bounded, no additional values are allowed.
enum class BiquadTypeFilterChoices {
    LOWPASS,  // "LOWPASS"
    HIGHPASS,  // "HIGHPASS"
    BANDPASS,  // "BANDPASS"
    LOWSHELF,  // "LOWSHELF"
    HIGHSHELF,  // "HIGHSHELF"
    PEAKING,  // "PEAKING"
    NOTCH,  // "NOTCH"
    ALLPASS,  // "ALLPASS"
};

// ChannelCountModeChoices: Permitted values for channelCountMode in X3DSoundChannelNode and X3DSoundDestinationNode. X3D enumeration naming conventions are capitalized versions of Web Audio API enumerations, also changing hyphens to underscores. This list is bounded, no additional values are allowed.
enum class ChannelCountModeChoices {
    MAX,  // "MAX"
    CLAMPED_MAX,  // "CLAMPED_MAX"
    EXPLICIT,  // "EXPLICIT"
};

// ChannelInterpretationChoices: Permitted values for channelInterpretation in X3DSoundChannelNode and X3DSoundDestinationNode. X3D enumeration naming conventions are capitalized versions of Web Audio API enumerations, also changing hyphens to underscores. This list is bounded, no additional values are allowed.
enum class ChannelInterpretationChoices {
    SPEAKERS,  // "SPEAKERS"
    DISCRETE,  // "DISCRETE"
};

// ClosureTypeChoices: closureTypeChoices are strictly allowed enumeration values for ArcClose2D closureType field. This list is bounded, no additional values are allowed.
enum class ClosureTypeChoices {
    PIE,  // "PIE"
    CHORD,  // "CHORD"
};

// ComponentNameChoices: componentNameChoices are enumeration constants used to identify the profile for each scene-graph node, and also utilized by X3D element to identify the components required by the contained Scene. This list is bounded, no additional values are allowed.
enum class ComponentNameChoices {
    CORE,  // "Core"
    CADGEOMETRY,  // "CADGeometry"
    CUBEMAPTEXTURING,  // "CubeMapTexturing"
    DIS,  // "DIS"
    ENVIRONMENTALEFFECTS,  // "EnvironmentalEffects"
    ENVIRONMENTALSENSOR,  // "EnvironmentalSensor"
    EVENTUTILITIES,  // "EventUtilities"
    FOLLOWERS,  // "Followers"
    GEOMETRY2D,  // "Geometry2D"
    GEOMETRY3D,  // "Geometry3D"
    GEOSPATIAL,  // "Geospatial"
    GROUPING,  // "Grouping"
    HANIM,  // "HAnim"
    H_ANIM,  // "H-Anim"
    INTERPOLATION,  // "Interpolation"
    KEYDEVICESENSOR,  // "KeyDeviceSensor"
    LAYERING,  // "Layering"
    LAYOUT,  // "Layout"
    LIGHTING,  // "Lighting"
    NAVIGATION,  // "Navigation"
    NETWORKING,  // "Networking"
    NURBS,  // "NURBS"
    PARTICLESYSTEMS,  // "ParticleSystems"
    PICKING,  // "Picking"
    POINTINGDEVICESENSOR,  // "PointingDeviceSensor"
    TEXTUREPROJECTION,  // "TextureProjection"
    RENDERING,  // "Rendering"
    RIGIDBODYPHYSICS,  // "RigidBodyPhysics"
    SCRIPTING,  // "Scripting"
    SHADERS,  // "Shaders"
    SHAPE,  // "Shape"
    SOUND,  // "Sound"
    TEXT,  // "Text"
    TEXTURING,  // "Texturing"
    TEXTURING3D,  // "Texturing3D"
    TIME,  // "Time"
    VOLUMERENDERING,  // "VolumeRendering"
};

// DistanceModelChoices: Permitted values for SpatialSound distanceModel. X3D enumeration naming conventions are capitalized versions of Web Audio API enumerations, also changing hyphens to underscores. This list is bounded, no additional values are allowed.
enum class DistanceModelChoices {
    LINEAR,  // "LINEAR"
    INVERSE,  // "INVERSE"
    EXPONENTIAL,  // "EXPONENTIAL"
};

// FieldTypeChoices: fieldTypeChoices are enumerations for all allowed names of X3DField types. This list is bounded, no additional values are allowed.
enum class FieldTypeChoices {
    SFBOOL,  // "SFBool"
    MFBOOL,  // "MFBool"
    SFCOLOR,  // "SFColor"
    MFCOLOR,  // "MFColor"
    SFCOLORRGBA,  // "SFColorRGBA"
    MFCOLORRGBA,  // "MFColorRGBA"
    SFDOUBLE,  // "SFDouble"
    MFDOUBLE,  // "MFDouble"
    SFFLOAT,  // "SFFloat"
    MFFLOAT,  // "MFFloat"
    SFIMAGE,  // "SFImage"
    MFIMAGE,  // "MFImage"
    SFINT32,  // "SFInt32"
    MFINT32,  // "MFInt32"
    SFNODE,  // "SFNode"
    MFNODE,  // "MFNode"
    SFROTATION,  // "SFRotation"
    MFROTATION,  // "MFRotation"
    SFSTRING,  // "SFString"
    MFSTRING,  // "MFString"
    SFTIME,  // "SFTime"
    MFTIME,  // "MFTime"
    SFVEC2D,  // "SFVec2d"
    MFVEC2D,  // "MFVec2d"
    SFVEC2F,  // "SFVec2f"
    MFVEC2F,  // "MFVec2f"
    SFVEC3D,  // "SFVec3d"
    MFVEC3D,  // "MFVec3d"
    SFVEC3F,  // "SFVec3f"
    MFVEC3F,  // "MFVec3f"
    SFVEC4D,  // "SFVec4d"
    MFVEC4D,  // "MFVec4d"
    SFVEC4F,  // "SFVec4f"
    MFVEC4F,  // "MFVec4f"
    SFMATRIX3D,  // "SFMatrix3d"
    MFMATRIX3D,  // "MFMatrix3d"
    SFMATRIX3F,  // "SFMatrix3f"
    MFMATRIX3F,  // "MFMatrix3f"
    SFMATRIX4D,  // "SFMatrix4d"
    MFMATRIX4D,  // "MFMatrix4d"
    SFMATRIX4F,  // "SFMatrix4f"
    MFMATRIX4F,  // "MFMatrix4f"
};

// FogTypeChoices: fogTypeChoices are strictly allowed enumeration values for Fog node fogType field. This list is bounded, no additional values are allowed.
enum class FogTypeChoices {
    LINEAR,  // "LINEAR"
    EXPONENTIAL,  // "EXPONENTIAL"
};

// FontFamilyValues: fontFamilyValues are supported enumeration values for FontStyle/ScreenFontStyle node family field. This list is unbounded, additional enumeration values are allowed.
enum class FontFamilyValues {
    SANS,  // ""SANS""
    SERIF,  // ""SERIF""
    TYPEWRITER,  // ""TYPEWRITER""
};

// FontStyleChoices: fontStyleChoices are strictly allowed enumeration values for FontStyle/ScreenFontStyle node style field. This list is bounded, no additional values are allowed.
enum class FontStyleChoices {
    PLAIN,  // "PLAIN"
    BOLD,  // "BOLD"
    ITALIC,  // "ITALIC"
    BOLDITALIC,  // "BOLDITALIC"
};

// ForceOutputValues: forceOutputValues are supported enumeration values for X3DRigidJointNode type forceOutput field. This list is unbounded, additional enumeration values are allowed.
enum class ForceOutputValues {
    ALL,  // ""ALL""
    NONE,  // ""NONE""
};

// GeneratedCubeMapTextureUpdateChoices: generatedCubeMapTextureUpdateChoices are strictly allowed enumeration values for GeneratedCubeMapTexture field named 'update'. This list is bounded, no additional values are allowed.
enum class GeneratedCubeMapTextureUpdateChoices {
    NONE,  // "NONE"
    NEXT_FRAME_ONLY,  // "NEXT_FRAME_ONLY"
    ALWAYS,  // "ALWAYS"
};

// GeoMetadataSummaryKeyValues: The enumeration keys for the GeoMetadata summary field. This list is unbounded, additional enumeration values are allowed.
enum class GeoMetadataSummaryKeyValues {
    TITLE,  // "title"
    DESCRIPTION,  // "description"
    COORDINATESYSTEM,  // "coordinateSystem"
    HORIZONTALDATUM,  // "horizontalDatum"
    VERTICALDATUM,  // "verticalDatum"
    ELLIPSOID,  // "ellipsoid"
    EXTENT,  // "extent"
    RESOLUTION,  // "resolution"
    ORIGINATOR,  // "originator"
    COPYRIGHT,  // "copyright"
    DATE,  // "date"
    METADATAFORMAT,  // "metadataFormat"
    DATAURL,  // "dataUrl"
    DATAFORMAT,  // "dataFormat"
};

// GeoSystemEarthEllipsoidValues: geoSystemEarthEllipsoidValues are allowed enumeration values of earth ellipsoids, providing second value for any geoSystem field that has first value GD. This list is unbounded, additional enumeration values are allowed..
enum class GeoSystemEarthEllipsoidValues {
    AM,  // "AM"
    AN,  // "AN"
    BN,  // "BN"
    BR,  // "BR"
    CC,  // "CC"
    CD,  // "CD"
    EA,  // "EA"
    EB,  // "EB"
    EC,  // "EC"
    ED,  // "ED"
    EE,  // "EE"
    EF,  // "EF"
    FA,  // "FA"
    HE,  // "HE"
    HO,  // "HO"
    ID,  // "ID"
    IN,  // "IN"
    KA,  // "KA"
    RF,  // "RF"
    SA,  // "SA"
    WD,  // "WD"
    WE,  // "WE"
    WGS84,  // "WGS84"
    ZN,  // "Zn"
    S,  // "S"
};

// GeoSystemSpatialReferenceFrameValues: geoSystemSpatialReferenceFrameValues are allowed enumeration values of spatial reference frames and earth ellipsoids, providing first value for geoSystem field. This list is unbounded, additional enumeration values are allowed.
enum class GeoSystemSpatialReferenceFrameValues {
    GD,  // "GD"
    UTM,  // "UTM"
    GC,  // "GC"
    GDC,  // "GDC"
    GCC,  // "GCC"
};

// HanimHumanoidInfoKeyValues: Enumerated keyword names for keyword=value metadata pairs describing a given HAnimHumanoid. Additional keyword=value pairs may be included as needed for specific applications. This list is unbounded, additional enumeration values are allowed.
enum class HanimHumanoidInfoKeyValues {
    AUTHORNAME,  // "authorName"
    AUTHOREMAIL,  // "authorEmail"
    COPYRIGHT,  // "copyright"
    CREATIONDATE,  // "creationDate"
    USAGERESTRICTIONS,  // "usageRestrictions"
    HUMANOIDVERSION,  // "humanoidVersion"
    AGE,  // "age"
    GENDER,  // "gender"
    HEIGHT,  // "height"
    WEIGHT,  // "weight"
};

// HanimJointNameValues: CAESAR joint names for identification of HAnimJoint nodes as defined in the HAnim Architecture specification. This list is unbounded, additional enumeration values are allowed.
enum class HanimJointNameValues {
    HUMANOID_ROOT,  // "humanoid_root"
    SACROILIAC,  // "sacroiliac"
    L_HIP,  // "l_hip"
    L_KNEE,  // "l_knee"
    L_TALOCRURAL,  // "l_talocrural"
    L_TALOCALCANEONAVICULAR,  // "l_talocalcaneonavicular"
    L_CUNEONAVICULAR_1,  // "l_cuneonavicular_1"
    L_TARSOMETATARSAL_1,  // "l_tarsometatarsal_1"
    L_METATARSOPHALANGEAL_1,  // "l_metatarsophalangeal_1"
    L_TARSAL_INTERPHALANGEAL_1,  // "l_tarsal_interphalangeal_1"
    L_CUNEONAVICULAR_2,  // "l_cuneonavicular_2"
    L_TARSOMETATARSAL_2,  // "l_tarsometatarsal_2"
    L_METATARSOPHALANGEAL_2,  // "l_metatarsophalangeal_2"
    L_TARSAL_PROXIMAL_INTERPHALANGEAL_2,  // "l_tarsal_proximal_interphalangeal_2"
    L_TARSAL_DISTAL_INTERPHALANGEAL_2,  // "l_tarsal_distal_interphalangeal_2"
    L_CUNEONAVICULAR_3,  // "l_cuneonavicular_3"
    L_TARSOMETATARSAL_3,  // "l_tarsometatarsal_3"
    L_METATARSOPHALANGEAL_3,  // "l_metatarsophalangeal_3"
    L_TARSAL_PROXIMAL_INTERPHALANGEAL_3,  // "l_tarsal_proximal_interphalangeal_3"
    L_TARSAL_DISTAL_INTERPHALANGEAL_3,  // "l_tarsal_distal_interphalangeal_3"
    L_CALCANEOCUBOID,  // "l_calcaneocuboid"
    L_TRANSVERSETARSAL,  // "l_transversetarsal"
    L_TARSOMETATARSAL_4,  // "l_tarsometatarsal_4"
    L_METATARSOPHALANGEAL_4,  // "l_metatarsophalangeal_4"
    L_TARSAL_PROXIMAL_INTERPHALANGEAL_4,  // "l_tarsal_proximal_interphalangeal_4"
    L_TARSAL_DISTAL_INTERPHALANGEAL_4,  // "l_tarsal_distal_interphalangeal_4"
    L_TARSOMETATARSAL_5,  // "l_tarsometatarsal_5"
    L_METATARSOPHALANGEAL_5,  // "l_metatarsophalangeal_5"
    L_TARSAL_PROXIMAL_INTERPHALANGEAL_5,  // "l_tarsal_proximal_interphalangeal_5"
    L_TARSAL_DISTAL_INTERPHALANGEAL_5,  // "l_tarsal_distal_interphalangeal_5"
    R_HIP,  // "r_hip"
    R_KNEE,  // "r_knee"
    R_TALOCRURAL,  // "r_talocrural"
    R_TALOCALCANEONAVICULAR,  // "r_talocalcaneonavicular"
    R_CUNEONAVICULAR_1,  // "r_cuneonavicular_1"
    R_TARSOMETATARSAL_1,  // "r_tarsometatarsal_1"
    R_METATARSOPHALANGEAL_1,  // "r_metatarsophalangeal_1"
    R_TARSAL_INTERPHALANGEAL_1,  // "r_tarsal_interphalangeal_1"
    R_CUNEONAVICULAR_2,  // "r_cuneonavicular_2"
    R_TARSOMETATARSAL_2,  // "r_tarsometatarsal_2"
    R_METATARSOPHALANGEAL_2,  // "r_metatarsophalangeal_2"
    R_TARSAL_PROXIMAL_INTERPHALANGEAL_2,  // "r_tarsal_proximal_interphalangeal_2"
    R_TARSAL_DISTAL_INTERPHALANGEAL_2,  // "r_tarsal_distal_interphalangeal_2"
    R_CUNEONAVICULAR_3,  // "r_cuneonavicular_3"
    R_TARSOMETATARSAL_3,  // "r_tarsometatarsal_3"
    R_METATARSOPHALANGEAL_3,  // "r_metatarsophalangeal_3"
    R_TARSAL_PROXIMAL_INTERPHALANGEAL_3,  // "r_tarsal_proximal_interphalangeal_3"
    R_TARSAL_DISTAL_INTERPHALANGEAL_3,  // "r_tarsal_distal_interphalangeal_3"
    R_CALCANEOCUBOID,  // "r_calcaneocuboid"
    R_TRANSVERSETARSAL,  // "r_transversetarsal"
    R_TARSOMETATARSAL_4,  // "r_tarsometatarsal_4"
    R_METATARSOPHALANGEAL_4,  // "r_metatarsophalangeal_4"
    R_TARSAL_PROXIMAL_INTERPHALANGEAL_4,  // "r_tarsal_proximal_interphalangeal_4"
    R_TARSAL_DISTAL_INTERPHALANGEAL_4,  // "r_tarsal_distal_interphalangeal_4"
    R_TARSOMETATARSAL_5,  // "r_tarsometatarsal_5"
    R_METATARSOPHALANGEAL_5,  // "r_metatarsophalangeal_5"
    R_TARSAL_PROXIMAL_INTERPHALANGEAL_5,  // "r_tarsal_proximal_interphalangeal_5"
    R_TARSAL_DISTAL_INTERPHALANGEAL_5,  // "r_tarsal_distal_interphalangeal_5"
    VL5,  // "vl5"
    VL4,  // "vl4"
    VL3,  // "vl3"
    VL2,  // "vl2"
    VL1,  // "vl1"
    VT12,  // "vt12"
    VT11,  // "vt11"
    VT10,  // "vt10"
    VT9,  // "vt9"
    VT8,  // "vt8"
    VT7,  // "vt7"
    VT6,  // "vt6"
    VT5,  // "vt5"
    VT4,  // "vt4"
    VT3,  // "vt3"
    VT2,  // "vt2"
    VT1,  // "vt1"
    VC7,  // "vc7"
    VC6,  // "vc6"
    VC5,  // "vc5"
    VC4,  // "vc4"
    VC3,  // "vc3"
    VC2,  // "vc2"
    VC1,  // "vc1"
    SKULLBASE,  // "skullbase"
    L_EYELID_JOINT,  // "l_eyelid_joint"
    R_EYELID_JOINT,  // "r_eyelid_joint"
    L_EYEBALL_JOINT,  // "l_eyeball_joint"
    R_EYEBALL_JOINT,  // "r_eyeball_joint"
    L_EYEBROW_JOINT,  // "l_eyebrow_joint"
    R_EYEBROW_JOINT,  // "r_eyebrow_joint"
    TEMPOROMANDIBULAR,  // "temporomandibular"
    L_STERNOCLAVICULAR,  // "l_sternoclavicular"
    L_ACROMIOCLAVICULAR,  // "l_acromioclavicular"
    L_SHOULDER,  // "l_shoulder"
    L_ELBOW,  // "l_elbow"
    L_RADIOCARPAL,  // "l_radiocarpal"
    L_MIDCARPAL_1,  // "l_midcarpal_1"
    L_CARPOMETACARPAL_1,  // "l_carpometacarpal_1"
    L_METACARPOPHALANGEAL_1,  // "l_metacarpophalangeal_1"
    L_CARPAL_INTERPHALANGEAL_1,  // "l_carpal_interphalangeal_1"
    L_MIDCARPAL_2,  // "l_midcarpal_2"
    L_CARPOMETACARPAL_2,  // "l_carpometacarpal_2"
    L_METACARPOPHALANGEAL_2,  // "l_metacarpophalangeal_2"
    L_CARPAL_PROXIMAL_INTERPHALANGEAL_2,  // "l_carpal_proximal_interphalangeal_2"
    L_CARPAL_DISTAL_INTERPHALANGEAL_2,  // "l_carpal_distal_interphalangeal_2"
    L_MIDCARPAL_3,  // "l_midcarpal_3"
    L_CARPOMETACARPAL_3,  // "l_carpometacarpal_3"
    L_METACARPOPHALANGEAL_3,  // "l_metacarpophalangeal_3"
    L_CARPAL_PROXIMAL_INTERPHALANGEAL_3,  // "l_carpal_proximal_interphalangeal_3"
    L_CARPAL_DISTAL_INTERPHALANGEAL_3,  // "l_carpal_distal_interphalangeal_3"
    L_MIDCARPAL_4_5,  // "l_midcarpal_4_5"
    L_CARPOMETACARPAL_4,  // "l_carpometacarpal_4"
    L_METACARPOPHALANGEAL_4,  // "l_metacarpophalangeal_4"
    L_CARPAL_PROXIMAL_INTERPHALANGEAL_4,  // "l_carpal_proximal_interphalangeal_4"
    L_CARPAL_DISTAL_INTERPHALANGEAL_4,  // "l_carpal_distal_interphalangeal_4"
    L_CARPOMETACARPAL_5,  // "l_carpometacarpal_5"
    L_METACARPOPHALANGEAL_5,  // "l_metacarpophalangeal_5"
    L_CARPAL_PROXIMAL_INTERPHALANGEAL_5,  // "l_carpal_proximal_interphalangeal_5"
    L_CARPAL_DISTAL_INTERPHALANGEAL_5,  // "l_carpal_distal_interphalangeal_5"
    R_STERNOCLAVICULAR,  // "r_sternoclavicular"
    R_ACROMIOCLAVICULAR,  // "r_acromioclavicular"
    R_SHOULDER,  // "r_shoulder"
    R_ELBOW,  // "r_elbow"
    R_RADIOCARPAL,  // "r_radiocarpal"
    R_MIDCARPAL_1,  // "r_midcarpal_1"
    R_CARPOMETACARPAL_1,  // "r_carpometacarpal_1"
    R_METACARPOPHALANGEAL_1,  // "r_metacarpophalangeal_1"
    R_CARPAL_INTERPHALANGEAL_1,  // "r_carpal_interphalangeal_1"
    R_MIDCARPAL_2,  // "r_midcarpal_2"
    R_CARPOMETACARPAL_2,  // "r_carpometacarpal_2"
    R_METACARPOPHALANGEAL_2,  // "r_metacarpophalangeal_2"
    R_CARPAL_PROXIMAL_INTERPHALANGEAL_2,  // "r_carpal_proximal_interphalangeal_2"
    R_CARPAL_DISTAL_INTERPHALANGEAL_2,  // "r_carpal_distal_interphalangeal_2"
    R_MIDCARPAL_3,  // "r_midcarpal_3"
    R_CARPOMETACARPAL_3,  // "r_carpometacarpal_3"
    R_METACARPOPHALANGEAL_3,  // "r_metacarpophalangeal_3"
    R_CARPAL_PROXIMAL_INTERPHALANGEAL_3,  // "r_carpal_proximal_interphalangeal_3"
    R_CARPAL_DISTAL_INTERPHALANGEAL_3,  // "r_carpal_distal_interphalangeal_3"
    R_MIDCARPAL_4_5,  // "r_midcarpal_4_5"
    R_CARPOMETACARPAL_4,  // "r_carpometacarpal_4"
    R_METACARPOPHALANGEAL_4,  // "r_metacarpophalangeal_4"
    R_CARPAL_PROXIMAL_INTERPHALANGEAL_4,  // "r_carpal_proximal_interphalangeal_4"
    R_CARPAL_DISTAL_INTERPHALANGEAL_4,  // "r_carpal_distal_interphalangeal_4"
    R_CARPOMETACARPAL_5,  // "r_carpometacarpal_5"
    R_METACARPOPHALANGEAL_5,  // "r_metacarpophalangeal_5"
    R_CARPAL_PROXIMAL_INTERPHALANGEAL_5,  // "r_carpal_proximal_interphalangeal_5"
    R_CARPAL_DISTAL_INTERPHALANGEAL_5,  // "r_carpal_distal_interphalangeal_5"
};

// HanimSegmentNameValues: CAESAR segment names for identification of HAnimSegment nodes as defined in the HAnim Architecture specification. This list is unbounded, additional enumeration values are allowed.
enum class HanimSegmentNameValues {
    SACRUM,  // "sacrum"
    PELVIS,  // "pelvis"
    L_THIGH,  // "l_thigh"
    L_CALF,  // "l_calf"
    L_TALUS,  // "l_talus"
    L_NAVICULAR,  // "l_navicular"
    L_CUNEIFORM_1,  // "l_cuneiform_1"
    L_METATARSAL_1,  // "l_metatarsal_1"
    L_TARSAL_PROXIMAL_PHALANX_1,  // "l_tarsal_proximal_phalanx_1"
    L_TARSAL_DISTAL_PHALANX_1,  // "l_tarsal_distal_phalanx_1"
    L_CUNEIFORM_2,  // "l_cuneiform_2"
    L_METATARSAL_2,  // "l_metatarsal_2"
    L_TARSAL_PROXIMAL_PHALANX_2,  // "l_tarsal_proximal_phalanx_2"
    L_TARSAL_MIDDLE_PHALANX_2,  // "l_tarsal_middle_phalanx_2"
    L_TARSAL_DISTAL_PHALANX_2,  // "l_tarsal_distal_phalanx_2"
    L_CUNEIFORM_3,  // "l_cuneiform_3"
    L_METATARSAL_3,  // "l_metatarsal_3"
    L_TARSAL_PROXIMAL_PHALANX_3,  // "l_tarsal_proximal_phalanx_3"
    L_TARSAL_MIDDLE_PHALANX_3,  // "l_tarsal_middle_phalanx_3"
    L_TARSAL_DISTAL_PHALANX_3,  // "l_tarsal_distal_phalanx_3"
    L_CALCANEUS,  // "l_calcaneus"
    L_CUBOID,  // "l_cuboid"
    L_METATARSAL_4,  // "l_metatarsal_4"
    L_TARSAL_PROXIMAL_PHALANX_4,  // "l_tarsal_proximal_phalanx_4"
    L_TARSAL_MIDDLE_PHALANX_4,  // "l_tarsal_middle_phalanx_4"
    L_TARSAL_DISTAL_PHALANX_4,  // "l_tarsal_distal_phalanx_4"
    L_METATARSAL_5,  // "l_metatarsal_5"
    L_TARSAL_PROXIMAL_PHALANX_5,  // "l_tarsal_proximal_phalanx_5"
    L_TARSAL_MIDDLE_PHALANX_5,  // "l_tarsal_middle_phalanx_5"
    L_TARSAL_DISTAL_PHALANX_5,  // "l_tarsal_distal_phalanx_5"
    R_THIGH,  // "r_thigh"
    R_CALF,  // "r_calf"
    R_TALUS,  // "r_talus"
    R_NAVICULAR,  // "r_navicular"
    R_CUNEIFORM_1,  // "r_cuneiform_1"
    R_METATARSAL_1,  // "r_metatarsal_1"
    R_TARSAL_PROXIMAL_PHALANX_1,  // "r_tarsal_proximal_phalanx_1"
    R_TARSAL_DISTAL_PHALANX_1,  // "r_tarsal_distal_phalanx_1"
    R_CUNEIFORM_2,  // "r_cuneiform_2"
    R_METATARSAL_2,  // "r_metatarsal_2"
    R_TARSAL_PROXIMAL_PHALANX_2,  // "r_tarsal_proximal_phalanx_2"
    R_TARSAL_MIDDLE_PHALANX_2,  // "r_tarsal_middle_phalanx_2"
    R_TARSAL_DISTAL_PHALANX_2,  // "r_tarsal_distal_phalanx_2"
    R_CUNEIFORM_3,  // "r_cuneiform_3"
    R_METATARSAL_3,  // "r_metatarsal_3"
    R_TARSAL_PROXIMAL_PHALANX_3,  // "r_tarsal_proximal_phalanx_3"
    R_TARSAL_MIDDLE_PHALANX_3,  // "r_tarsal_middle_phalanx_3"
    R_TARSAL_DISTAL_PHALANX_3,  // "r_tarsal_distal_phalanx_3"
    R_CALCANEUS,  // "r_calcaneus"
    R_CUBOID,  // "r_cuboid"
    R_METATARSAL_4,  // "r_metatarsal_4"
    R_TARSAL_PROXIMAL_PHALANX_4,  // "r_tarsal_proximal_phalanx_4"
    R_TARSAL_MIDDLE_PHALANX_4,  // "r_tarsal_middle_phalanx_4"
    R_TARSAL_DISTAL_PHALANX_4,  // "r_tarsal_distal_phalanx_4"
    R_METATARSAL_5,  // "r_metatarsal_5"
    R_TARSAL_PROXIMAL_PHALANX_5,  // "r_tarsal_proximal_phalanx_5"
    R_TARSAL_MIDDLE_PHALANX_5,  // "r_tarsal_middle_phalanx_5"
    R_TARSAL_DISTAL_PHALANX_5,  // "r_tarsal_distal_phalanx_5"
    L5,  // "l5"
    L4,  // "l4"
    L3,  // "l3"
    L2,  // "l2"
    L1,  // "l1"
    T12,  // "t12"
    T11,  // "t11"
    T10,  // "t10"
    T9,  // "t9"
    T8,  // "t8"
    T7,  // "t7"
    T6,  // "t6"
    T5,  // "t5"
    T4,  // "t4"
    T3,  // "t3"
    T2,  // "t2"
    T1,  // "t1"
    C7,  // "c7"
    C6,  // "c6"
    C5,  // "c5"
    C4,  // "c4"
    C3,  // "c3"
    C2,  // "c2"
    C1,  // "c1"
    SKULL,  // "skull"
    L_EYELID,  // "l_eyelid"
    R_EYELID,  // "r_eyelid"
    L_EYEBALL,  // "l_eyeball"
    R_EYEBALL,  // "r_eyeball"
    L_EYEBROW,  // "l_eyebrow"
    R_EYEBROW,  // "r_eyebrow"
    JAW,  // "jaw"
    L_CLAVICLE,  // "l_clavicle"
    L_SCAPULA,  // "l_scapula"
    L_UPPERARM,  // "l_upperarm"
    L_FOREARM,  // "l_forearm"
    L_CARPAL,  // "l_carpal"
    L_TRAPEZIUM,  // "l_trapezium"
    L_METACARPAL_1,  // "l_metacarpal_1"
    L_CARPAL_PROXIMAL_PHALANX_1,  // "l_carpal_proximal_phalanx_1"
    L_CARPAL_DISTAL_PHALANX_1,  // "l_carpal_distal_phalanx_1"
    L_TRAPEZOID,  // "l_trapezoid"
    L_METACARPAL_2,  // "l_metacarpal_2"
    L_CARPAL_PROXIMAL_PHALANX_2,  // "l_carpal_proximal_phalanx_2"
    L_CARPAL_MIDDLE_PHALANX_2,  // "l_carpal_middle_phalanx_2"
    L_CARPAL_DISTAL_PHALANX_2,  // "l_carpal_distal_phalanx_2"
    L_CAPITATE,  // "l_capitate"
    L_METACARPAL_3,  // "l_metacarpal_3"
    L_CARPAL_PROXIMAL_PHALANX_3,  // "l_carpal_proximal_phalanx_3"
    L_CARPAL_MIDDLE_PHALANX_3,  // "l_carpal_middle_phalanx_3"
    L_CARPAL_DISTAL_PHALANX_3,  // "l_carpal_distal_phalanx_3"
    L_HAMATE,  // "l_hamate"
    L_METACARPAL_4,  // "l_metacarpal_4"
    L_CARPAL_PROXIMAL_PHALANX_4,  // "l_carpal_proximal_phalanx_4"
    L_CARPAL_MIDDLE_PHALANX_4,  // "l_carpal_middle_phalanx_4"
    L_CARPAL_DISTAL_PHALANX_4,  // "l_carpal_distal_phalanx_4"
    L_METACARPAL_5,  // "l_metacarpal_5"
    L_CARPAL_PROXIMAL_PHALANX_5,  // "l_carpal_proximal_phalanx_5"
    L_CARPAL_MIDDLE_PHALANX_5,  // "l_carpal_middle_phalanx_5"
    L_CARPAL_DISTAL_PHALANX_5,  // "l_carpal_distal_phalanx_5"
    R_CLAVICLE,  // "r_clavicle"
    R_SCAPULA,  // "r_scapula"
    R_UPPERARM,  // "r_upperarm"
    R_FOREARM,  // "r_forearm"
    R_CARPAL,  // "r_carpal"
    R_TRAPEZIUM,  // "r_trapezium"
    R_METACARPAL_1,  // "r_metacarpal_1"
    R_CARPAL_PROXIMAL_PHALANX_1,  // "r_carpal_proximal_phalanx_1"
    R_CARPAL_DISTAL_PHALANX_1,  // "r_carpal_distal_phalanx_1"
    R_TRAPEZOID,  // "r_trapezoid"
    R_METACARPAL_2,  // "r_metacarpal_2"
    R_CARPAL_PROXIMAL_PHALANX_2,  // "r_carpal_proximal_phalanx_2"
    R_CARPAL_MIDDLE_PHALANX_2,  // "r_carpal_middle_phalanx_2"
    R_CARPAL_DISTAL_PHALANX_2,  // "r_carpal_distal_phalanx_2"
    R_CAPITATE,  // "r_capitate"
    R_METACARPAL_3,  // "r_metacarpal_3"
    R_CARPAL_PROXIMAL_PHALANX_3,  // "r_carpal_proximal_phalanx_3"
    R_CARPAL_MIDDLE_PHALANX_3,  // "r_carpal_middle_phalanx_3"
    R_CARPAL_DISTAL_PHALANX_3,  // "r_carpal_distal_phalanx_3"
    R_HAMATE,  // "r_hamate"
    R_METACARPAL_4,  // "r_metacarpal_4"
    R_CARPAL_PROXIMAL_PHALANX_4,  // "r_carpal_proximal_phalanx_4"
    R_CARPAL_MIDDLE_PHALANX_4,  // "r_carpal_middle_phalanx_4"
    R_CARPAL_DISTAL_PHALANX_4,  // "r_carpal_distal_phalanx_4"
    R_METACARPAL_5,  // "r_metacarpal_5"
    R_CARPAL_PROXIMAL_PHALANX_5,  // "r_carpal_proximal_phalanx_5"
    R_CARPAL_MIDDLE_PHALANX_5,  // "r_carpal_middle_phalanx_5"
    R_CARPAL_DISTAL_PHALANX_5,  // "r_carpal_distal_phalanx_5"
};

// HanimVersionChoices: hanimVersionChoices enumeration constants are used to identify the allowed versions for an HAnimHumanoid node. Note that default HAnimHumanoid version is 2.0 for X3D version 4, and default is 1.0 for X3D version 3. This list is bounded, no additional values are allowed.
enum class HanimVersionChoices {
    _2_0,  // "2.0"
};

// IntersectionTypeValues: intersectionTypeValues are supported enumeration values for X3DPickSensorNode type intersectionType field. This list is unbounded, additional enumeration values are allowed.
enum class IntersectionTypeValues {
    BOUNDS,  // "BOUNDS"
    GEOMETRY,  // "GEOMETRY"
};

// JustifyChoices: justifyChoices are strictly allowed enumeration values for justify field in FontStyle and ScreenFontStyle nodes determining major and minor axis alignment (typically horizontal and vertical) of text layout. Note that intermediate commas and extraneous whitespace are disallowed by these strictly defined enumeration values. This list is bounded, no additional values are allowed.
enum class JustifyChoices {
    MIDDLE,  // ""MIDDLE""
    MIDDLE_BEGIN,  // ""MIDDLE" "BEGIN""
    MIDDLE_END,  // ""MIDDLE" "END""
    MIDDLE_FIRST,  // ""MIDDLE" "FIRST""
    MIDDLE_MIDDLE,  // ""MIDDLE" "MIDDLE""
    BEGIN,  // ""BEGIN""
    BEGIN_BEGIN,  // ""BEGIN" "BEGIN""
    BEGIN_END,  // ""BEGIN" "END""
    BEGIN_FIRST,  // ""BEGIN" "FIRST""
    BEGIN_MIDDLE,  // ""BEGIN" "MIDDLE""
    END,  // ""END""
    END_BEGIN,  // ""END" "BEGIN""
    END_END,  // ""END" "END""
    END_FIRST,  // ""END" "FIRST""
    END_MIDDLE,  // ""END" "MIDDLE""
    FIRST,  // ""FIRST""
    FIRST_BEGIN,  // ""FIRST" "BEGIN""
    FIRST_END,  // ""FIRST" "END""
    FIRST_FIRST,  // ""FIRST" "FIRST""
    FIRST_MIDDLE,  // ""FIRST" "MIDDLE""
};

// LayoutAlignChoices: Permitted combinations of horizontal and vertical values for the align field in the Layout node. Note that intermediate commas and extraneous whitespace are disallowed by these strictly defined enumeration values. This list is bounded, no additional values are allowed.
enum class LayoutAlignChoices {
    LEFT_BOTTOM,  // ""LEFT" "BOTTOM""
    LEFT_CENTER,  // ""LEFT" "CENTER""
    LEFT_TOP,  // ""LEFT" "TOP""
    CENTER_BOTTOM,  // ""CENTER" "BOTTOM""
    CENTER_CENTER,  // ""CENTER" "CENTER""
    CENTER_TOP,  // ""CENTER" "TOP""
    RIGHT_BOTTOM,  // ""RIGHT" "BOTTOM""
    RIGHT_CENTER,  // ""RIGHT" "CENTER""
    RIGHT_TOP,  // ""RIGHT" "TOP""
};

// LayoutScaleModeChoices: Permitted combinations of horizontal and vertical values for the scaleMode field in the Layout node. Note that intermediate commas and extraneous whitespace are disallowed by these strictly defined enumeration values. This list is bounded, no additional values are allowed.
enum class LayoutScaleModeChoices {
    NONE_NONE,  // ""NONE" "NONE""
    NONE_FRACTION,  // ""NONE" "FRACTION""
    NONE_STRETCH,  // ""NONE" "STRETCH""
    NONE_PIXEL,  // ""NONE" "PIXEL""
    FRACTION_NONE,  // ""FRACTION" "NONE""
    FRACTION_FRACTION,  // ""FRACTION" "FRACTION""
    FRACTION_STRETCH,  // ""FRACTION" "STRETCH""
    FRACTION_PIXEL,  // ""FRACTION" "PIXEL""
    STRETCH_NONE,  // ""STRETCH" "NONE""
    STRETCH_FRACTION,  // ""STRETCH" "FRACTION""
    STRETCH_STRETCH,  // ""STRETCH" "STRETCH""
    STRETCH_PIXEL,  // ""STRETCH" "PIXEL""
    PIXEL_NONE,  // ""PIXEL" "NONE""
    PIXEL_FRACTION,  // ""PIXEL" "FRACTION""
    PIXEL_STRETCH,  // ""PIXEL" "STRETCH""
    PIXEL_PIXEL,  // ""PIXEL" "PIXEL""
};

// LayoutUnitsChoices: Permitted combinations of horizontal and vertical values for the offsetUnits field in the Layout node. Note that intermediate commas and extraneous whitespace are disallowed by these strictly defined enumeration values. This list is bounded, no additional values are allowed.
enum class LayoutUnitsChoices {
    WORLD_WORLD,  // ""WORLD" "WORLD""
    WORLD_FRACTION,  // ""WORLD" "FRACTION""
    WORLD_PIXEL,  // ""WORLD" "PIXEL""
    FRACTION_WORLD,  // ""FRACTION" "WORLD""
    FRACTION_FRACTION,  // ""FRACTION" "FRACTION""
    FRACTION_PIXEL,  // ""FRACTION" "PIXEL""
    PIXEL_WORLD,  // ""PIXEL" "WORLD""
    PIXEL_FRACTION,  // ""PIXEL" "FRACTION""
    PIXEL_PIXEL,  // ""PIXEL" "PIXEL""
};

// MetaDirectionChoices: metaDirectionChoices are strictly allowed enumeration values for meta element direction field. This list is bounded, no additional values are allowed.
enum class MetaDirectionChoices {
    RTL,  // "rtl"
    LTR,  // "ltr"
};

// MetaNameValues: metaNameValues are supported enumeration values for meta element name field. This list is unbounded, additional enumeration values are allowed.
enum class MetaNameValues {
    ACCESSRIGHTS,  // "accessRights"
    AUTHOR,  // "author"
    CML_VERSION,  // "CML-version"
    CONTRIBUTOR,  // "contributor"
    CREATED,  // "created"
    CREATOR,  // "creator"
    DESCRIPTION,  // "description"
    DISCLAIMER,  // "disclaimer"
    DRAWING,  // "drawing"
    ERROR,  // "error"
    GENERATOR,  // "generator"
    HINT,  // "hint"
    IDENTIFIER,  // "identifier"
    IMAGE,  // "Image"
    INFO,  // "info"
    INFORMATION,  // "information"
    ISVERSIONOF,  // "isVersionOf"
    KEYWORDS,  // "keywords"
    LICENSE,  // "license"
    MEDIATOR,  // "mediator"
    MODIFIED,  // "modified"
    MOVIE,  // "movie"
    MOVINGIMAGE,  // "MovingImage"
    ORIGINAL,  // "original"
    PHOTO,  // "photo"
    PHOTOGRAPH,  // "photograph"
    PUBLISHER,  // "publisher"
    REFERENCE,  // "reference"
    REQUIRES,  // "requires"
    RIGHTS,  // "rights"
    ROBOTS,  // "robots"
    SOUND,  // "Sound"
    SOURCE,  // "source"
    SPECIFICATIONSECTION,  // "specificationSection"
    SPECIFICATIONURL,  // "specificationUrl"
    SUBJECT,  // "subject"
    TEXT,  // "Text"
    TITLE,  // "title"
    TODO,  // "TODO"
    TRANSLATOR,  // "translator"
    TRANSLATED,  // "translated"
    VERSION,  // "version"
    WARNING,  // "warning"
};

// NavigationTransitionTypeValues: navigationTransitionTypeValues are supported enumeration values for the transitionType field in the NavigationInfo node. This list is unbounded, additional enumeration values are allowed.
enum class NavigationTransitionTypeValues {
    TELEPORT,  // ""TELEPORT""
    LINEAR,  // ""LINEAR""
    ANIMATE,  // ""ANIMATE""
};

// NavigationTypeValues: navigationTypeValues are supported enumeration values for the type field in the NavigationInfo node. This list is unbounded, additional enumeration values are allowed.
enum class NavigationTypeValues {
    ANY,  // ""ANY""
    WALK,  // ""WALK""
    EXAMINE,  // ""EXAMINE""
    FLY,  // ""FLY""
    LOOKAT,  // ""LOOKAT""
    NONE,  // ""NONE""
    EXPLORE,  // ""EXPLORE""
};

// NetworkModeChoices: networkModeChoices are strictly allowed enumeration values for DIS field networkMode. This list is bounded, no additional values are allowed.
enum class NetworkModeChoices {
    STANDALONE,  // "standAlone"
    NETWORKREADER,  // "networkReader"
    NETWORKWRITER,  // "networkWriter"
};

// ParticleSystemGeometryTypeValues: particleSystemGeometryTypeValues are supported enumeration values for the ParticleSystem node geometryType field. This list is unbounded, additional enumeration values are allowed.
enum class ParticleSystemGeometryTypeValues {
    LINE,  // "LINE"
    POINT,  // "POINT"
    QUAD,  // "QUAD"
    SPRITE,  // "SPRITE"
    TRIANGLE,  // "TRIANGLE"
    GEOMETRY,  // "GEOMETRY"
};

// PeriodicWaveTypeChoices: Permitted values for PeriodicWave type. X3D enumeration naming conventions are capitalized versions of Web Audio API enumerations, also changing hyphens to underscores. This list is bounded, no additional values are allowed.
enum class PeriodicWaveTypeChoices {
    SINE,  // "SINE"
    SQUARE,  // "SQUARE"
    SAWTOOTH,  // "SAWTOOTH"
    TRIANGLE,  // "TRIANGLE"
    CUSTOM,  // "CUSTOM"
};

// PhaseFunctionValues: phaseFunctionValues are supported enumeration values for the phaseFunction field in the ShadedVolumeStyle. This list is unbounded, additional enumeration values are allowed.
enum class PhaseFunctionValues {
    HENYEY_GREENSTEIN,  // "Henyey-Greenstein"
    NONE,  // "NONE"
};

// PickSensorMatchCriterionChoices: pickSensorMatchCriterionChoices are strictly allowed enumeration values for X3DPickSensorNode node matchCriterion field. This list is bounded, no additional values are allowed.
enum class PickSensorMatchCriterionChoices {
    MATCH_ANY,  // "MATCH_ANY"
    MATCH_EVERY,  // "MATCH_EVERY"
    MATCH_ONLY_ONE,  // "MATCH_ONLY_ONE"
};

// PickSensorSortOrderValues: pickSensorSortOrderValues are supported enumeration values for X3DPickSensorNode node sortOrder field. This list is unbounded, additional enumeration values are allowed.
enum class PickSensorSortOrderValues {
    ANY,  // "ANY"
    CLOSEST,  // "CLOSEST"
    ALL,  // "ALL"
    ALL_SORTED,  // "ALL_SORTED"
};

// PickableObjectTypeValues: pickableObjectTypeValues are supported enumeration values for the objectType field in the abstract types X3DPickableObject and X3DPickSensorNode. This list is unbounded, additional enumeration values are allowed.
enum class PickableObjectTypeValues {
    ALL,  // ""ALL""
    NONE,  // ""NONE""
    TERRAIN,  // ""TERRAIN""
};

// ProfileNameChoices: profileNameChoices enumeration constants are used to identify the profile for each scene-graph node, and also utilized by X3D element to identify the profile of a contained Scene. Profiles correspond primarily to subsets of allowed X3D nodes. Some nodes in Interchange profile include fields that are ignored unless Immersive or Full profile is active. This list is bounded, no additional values are allowed.
enum class ProfileNameChoices {
    CORE,  // "Core"
    INTERCHANGE,  // "Interchange"
    CADINTERCHANGE,  // "CADInterchange"
    INTERACTIVE,  // "Interactive"
    IMMERSIVE,  // "Immersive"
    MEDICALINTERCHANGE,  // "MedicalInterchange"
    MPEG4INTERACTIVE,  // "MPEG4Interactive"
    FULL,  // "Full"
};

// ProjectionVolumeStyleTypeChoices: projectionVolumeStyleTypeChoices are strictly allowed enumeration values for ProjectionVolumeStyle field named 'type'. This list is bounded, no additional values are allowed.
enum class ProjectionVolumeStyleTypeChoices {
    MAX,  // "MAX"
    MIN,  // "MIN"
    AVERAGE,  // "AVERAGE"
};

// ShaderLanguageValues: shaderLanguageValues are supported enumeration values for the language field in shader nodes including "Cg" "GLSL" "HLSL", other values are optionally supported by browsers. Used to optionally determine the language type if no MIME-type information is available. This list is unbounded, additional enumeration values are allowed.
enum class ShaderLanguageValues {
    CG,  // "Cg"
    GLSL,  // "GLSL"
    HLSL,  // "HLSL"
};

// ShaderPartTypeValues: shaderPartTypeValues are allowed enumeration values for ShaderPart type field. This list is unbounded, additional enumeration values are allowed.
enum class ShaderPartTypeValues {
    VERTEX,  // "VERTEX"
    FRAGMENT,  // "FRAGMENT"
};

// TextureBoundaryModeChoices: textureBoundaryModeChoices are strictly allowed enumeration values for TextureProperties boundaryMode* fields. This list is bounded, no additional values are allowed.
enum class TextureBoundaryModeChoices {
    CLAMP,  // "CLAMP"
    CLAMP_TO_EDGE,  // "CLAMP_TO_EDGE"
    CLAMP_TO_BOUNDARY,  // "CLAMP_TO_BOUNDARY"
    MIRRORED_REPEAT,  // "MIRRORED_REPEAT"
    REPEAT,  // "REPEAT"
};

// TextureCompressionModeChoices: textureCompressionModeChoices are strictly allowed enumeration values for TextureProperties field textureCompression. This list is bounded, no additional values are allowed.
enum class TextureCompressionModeChoices {
    DEFAULT,  // "DEFAULT"
    FASTEST,  // "FASTEST"
    HIGH,  // "HIGH"
    LOW,  // "LOW"
    MEDIUM,  // "MEDIUM"
    NICEST,  // "NICEST"
};

// TextureCoordinateGeneratorModeChoices: textureCoordinateGeneratorModeChoices are strictly allowed enumeration values for TextureCoordinateGenerator mode field. This list is bounded, no additional values are allowed.
enum class TextureCoordinateGeneratorModeChoices {
    SPHERE,  // "SPHERE"
    CAMERASPACENORMAL,  // "CAMERASPACENORMAL"
    CAMERASPACEPOSITION,  // "CAMERASPACEPOSITION"
    CAMERASPACEREFLECTIONVECTOR,  // "CAMERASPACEREFLECTIONVECTOR"
    SPHERE_LOCAL,  // "SPHERE-LOCAL"
    COORD,  // "COORD"
    COORD_EYE,  // "COORD-EYE"
    NOISE,  // "NOISE"
    NOISE_EYE,  // "NOISE-EYE"
    SPHERE_REFLECT,  // "SPHERE-REFLECT"
    SPHERE_REFLECT_LOCAL,  // "SPHERE-REFLECT-LOCAL"
};

// TextureMagnificationModeChoices: textureMagnificationModeChoices are strictly allowed enumeration values for TextureProperties field magnificationFilter. This list is bounded, no additional values are allowed.
enum class TextureMagnificationModeChoices {
    AVG_PIXEL,  // "AVG_PIXEL"
    DEFAULT,  // "DEFAULT"
    FASTEST,  // "FASTEST"
    NEAREST_PIXEL,  // "NEAREST_PIXEL"
    NICEST,  // "NICEST"
};

// TextureMinificationModeChoices: textureMinificationModeChoices are strictly allowed enumeration values for TextureProperties field minificationFilter. This list is bounded, no additional values are allowed.
enum class TextureMinificationModeChoices {
    AVG_PIXEL,  // "AVG_PIXEL"
    AVG_PIXEL_AVG_MIPMAP,  // "AVG_PIXEL_AVG_MIPMAP"
    AVG_PIXEL_NEAREST_MIPMAP,  // "AVG_PIXEL_NEAREST_MIPMAP"
    DEFAULT,  // "DEFAULT"
    FASTEST,  // "FASTEST"
    NEAREST_PIXEL,  // "NEAREST_PIXEL"
    NEAREST_PIXEL_AVG_MIPMAP,  // "NEAREST_PIXEL_AVG_MIPMAP"
    NEAREST_PIXEL_NEAREST_MIPMAP,  // "NEAREST_PIXEL_NEAREST_MIPMAP"
    NICEST,  // "NICEST"
};

// UnitCategoryChoices: unitCategoryChoices are strictly allowed enumeration values for standard units in the UNIT statement. This list is bounded, no additional values are allowed.
enum class UnitCategoryChoices {
    ANGLE,  // "angle"
    FORCE,  // "force"
    LENGTH,  // "length"
    MASS,  // "mass"
};

// WaveShaperOversampleChoices: Permitted values for WaveShaper oversample field. This list is bounded, no additional values are allowed.
enum class WaveShaperOversampleChoices {
    NONE,  // "NONE"
    _2X,  // "2X"
    _4X,  // "4X"
};

// X3dVersionChoices: x3dVersionChoices enumeration string constants are used to identify the allowed versions for an X3D scene graph. This list is bounded, no additional values are allowed.
enum class X3dVersionChoices {
    _3_0,  // "3.0"
    _3_1,  // "3.1"
    _3_2,  // "3.2"
    _3_3,  // "3.3"
    _4_0,  // "4.0"
};

// X3D-token <-> enum-value conversions for the bounded enums above.
inline std::string to_string(AccessTypeChoices value) {
    switch (value) {
    case AccessTypeChoices::INITIALIZEONLY: return "initializeOnly";
    case AccessTypeChoices::INPUTONLY: return "inputOnly";
    case AccessTypeChoices::OUTPUTONLY: return "outputOnly";
    case AccessTypeChoices::INPUTOUTPUT: return "inputOutput";
    }
    return "";
}

inline bool from_string(const std::string& token, AccessTypeChoices& out) {
    if (token == "initializeOnly") { out = AccessTypeChoices::INITIALIZEONLY; return true; }
    if (token == "inputOnly") { out = AccessTypeChoices::INPUTONLY; return true; }
    if (token == "outputOnly") { out = AccessTypeChoices::OUTPUTONLY; return true; }
    if (token == "inputOutput") { out = AccessTypeChoices::INPUTOUTPUT; return true; }
    return false;
}

inline std::string to_string(AlphaModeChoices value) {
    switch (value) {
    case AlphaModeChoices::AUTO: return "AUTO";
    case AlphaModeChoices::OPAQUE: return "OPAQUE";
    case AlphaModeChoices::MASK: return "MASK";
    case AlphaModeChoices::BLEND: return "BLEND";
    }
    return "";
}

inline bool from_string(const std::string& token, AlphaModeChoices& out) {
    if (token == "AUTO") { out = AlphaModeChoices::AUTO; return true; }
    if (token == "OPAQUE") { out = AlphaModeChoices::OPAQUE; return true; }
    if (token == "MASK") { out = AlphaModeChoices::MASK; return true; }
    if (token == "BLEND") { out = AlphaModeChoices::BLEND; return true; }
    return false;
}

inline std::string to_string(AppliedParametersChoices value) {
    switch (value) {
    case AppliedParametersChoices::BOUNCE: return "BOUNCE";
    case AppliedParametersChoices::USER_FRICTION: return "USER_FRICTION";
    case AppliedParametersChoices::FRICTION_COEFFICIENT_2: return "FRICTION_COEFFICIENT-2";
    case AppliedParametersChoices::ERROR_REDUCTION: return "ERROR_REDUCTION";
    case AppliedParametersChoices::CONSTANT_FORCE: return "CONSTANT_FORCE";
    case AppliedParametersChoices::SPEED_1: return "SPEED-1";
    case AppliedParametersChoices::SPEED_2: return "SPEED-2";
    case AppliedParametersChoices::SLIP_1: return "SLIP-1";
    case AppliedParametersChoices::SLIP_2: return "SLIP-2";
    }
    return "";
}

inline bool from_string(const std::string& token, AppliedParametersChoices& out) {
    if (token == "BOUNCE") { out = AppliedParametersChoices::BOUNCE; return true; }
    if (token == "USER_FRICTION") { out = AppliedParametersChoices::USER_FRICTION; return true; }
    if (token == "FRICTION_COEFFICIENT-2") { out = AppliedParametersChoices::FRICTION_COEFFICIENT_2; return true; }
    if (token == "ERROR_REDUCTION") { out = AppliedParametersChoices::ERROR_REDUCTION; return true; }
    if (token == "CONSTANT_FORCE") { out = AppliedParametersChoices::CONSTANT_FORCE; return true; }
    if (token == "SPEED-1") { out = AppliedParametersChoices::SPEED_1; return true; }
    if (token == "SPEED-2") { out = AppliedParametersChoices::SPEED_2; return true; }
    if (token == "SLIP-1") { out = AppliedParametersChoices::SLIP_1; return true; }
    if (token == "SLIP-2") { out = AppliedParametersChoices::SLIP_2; return true; }
    return false;
}

inline std::string to_string(BiquadTypeFilterChoices value) {
    switch (value) {
    case BiquadTypeFilterChoices::LOWPASS: return "LOWPASS";
    case BiquadTypeFilterChoices::HIGHPASS: return "HIGHPASS";
    case BiquadTypeFilterChoices::BANDPASS: return "BANDPASS";
    case BiquadTypeFilterChoices::LOWSHELF: return "LOWSHELF";
    case BiquadTypeFilterChoices::HIGHSHELF: return "HIGHSHELF";
    case BiquadTypeFilterChoices::PEAKING: return "PEAKING";
    case BiquadTypeFilterChoices::NOTCH: return "NOTCH";
    case BiquadTypeFilterChoices::ALLPASS: return "ALLPASS";
    }
    return "";
}

inline bool from_string(const std::string& token, BiquadTypeFilterChoices& out) {
    if (token == "LOWPASS") { out = BiquadTypeFilterChoices::LOWPASS; return true; }
    if (token == "HIGHPASS") { out = BiquadTypeFilterChoices::HIGHPASS; return true; }
    if (token == "BANDPASS") { out = BiquadTypeFilterChoices::BANDPASS; return true; }
    if (token == "LOWSHELF") { out = BiquadTypeFilterChoices::LOWSHELF; return true; }
    if (token == "HIGHSHELF") { out = BiquadTypeFilterChoices::HIGHSHELF; return true; }
    if (token == "PEAKING") { out = BiquadTypeFilterChoices::PEAKING; return true; }
    if (token == "NOTCH") { out = BiquadTypeFilterChoices::NOTCH; return true; }
    if (token == "ALLPASS") { out = BiquadTypeFilterChoices::ALLPASS; return true; }
    return false;
}

inline std::string to_string(ChannelCountModeChoices value) {
    switch (value) {
    case ChannelCountModeChoices::MAX: return "MAX";
    case ChannelCountModeChoices::CLAMPED_MAX: return "CLAMPED_MAX";
    case ChannelCountModeChoices::EXPLICIT: return "EXPLICIT";
    }
    return "";
}

inline bool from_string(const std::string& token, ChannelCountModeChoices& out) {
    if (token == "MAX") { out = ChannelCountModeChoices::MAX; return true; }
    if (token == "CLAMPED_MAX") { out = ChannelCountModeChoices::CLAMPED_MAX; return true; }
    if (token == "EXPLICIT") { out = ChannelCountModeChoices::EXPLICIT; return true; }
    return false;
}

inline std::string to_string(ChannelInterpretationChoices value) {
    switch (value) {
    case ChannelInterpretationChoices::SPEAKERS: return "SPEAKERS";
    case ChannelInterpretationChoices::DISCRETE: return "DISCRETE";
    }
    return "";
}

inline bool from_string(const std::string& token, ChannelInterpretationChoices& out) {
    if (token == "SPEAKERS") { out = ChannelInterpretationChoices::SPEAKERS; return true; }
    if (token == "DISCRETE") { out = ChannelInterpretationChoices::DISCRETE; return true; }
    return false;
}

inline std::string to_string(ClosureTypeChoices value) {
    switch (value) {
    case ClosureTypeChoices::PIE: return "PIE";
    case ClosureTypeChoices::CHORD: return "CHORD";
    }
    return "";
}

inline bool from_string(const std::string& token, ClosureTypeChoices& out) {
    if (token == "PIE") { out = ClosureTypeChoices::PIE; return true; }
    if (token == "CHORD") { out = ClosureTypeChoices::CHORD; return true; }
    return false;
}

inline std::string to_string(ComponentNameChoices value) {
    switch (value) {
    case ComponentNameChoices::CORE: return "Core";
    case ComponentNameChoices::CADGEOMETRY: return "CADGeometry";
    case ComponentNameChoices::CUBEMAPTEXTURING: return "CubeMapTexturing";
    case ComponentNameChoices::DIS: return "DIS";
    case ComponentNameChoices::ENVIRONMENTALEFFECTS: return "EnvironmentalEffects";
    case ComponentNameChoices::ENVIRONMENTALSENSOR: return "EnvironmentalSensor";
    case ComponentNameChoices::EVENTUTILITIES: return "EventUtilities";
    case ComponentNameChoices::FOLLOWERS: return "Followers";
    case ComponentNameChoices::GEOMETRY2D: return "Geometry2D";
    case ComponentNameChoices::GEOMETRY3D: return "Geometry3D";
    case ComponentNameChoices::GEOSPATIAL: return "Geospatial";
    case ComponentNameChoices::GROUPING: return "Grouping";
    case ComponentNameChoices::HANIM: return "HAnim";
    case ComponentNameChoices::H_ANIM: return "H-Anim";
    case ComponentNameChoices::INTERPOLATION: return "Interpolation";
    case ComponentNameChoices::KEYDEVICESENSOR: return "KeyDeviceSensor";
    case ComponentNameChoices::LAYERING: return "Layering";
    case ComponentNameChoices::LAYOUT: return "Layout";
    case ComponentNameChoices::LIGHTING: return "Lighting";
    case ComponentNameChoices::NAVIGATION: return "Navigation";
    case ComponentNameChoices::NETWORKING: return "Networking";
    case ComponentNameChoices::NURBS: return "NURBS";
    case ComponentNameChoices::PARTICLESYSTEMS: return "ParticleSystems";
    case ComponentNameChoices::PICKING: return "Picking";
    case ComponentNameChoices::POINTINGDEVICESENSOR: return "PointingDeviceSensor";
    case ComponentNameChoices::TEXTUREPROJECTION: return "TextureProjection";
    case ComponentNameChoices::RENDERING: return "Rendering";
    case ComponentNameChoices::RIGIDBODYPHYSICS: return "RigidBodyPhysics";
    case ComponentNameChoices::SCRIPTING: return "Scripting";
    case ComponentNameChoices::SHADERS: return "Shaders";
    case ComponentNameChoices::SHAPE: return "Shape";
    case ComponentNameChoices::SOUND: return "Sound";
    case ComponentNameChoices::TEXT: return "Text";
    case ComponentNameChoices::TEXTURING: return "Texturing";
    case ComponentNameChoices::TEXTURING3D: return "Texturing3D";
    case ComponentNameChoices::TIME: return "Time";
    case ComponentNameChoices::VOLUMERENDERING: return "VolumeRendering";
    }
    return "";
}

inline bool from_string(const std::string& token, ComponentNameChoices& out) {
    if (token == "Core") { out = ComponentNameChoices::CORE; return true; }
    if (token == "CADGeometry") { out = ComponentNameChoices::CADGEOMETRY; return true; }
    if (token == "CubeMapTexturing") { out = ComponentNameChoices::CUBEMAPTEXTURING; return true; }
    if (token == "DIS") { out = ComponentNameChoices::DIS; return true; }
    if (token == "EnvironmentalEffects") { out = ComponentNameChoices::ENVIRONMENTALEFFECTS; return true; }
    if (token == "EnvironmentalSensor") { out = ComponentNameChoices::ENVIRONMENTALSENSOR; return true; }
    if (token == "EventUtilities") { out = ComponentNameChoices::EVENTUTILITIES; return true; }
    if (token == "Followers") { out = ComponentNameChoices::FOLLOWERS; return true; }
    if (token == "Geometry2D") { out = ComponentNameChoices::GEOMETRY2D; return true; }
    if (token == "Geometry3D") { out = ComponentNameChoices::GEOMETRY3D; return true; }
    if (token == "Geospatial") { out = ComponentNameChoices::GEOSPATIAL; return true; }
    if (token == "Grouping") { out = ComponentNameChoices::GROUPING; return true; }
    if (token == "HAnim") { out = ComponentNameChoices::HANIM; return true; }
    if (token == "H-Anim") { out = ComponentNameChoices::H_ANIM; return true; }
    if (token == "Interpolation") { out = ComponentNameChoices::INTERPOLATION; return true; }
    if (token == "KeyDeviceSensor") { out = ComponentNameChoices::KEYDEVICESENSOR; return true; }
    if (token == "Layering") { out = ComponentNameChoices::LAYERING; return true; }
    if (token == "Layout") { out = ComponentNameChoices::LAYOUT; return true; }
    if (token == "Lighting") { out = ComponentNameChoices::LIGHTING; return true; }
    if (token == "Navigation") { out = ComponentNameChoices::NAVIGATION; return true; }
    if (token == "Networking") { out = ComponentNameChoices::NETWORKING; return true; }
    if (token == "NURBS") { out = ComponentNameChoices::NURBS; return true; }
    if (token == "ParticleSystems") { out = ComponentNameChoices::PARTICLESYSTEMS; return true; }
    if (token == "Picking") { out = ComponentNameChoices::PICKING; return true; }
    if (token == "PointingDeviceSensor") { out = ComponentNameChoices::POINTINGDEVICESENSOR; return true; }
    if (token == "TextureProjection") { out = ComponentNameChoices::TEXTUREPROJECTION; return true; }
    if (token == "Rendering") { out = ComponentNameChoices::RENDERING; return true; }
    if (token == "RigidBodyPhysics") { out = ComponentNameChoices::RIGIDBODYPHYSICS; return true; }
    if (token == "Scripting") { out = ComponentNameChoices::SCRIPTING; return true; }
    if (token == "Shaders") { out = ComponentNameChoices::SHADERS; return true; }
    if (token == "Shape") { out = ComponentNameChoices::SHAPE; return true; }
    if (token == "Sound") { out = ComponentNameChoices::SOUND; return true; }
    if (token == "Text") { out = ComponentNameChoices::TEXT; return true; }
    if (token == "Texturing") { out = ComponentNameChoices::TEXTURING; return true; }
    if (token == "Texturing3D") { out = ComponentNameChoices::TEXTURING3D; return true; }
    if (token == "Time") { out = ComponentNameChoices::TIME; return true; }
    if (token == "VolumeRendering") { out = ComponentNameChoices::VOLUMERENDERING; return true; }
    return false;
}

inline std::string to_string(DistanceModelChoices value) {
    switch (value) {
    case DistanceModelChoices::LINEAR: return "LINEAR";
    case DistanceModelChoices::INVERSE: return "INVERSE";
    case DistanceModelChoices::EXPONENTIAL: return "EXPONENTIAL";
    }
    return "";
}

inline bool from_string(const std::string& token, DistanceModelChoices& out) {
    if (token == "LINEAR") { out = DistanceModelChoices::LINEAR; return true; }
    if (token == "INVERSE") { out = DistanceModelChoices::INVERSE; return true; }
    if (token == "EXPONENTIAL") { out = DistanceModelChoices::EXPONENTIAL; return true; }
    return false;
}

inline std::string to_string(FieldTypeChoices value) {
    switch (value) {
    case FieldTypeChoices::SFBOOL: return "SFBool";
    case FieldTypeChoices::MFBOOL: return "MFBool";
    case FieldTypeChoices::SFCOLOR: return "SFColor";
    case FieldTypeChoices::MFCOLOR: return "MFColor";
    case FieldTypeChoices::SFCOLORRGBA: return "SFColorRGBA";
    case FieldTypeChoices::MFCOLORRGBA: return "MFColorRGBA";
    case FieldTypeChoices::SFDOUBLE: return "SFDouble";
    case FieldTypeChoices::MFDOUBLE: return "MFDouble";
    case FieldTypeChoices::SFFLOAT: return "SFFloat";
    case FieldTypeChoices::MFFLOAT: return "MFFloat";
    case FieldTypeChoices::SFIMAGE: return "SFImage";
    case FieldTypeChoices::MFIMAGE: return "MFImage";
    case FieldTypeChoices::SFINT32: return "SFInt32";
    case FieldTypeChoices::MFINT32: return "MFInt32";
    case FieldTypeChoices::SFNODE: return "SFNode";
    case FieldTypeChoices::MFNODE: return "MFNode";
    case FieldTypeChoices::SFROTATION: return "SFRotation";
    case FieldTypeChoices::MFROTATION: return "MFRotation";
    case FieldTypeChoices::SFSTRING: return "SFString";
    case FieldTypeChoices::MFSTRING: return "MFString";
    case FieldTypeChoices::SFTIME: return "SFTime";
    case FieldTypeChoices::MFTIME: return "MFTime";
    case FieldTypeChoices::SFVEC2D: return "SFVec2d";
    case FieldTypeChoices::MFVEC2D: return "MFVec2d";
    case FieldTypeChoices::SFVEC2F: return "SFVec2f";
    case FieldTypeChoices::MFVEC2F: return "MFVec2f";
    case FieldTypeChoices::SFVEC3D: return "SFVec3d";
    case FieldTypeChoices::MFVEC3D: return "MFVec3d";
    case FieldTypeChoices::SFVEC3F: return "SFVec3f";
    case FieldTypeChoices::MFVEC3F: return "MFVec3f";
    case FieldTypeChoices::SFVEC4D: return "SFVec4d";
    case FieldTypeChoices::MFVEC4D: return "MFVec4d";
    case FieldTypeChoices::SFVEC4F: return "SFVec4f";
    case FieldTypeChoices::MFVEC4F: return "MFVec4f";
    case FieldTypeChoices::SFMATRIX3D: return "SFMatrix3d";
    case FieldTypeChoices::MFMATRIX3D: return "MFMatrix3d";
    case FieldTypeChoices::SFMATRIX3F: return "SFMatrix3f";
    case FieldTypeChoices::MFMATRIX3F: return "MFMatrix3f";
    case FieldTypeChoices::SFMATRIX4D: return "SFMatrix4d";
    case FieldTypeChoices::MFMATRIX4D: return "MFMatrix4d";
    case FieldTypeChoices::SFMATRIX4F: return "SFMatrix4f";
    case FieldTypeChoices::MFMATRIX4F: return "MFMatrix4f";
    }
    return "";
}

inline bool from_string(const std::string& token, FieldTypeChoices& out) {
    if (token == "SFBool") { out = FieldTypeChoices::SFBOOL; return true; }
    if (token == "MFBool") { out = FieldTypeChoices::MFBOOL; return true; }
    if (token == "SFColor") { out = FieldTypeChoices::SFCOLOR; return true; }
    if (token == "MFColor") { out = FieldTypeChoices::MFCOLOR; return true; }
    if (token == "SFColorRGBA") { out = FieldTypeChoices::SFCOLORRGBA; return true; }
    if (token == "MFColorRGBA") { out = FieldTypeChoices::MFCOLORRGBA; return true; }
    if (token == "SFDouble") { out = FieldTypeChoices::SFDOUBLE; return true; }
    if (token == "MFDouble") { out = FieldTypeChoices::MFDOUBLE; return true; }
    if (token == "SFFloat") { out = FieldTypeChoices::SFFLOAT; return true; }
    if (token == "MFFloat") { out = FieldTypeChoices::MFFLOAT; return true; }
    if (token == "SFImage") { out = FieldTypeChoices::SFIMAGE; return true; }
    if (token == "MFImage") { out = FieldTypeChoices::MFIMAGE; return true; }
    if (token == "SFInt32") { out = FieldTypeChoices::SFINT32; return true; }
    if (token == "MFInt32") { out = FieldTypeChoices::MFINT32; return true; }
    if (token == "SFNode") { out = FieldTypeChoices::SFNODE; return true; }
    if (token == "MFNode") { out = FieldTypeChoices::MFNODE; return true; }
    if (token == "SFRotation") { out = FieldTypeChoices::SFROTATION; return true; }
    if (token == "MFRotation") { out = FieldTypeChoices::MFROTATION; return true; }
    if (token == "SFString") { out = FieldTypeChoices::SFSTRING; return true; }
    if (token == "MFString") { out = FieldTypeChoices::MFSTRING; return true; }
    if (token == "SFTime") { out = FieldTypeChoices::SFTIME; return true; }
    if (token == "MFTime") { out = FieldTypeChoices::MFTIME; return true; }
    if (token == "SFVec2d") { out = FieldTypeChoices::SFVEC2D; return true; }
    if (token == "MFVec2d") { out = FieldTypeChoices::MFVEC2D; return true; }
    if (token == "SFVec2f") { out = FieldTypeChoices::SFVEC2F; return true; }
    if (token == "MFVec2f") { out = FieldTypeChoices::MFVEC2F; return true; }
    if (token == "SFVec3d") { out = FieldTypeChoices::SFVEC3D; return true; }
    if (token == "MFVec3d") { out = FieldTypeChoices::MFVEC3D; return true; }
    if (token == "SFVec3f") { out = FieldTypeChoices::SFVEC3F; return true; }
    if (token == "MFVec3f") { out = FieldTypeChoices::MFVEC3F; return true; }
    if (token == "SFVec4d") { out = FieldTypeChoices::SFVEC4D; return true; }
    if (token == "MFVec4d") { out = FieldTypeChoices::MFVEC4D; return true; }
    if (token == "SFVec4f") { out = FieldTypeChoices::SFVEC4F; return true; }
    if (token == "MFVec4f") { out = FieldTypeChoices::MFVEC4F; return true; }
    if (token == "SFMatrix3d") { out = FieldTypeChoices::SFMATRIX3D; return true; }
    if (token == "MFMatrix3d") { out = FieldTypeChoices::MFMATRIX3D; return true; }
    if (token == "SFMatrix3f") { out = FieldTypeChoices::SFMATRIX3F; return true; }
    if (token == "MFMatrix3f") { out = FieldTypeChoices::MFMATRIX3F; return true; }
    if (token == "SFMatrix4d") { out = FieldTypeChoices::SFMATRIX4D; return true; }
    if (token == "MFMatrix4d") { out = FieldTypeChoices::MFMATRIX4D; return true; }
    if (token == "SFMatrix4f") { out = FieldTypeChoices::SFMATRIX4F; return true; }
    if (token == "MFMatrix4f") { out = FieldTypeChoices::MFMATRIX4F; return true; }
    return false;
}

inline std::string to_string(FogTypeChoices value) {
    switch (value) {
    case FogTypeChoices::LINEAR: return "LINEAR";
    case FogTypeChoices::EXPONENTIAL: return "EXPONENTIAL";
    }
    return "";
}

inline bool from_string(const std::string& token, FogTypeChoices& out) {
    if (token == "LINEAR") { out = FogTypeChoices::LINEAR; return true; }
    if (token == "EXPONENTIAL") { out = FogTypeChoices::EXPONENTIAL; return true; }
    return false;
}

inline std::string to_string(FontFamilyValues value) {
    switch (value) {
    case FontFamilyValues::SANS: return "SANS";
    case FontFamilyValues::SERIF: return "SERIF";
    case FontFamilyValues::TYPEWRITER: return "TYPEWRITER";
    }
    return "";
}

inline bool from_string(const std::string& token, FontFamilyValues& out) {
    if (token == "SANS") { out = FontFamilyValues::SANS; return true; }
    if (token == "SERIF") { out = FontFamilyValues::SERIF; return true; }
    if (token == "TYPEWRITER") { out = FontFamilyValues::TYPEWRITER; return true; }
    return false;
}

inline std::string to_string(FontStyleChoices value) {
    switch (value) {
    case FontStyleChoices::PLAIN: return "PLAIN";
    case FontStyleChoices::BOLD: return "BOLD";
    case FontStyleChoices::ITALIC: return "ITALIC";
    case FontStyleChoices::BOLDITALIC: return "BOLDITALIC";
    }
    return "";
}

inline bool from_string(const std::string& token, FontStyleChoices& out) {
    if (token == "PLAIN") { out = FontStyleChoices::PLAIN; return true; }
    if (token == "BOLD") { out = FontStyleChoices::BOLD; return true; }
    if (token == "ITALIC") { out = FontStyleChoices::ITALIC; return true; }
    if (token == "BOLDITALIC") { out = FontStyleChoices::BOLDITALIC; return true; }
    return false;
}

inline std::string to_string(ForceOutputValues value) {
    switch (value) {
    case ForceOutputValues::ALL: return "ALL";
    case ForceOutputValues::NONE: return "NONE";
    }
    return "";
}

inline bool from_string(const std::string& token, ForceOutputValues& out) {
    if (token == "ALL") { out = ForceOutputValues::ALL; return true; }
    if (token == "NONE") { out = ForceOutputValues::NONE; return true; }
    return false;
}

inline std::string to_string(GeneratedCubeMapTextureUpdateChoices value) {
    switch (value) {
    case GeneratedCubeMapTextureUpdateChoices::NONE: return "NONE";
    case GeneratedCubeMapTextureUpdateChoices::NEXT_FRAME_ONLY: return "NEXT_FRAME_ONLY";
    case GeneratedCubeMapTextureUpdateChoices::ALWAYS: return "ALWAYS";
    }
    return "";
}

inline bool from_string(const std::string& token, GeneratedCubeMapTextureUpdateChoices& out) {
    if (token == "NONE") { out = GeneratedCubeMapTextureUpdateChoices::NONE; return true; }
    if (token == "NEXT_FRAME_ONLY") { out = GeneratedCubeMapTextureUpdateChoices::NEXT_FRAME_ONLY; return true; }
    if (token == "ALWAYS") { out = GeneratedCubeMapTextureUpdateChoices::ALWAYS; return true; }
    return false;
}

inline std::string to_string(GeoMetadataSummaryKeyValues value) {
    switch (value) {
    case GeoMetadataSummaryKeyValues::TITLE: return "title";
    case GeoMetadataSummaryKeyValues::DESCRIPTION: return "description";
    case GeoMetadataSummaryKeyValues::COORDINATESYSTEM: return "coordinateSystem";
    case GeoMetadataSummaryKeyValues::HORIZONTALDATUM: return "horizontalDatum";
    case GeoMetadataSummaryKeyValues::VERTICALDATUM: return "verticalDatum";
    case GeoMetadataSummaryKeyValues::ELLIPSOID: return "ellipsoid";
    case GeoMetadataSummaryKeyValues::EXTENT: return "extent";
    case GeoMetadataSummaryKeyValues::RESOLUTION: return "resolution";
    case GeoMetadataSummaryKeyValues::ORIGINATOR: return "originator";
    case GeoMetadataSummaryKeyValues::COPYRIGHT: return "copyright";
    case GeoMetadataSummaryKeyValues::DATE: return "date";
    case GeoMetadataSummaryKeyValues::METADATAFORMAT: return "metadataFormat";
    case GeoMetadataSummaryKeyValues::DATAURL: return "dataUrl";
    case GeoMetadataSummaryKeyValues::DATAFORMAT: return "dataFormat";
    }
    return "";
}

inline bool from_string(const std::string& token, GeoMetadataSummaryKeyValues& out) {
    if (token == "title") { out = GeoMetadataSummaryKeyValues::TITLE; return true; }
    if (token == "description") { out = GeoMetadataSummaryKeyValues::DESCRIPTION; return true; }
    if (token == "coordinateSystem") { out = GeoMetadataSummaryKeyValues::COORDINATESYSTEM; return true; }
    if (token == "horizontalDatum") { out = GeoMetadataSummaryKeyValues::HORIZONTALDATUM; return true; }
    if (token == "verticalDatum") { out = GeoMetadataSummaryKeyValues::VERTICALDATUM; return true; }
    if (token == "ellipsoid") { out = GeoMetadataSummaryKeyValues::ELLIPSOID; return true; }
    if (token == "extent") { out = GeoMetadataSummaryKeyValues::EXTENT; return true; }
    if (token == "resolution") { out = GeoMetadataSummaryKeyValues::RESOLUTION; return true; }
    if (token == "originator") { out = GeoMetadataSummaryKeyValues::ORIGINATOR; return true; }
    if (token == "copyright") { out = GeoMetadataSummaryKeyValues::COPYRIGHT; return true; }
    if (token == "date") { out = GeoMetadataSummaryKeyValues::DATE; return true; }
    if (token == "metadataFormat") { out = GeoMetadataSummaryKeyValues::METADATAFORMAT; return true; }
    if (token == "dataUrl") { out = GeoMetadataSummaryKeyValues::DATAURL; return true; }
    if (token == "dataFormat") { out = GeoMetadataSummaryKeyValues::DATAFORMAT; return true; }
    return false;
}

inline std::string to_string(GeoSystemEarthEllipsoidValues value) {
    switch (value) {
    case GeoSystemEarthEllipsoidValues::AM: return "AM";
    case GeoSystemEarthEllipsoidValues::AN: return "AN";
    case GeoSystemEarthEllipsoidValues::BN: return "BN";
    case GeoSystemEarthEllipsoidValues::BR: return "BR";
    case GeoSystemEarthEllipsoidValues::CC: return "CC";
    case GeoSystemEarthEllipsoidValues::CD: return "CD";
    case GeoSystemEarthEllipsoidValues::EA: return "EA";
    case GeoSystemEarthEllipsoidValues::EB: return "EB";
    case GeoSystemEarthEllipsoidValues::EC: return "EC";
    case GeoSystemEarthEllipsoidValues::ED: return "ED";
    case GeoSystemEarthEllipsoidValues::EE: return "EE";
    case GeoSystemEarthEllipsoidValues::EF: return "EF";
    case GeoSystemEarthEllipsoidValues::FA: return "FA";
    case GeoSystemEarthEllipsoidValues::HE: return "HE";
    case GeoSystemEarthEllipsoidValues::HO: return "HO";
    case GeoSystemEarthEllipsoidValues::ID: return "ID";
    case GeoSystemEarthEllipsoidValues::IN: return "IN";
    case GeoSystemEarthEllipsoidValues::KA: return "KA";
    case GeoSystemEarthEllipsoidValues::RF: return "RF";
    case GeoSystemEarthEllipsoidValues::SA: return "SA";
    case GeoSystemEarthEllipsoidValues::WD: return "WD";
    case GeoSystemEarthEllipsoidValues::WE: return "WE";
    case GeoSystemEarthEllipsoidValues::WGS84: return "WGS84";
    case GeoSystemEarthEllipsoidValues::ZN: return "Zn";
    case GeoSystemEarthEllipsoidValues::S: return "S";
    }
    return "";
}

inline bool from_string(const std::string& token, GeoSystemEarthEllipsoidValues& out) {
    if (token == "AM") { out = GeoSystemEarthEllipsoidValues::AM; return true; }
    if (token == "AN") { out = GeoSystemEarthEllipsoidValues::AN; return true; }
    if (token == "BN") { out = GeoSystemEarthEllipsoidValues::BN; return true; }
    if (token == "BR") { out = GeoSystemEarthEllipsoidValues::BR; return true; }
    if (token == "CC") { out = GeoSystemEarthEllipsoidValues::CC; return true; }
    if (token == "CD") { out = GeoSystemEarthEllipsoidValues::CD; return true; }
    if (token == "EA") { out = GeoSystemEarthEllipsoidValues::EA; return true; }
    if (token == "EB") { out = GeoSystemEarthEllipsoidValues::EB; return true; }
    if (token == "EC") { out = GeoSystemEarthEllipsoidValues::EC; return true; }
    if (token == "ED") { out = GeoSystemEarthEllipsoidValues::ED; return true; }
    if (token == "EE") { out = GeoSystemEarthEllipsoidValues::EE; return true; }
    if (token == "EF") { out = GeoSystemEarthEllipsoidValues::EF; return true; }
    if (token == "FA") { out = GeoSystemEarthEllipsoidValues::FA; return true; }
    if (token == "HE") { out = GeoSystemEarthEllipsoidValues::HE; return true; }
    if (token == "HO") { out = GeoSystemEarthEllipsoidValues::HO; return true; }
    if (token == "ID") { out = GeoSystemEarthEllipsoidValues::ID; return true; }
    if (token == "IN") { out = GeoSystemEarthEllipsoidValues::IN; return true; }
    if (token == "KA") { out = GeoSystemEarthEllipsoidValues::KA; return true; }
    if (token == "RF") { out = GeoSystemEarthEllipsoidValues::RF; return true; }
    if (token == "SA") { out = GeoSystemEarthEllipsoidValues::SA; return true; }
    if (token == "WD") { out = GeoSystemEarthEllipsoidValues::WD; return true; }
    if (token == "WE") { out = GeoSystemEarthEllipsoidValues::WE; return true; }
    if (token == "WGS84") { out = GeoSystemEarthEllipsoidValues::WGS84; return true; }
    if (token == "Zn") { out = GeoSystemEarthEllipsoidValues::ZN; return true; }
    if (token == "S") { out = GeoSystemEarthEllipsoidValues::S; return true; }
    return false;
}

inline std::string to_string(GeoSystemSpatialReferenceFrameValues value) {
    switch (value) {
    case GeoSystemSpatialReferenceFrameValues::GD: return "GD";
    case GeoSystemSpatialReferenceFrameValues::UTM: return "UTM";
    case GeoSystemSpatialReferenceFrameValues::GC: return "GC";
    case GeoSystemSpatialReferenceFrameValues::GDC: return "GDC";
    case GeoSystemSpatialReferenceFrameValues::GCC: return "GCC";
    }
    return "";
}

inline bool from_string(const std::string& token, GeoSystemSpatialReferenceFrameValues& out) {
    if (token == "GD") { out = GeoSystemSpatialReferenceFrameValues::GD; return true; }
    if (token == "UTM") { out = GeoSystemSpatialReferenceFrameValues::UTM; return true; }
    if (token == "GC") { out = GeoSystemSpatialReferenceFrameValues::GC; return true; }
    if (token == "GDC") { out = GeoSystemSpatialReferenceFrameValues::GDC; return true; }
    if (token == "GCC") { out = GeoSystemSpatialReferenceFrameValues::GCC; return true; }
    return false;
}

inline std::string to_string(HanimHumanoidInfoKeyValues value) {
    switch (value) {
    case HanimHumanoidInfoKeyValues::AUTHORNAME: return "authorName";
    case HanimHumanoidInfoKeyValues::AUTHOREMAIL: return "authorEmail";
    case HanimHumanoidInfoKeyValues::COPYRIGHT: return "copyright";
    case HanimHumanoidInfoKeyValues::CREATIONDATE: return "creationDate";
    case HanimHumanoidInfoKeyValues::USAGERESTRICTIONS: return "usageRestrictions";
    case HanimHumanoidInfoKeyValues::HUMANOIDVERSION: return "humanoidVersion";
    case HanimHumanoidInfoKeyValues::AGE: return "age";
    case HanimHumanoidInfoKeyValues::GENDER: return "gender";
    case HanimHumanoidInfoKeyValues::HEIGHT: return "height";
    case HanimHumanoidInfoKeyValues::WEIGHT: return "weight";
    }
    return "";
}

inline bool from_string(const std::string& token, HanimHumanoidInfoKeyValues& out) {
    if (token == "authorName") { out = HanimHumanoidInfoKeyValues::AUTHORNAME; return true; }
    if (token == "authorEmail") { out = HanimHumanoidInfoKeyValues::AUTHOREMAIL; return true; }
    if (token == "copyright") { out = HanimHumanoidInfoKeyValues::COPYRIGHT; return true; }
    if (token == "creationDate") { out = HanimHumanoidInfoKeyValues::CREATIONDATE; return true; }
    if (token == "usageRestrictions") { out = HanimHumanoidInfoKeyValues::USAGERESTRICTIONS; return true; }
    if (token == "humanoidVersion") { out = HanimHumanoidInfoKeyValues::HUMANOIDVERSION; return true; }
    if (token == "age") { out = HanimHumanoidInfoKeyValues::AGE; return true; }
    if (token == "gender") { out = HanimHumanoidInfoKeyValues::GENDER; return true; }
    if (token == "height") { out = HanimHumanoidInfoKeyValues::HEIGHT; return true; }
    if (token == "weight") { out = HanimHumanoidInfoKeyValues::WEIGHT; return true; }
    return false;
}

inline std::string to_string(HanimJointNameValues value) {
    switch (value) {
    case HanimJointNameValues::HUMANOID_ROOT: return "humanoid_root";
    case HanimJointNameValues::SACROILIAC: return "sacroiliac";
    case HanimJointNameValues::L_HIP: return "l_hip";
    case HanimJointNameValues::L_KNEE: return "l_knee";
    case HanimJointNameValues::L_TALOCRURAL: return "l_talocrural";
    case HanimJointNameValues::L_TALOCALCANEONAVICULAR: return "l_talocalcaneonavicular";
    case HanimJointNameValues::L_CUNEONAVICULAR_1: return "l_cuneonavicular_1";
    case HanimJointNameValues::L_TARSOMETATARSAL_1: return "l_tarsometatarsal_1";
    case HanimJointNameValues::L_METATARSOPHALANGEAL_1: return "l_metatarsophalangeal_1";
    case HanimJointNameValues::L_TARSAL_INTERPHALANGEAL_1: return "l_tarsal_interphalangeal_1";
    case HanimJointNameValues::L_CUNEONAVICULAR_2: return "l_cuneonavicular_2";
    case HanimJointNameValues::L_TARSOMETATARSAL_2: return "l_tarsometatarsal_2";
    case HanimJointNameValues::L_METATARSOPHALANGEAL_2: return "l_metatarsophalangeal_2";
    case HanimJointNameValues::L_TARSAL_PROXIMAL_INTERPHALANGEAL_2: return "l_tarsal_proximal_interphalangeal_2";
    case HanimJointNameValues::L_TARSAL_DISTAL_INTERPHALANGEAL_2: return "l_tarsal_distal_interphalangeal_2";
    case HanimJointNameValues::L_CUNEONAVICULAR_3: return "l_cuneonavicular_3";
    case HanimJointNameValues::L_TARSOMETATARSAL_3: return "l_tarsometatarsal_3";
    case HanimJointNameValues::L_METATARSOPHALANGEAL_3: return "l_metatarsophalangeal_3";
    case HanimJointNameValues::L_TARSAL_PROXIMAL_INTERPHALANGEAL_3: return "l_tarsal_proximal_interphalangeal_3";
    case HanimJointNameValues::L_TARSAL_DISTAL_INTERPHALANGEAL_3: return "l_tarsal_distal_interphalangeal_3";
    case HanimJointNameValues::L_CALCANEOCUBOID: return "l_calcaneocuboid";
    case HanimJointNameValues::L_TRANSVERSETARSAL: return "l_transversetarsal";
    case HanimJointNameValues::L_TARSOMETATARSAL_4: return "l_tarsometatarsal_4";
    case HanimJointNameValues::L_METATARSOPHALANGEAL_4: return "l_metatarsophalangeal_4";
    case HanimJointNameValues::L_TARSAL_PROXIMAL_INTERPHALANGEAL_4: return "l_tarsal_proximal_interphalangeal_4";
    case HanimJointNameValues::L_TARSAL_DISTAL_INTERPHALANGEAL_4: return "l_tarsal_distal_interphalangeal_4";
    case HanimJointNameValues::L_TARSOMETATARSAL_5: return "l_tarsometatarsal_5";
    case HanimJointNameValues::L_METATARSOPHALANGEAL_5: return "l_metatarsophalangeal_5";
    case HanimJointNameValues::L_TARSAL_PROXIMAL_INTERPHALANGEAL_5: return "l_tarsal_proximal_interphalangeal_5";
    case HanimJointNameValues::L_TARSAL_DISTAL_INTERPHALANGEAL_5: return "l_tarsal_distal_interphalangeal_5";
    case HanimJointNameValues::R_HIP: return "r_hip";
    case HanimJointNameValues::R_KNEE: return "r_knee";
    case HanimJointNameValues::R_TALOCRURAL: return "r_talocrural";
    case HanimJointNameValues::R_TALOCALCANEONAVICULAR: return "r_talocalcaneonavicular";
    case HanimJointNameValues::R_CUNEONAVICULAR_1: return "r_cuneonavicular_1";
    case HanimJointNameValues::R_TARSOMETATARSAL_1: return "r_tarsometatarsal_1";
    case HanimJointNameValues::R_METATARSOPHALANGEAL_1: return "r_metatarsophalangeal_1";
    case HanimJointNameValues::R_TARSAL_INTERPHALANGEAL_1: return "r_tarsal_interphalangeal_1";
    case HanimJointNameValues::R_CUNEONAVICULAR_2: return "r_cuneonavicular_2";
    case HanimJointNameValues::R_TARSOMETATARSAL_2: return "r_tarsometatarsal_2";
    case HanimJointNameValues::R_METATARSOPHALANGEAL_2: return "r_metatarsophalangeal_2";
    case HanimJointNameValues::R_TARSAL_PROXIMAL_INTERPHALANGEAL_2: return "r_tarsal_proximal_interphalangeal_2";
    case HanimJointNameValues::R_TARSAL_DISTAL_INTERPHALANGEAL_2: return "r_tarsal_distal_interphalangeal_2";
    case HanimJointNameValues::R_CUNEONAVICULAR_3: return "r_cuneonavicular_3";
    case HanimJointNameValues::R_TARSOMETATARSAL_3: return "r_tarsometatarsal_3";
    case HanimJointNameValues::R_METATARSOPHALANGEAL_3: return "r_metatarsophalangeal_3";
    case HanimJointNameValues::R_TARSAL_PROXIMAL_INTERPHALANGEAL_3: return "r_tarsal_proximal_interphalangeal_3";
    case HanimJointNameValues::R_TARSAL_DISTAL_INTERPHALANGEAL_3: return "r_tarsal_distal_interphalangeal_3";
    case HanimJointNameValues::R_CALCANEOCUBOID: return "r_calcaneocuboid";
    case HanimJointNameValues::R_TRANSVERSETARSAL: return "r_transversetarsal";
    case HanimJointNameValues::R_TARSOMETATARSAL_4: return "r_tarsometatarsal_4";
    case HanimJointNameValues::R_METATARSOPHALANGEAL_4: return "r_metatarsophalangeal_4";
    case HanimJointNameValues::R_TARSAL_PROXIMAL_INTERPHALANGEAL_4: return "r_tarsal_proximal_interphalangeal_4";
    case HanimJointNameValues::R_TARSAL_DISTAL_INTERPHALANGEAL_4: return "r_tarsal_distal_interphalangeal_4";
    case HanimJointNameValues::R_TARSOMETATARSAL_5: return "r_tarsometatarsal_5";
    case HanimJointNameValues::R_METATARSOPHALANGEAL_5: return "r_metatarsophalangeal_5";
    case HanimJointNameValues::R_TARSAL_PROXIMAL_INTERPHALANGEAL_5: return "r_tarsal_proximal_interphalangeal_5";
    case HanimJointNameValues::R_TARSAL_DISTAL_INTERPHALANGEAL_5: return "r_tarsal_distal_interphalangeal_5";
    case HanimJointNameValues::VL5: return "vl5";
    case HanimJointNameValues::VL4: return "vl4";
    case HanimJointNameValues::VL3: return "vl3";
    case HanimJointNameValues::VL2: return "vl2";
    case HanimJointNameValues::VL1: return "vl1";
    case HanimJointNameValues::VT12: return "vt12";
    case HanimJointNameValues::VT11: return "vt11";
    case HanimJointNameValues::VT10: return "vt10";
    case HanimJointNameValues::VT9: return "vt9";
    case HanimJointNameValues::VT8: return "vt8";
    case HanimJointNameValues::VT7: return "vt7";
    case HanimJointNameValues::VT6: return "vt6";
    case HanimJointNameValues::VT5: return "vt5";
    case HanimJointNameValues::VT4: return "vt4";
    case HanimJointNameValues::VT3: return "vt3";
    case HanimJointNameValues::VT2: return "vt2";
    case HanimJointNameValues::VT1: return "vt1";
    case HanimJointNameValues::VC7: return "vc7";
    case HanimJointNameValues::VC6: return "vc6";
    case HanimJointNameValues::VC5: return "vc5";
    case HanimJointNameValues::VC4: return "vc4";
    case HanimJointNameValues::VC3: return "vc3";
    case HanimJointNameValues::VC2: return "vc2";
    case HanimJointNameValues::VC1: return "vc1";
    case HanimJointNameValues::SKULLBASE: return "skullbase";
    case HanimJointNameValues::L_EYELID_JOINT: return "l_eyelid_joint";
    case HanimJointNameValues::R_EYELID_JOINT: return "r_eyelid_joint";
    case HanimJointNameValues::L_EYEBALL_JOINT: return "l_eyeball_joint";
    case HanimJointNameValues::R_EYEBALL_JOINT: return "r_eyeball_joint";
    case HanimJointNameValues::L_EYEBROW_JOINT: return "l_eyebrow_joint";
    case HanimJointNameValues::R_EYEBROW_JOINT: return "r_eyebrow_joint";
    case HanimJointNameValues::TEMPOROMANDIBULAR: return "temporomandibular";
    case HanimJointNameValues::L_STERNOCLAVICULAR: return "l_sternoclavicular";
    case HanimJointNameValues::L_ACROMIOCLAVICULAR: return "l_acromioclavicular";
    case HanimJointNameValues::L_SHOULDER: return "l_shoulder";
    case HanimJointNameValues::L_ELBOW: return "l_elbow";
    case HanimJointNameValues::L_RADIOCARPAL: return "l_radiocarpal";
    case HanimJointNameValues::L_MIDCARPAL_1: return "l_midcarpal_1";
    case HanimJointNameValues::L_CARPOMETACARPAL_1: return "l_carpometacarpal_1";
    case HanimJointNameValues::L_METACARPOPHALANGEAL_1: return "l_metacarpophalangeal_1";
    case HanimJointNameValues::L_CARPAL_INTERPHALANGEAL_1: return "l_carpal_interphalangeal_1";
    case HanimJointNameValues::L_MIDCARPAL_2: return "l_midcarpal_2";
    case HanimJointNameValues::L_CARPOMETACARPAL_2: return "l_carpometacarpal_2";
    case HanimJointNameValues::L_METACARPOPHALANGEAL_2: return "l_metacarpophalangeal_2";
    case HanimJointNameValues::L_CARPAL_PROXIMAL_INTERPHALANGEAL_2: return "l_carpal_proximal_interphalangeal_2";
    case HanimJointNameValues::L_CARPAL_DISTAL_INTERPHALANGEAL_2: return "l_carpal_distal_interphalangeal_2";
    case HanimJointNameValues::L_MIDCARPAL_3: return "l_midcarpal_3";
    case HanimJointNameValues::L_CARPOMETACARPAL_3: return "l_carpometacarpal_3";
    case HanimJointNameValues::L_METACARPOPHALANGEAL_3: return "l_metacarpophalangeal_3";
    case HanimJointNameValues::L_CARPAL_PROXIMAL_INTERPHALANGEAL_3: return "l_carpal_proximal_interphalangeal_3";
    case HanimJointNameValues::L_CARPAL_DISTAL_INTERPHALANGEAL_3: return "l_carpal_distal_interphalangeal_3";
    case HanimJointNameValues::L_MIDCARPAL_4_5: return "l_midcarpal_4_5";
    case HanimJointNameValues::L_CARPOMETACARPAL_4: return "l_carpometacarpal_4";
    case HanimJointNameValues::L_METACARPOPHALANGEAL_4: return "l_metacarpophalangeal_4";
    case HanimJointNameValues::L_CARPAL_PROXIMAL_INTERPHALANGEAL_4: return "l_carpal_proximal_interphalangeal_4";
    case HanimJointNameValues::L_CARPAL_DISTAL_INTERPHALANGEAL_4: return "l_carpal_distal_interphalangeal_4";
    case HanimJointNameValues::L_CARPOMETACARPAL_5: return "l_carpometacarpal_5";
    case HanimJointNameValues::L_METACARPOPHALANGEAL_5: return "l_metacarpophalangeal_5";
    case HanimJointNameValues::L_CARPAL_PROXIMAL_INTERPHALANGEAL_5: return "l_carpal_proximal_interphalangeal_5";
    case HanimJointNameValues::L_CARPAL_DISTAL_INTERPHALANGEAL_5: return "l_carpal_distal_interphalangeal_5";
    case HanimJointNameValues::R_STERNOCLAVICULAR: return "r_sternoclavicular";
    case HanimJointNameValues::R_ACROMIOCLAVICULAR: return "r_acromioclavicular";
    case HanimJointNameValues::R_SHOULDER: return "r_shoulder";
    case HanimJointNameValues::R_ELBOW: return "r_elbow";
    case HanimJointNameValues::R_RADIOCARPAL: return "r_radiocarpal";
    case HanimJointNameValues::R_MIDCARPAL_1: return "r_midcarpal_1";
    case HanimJointNameValues::R_CARPOMETACARPAL_1: return "r_carpometacarpal_1";
    case HanimJointNameValues::R_METACARPOPHALANGEAL_1: return "r_metacarpophalangeal_1";
    case HanimJointNameValues::R_CARPAL_INTERPHALANGEAL_1: return "r_carpal_interphalangeal_1";
    case HanimJointNameValues::R_MIDCARPAL_2: return "r_midcarpal_2";
    case HanimJointNameValues::R_CARPOMETACARPAL_2: return "r_carpometacarpal_2";
    case HanimJointNameValues::R_METACARPOPHALANGEAL_2: return "r_metacarpophalangeal_2";
    case HanimJointNameValues::R_CARPAL_PROXIMAL_INTERPHALANGEAL_2: return "r_carpal_proximal_interphalangeal_2";
    case HanimJointNameValues::R_CARPAL_DISTAL_INTERPHALANGEAL_2: return "r_carpal_distal_interphalangeal_2";
    case HanimJointNameValues::R_MIDCARPAL_3: return "r_midcarpal_3";
    case HanimJointNameValues::R_CARPOMETACARPAL_3: return "r_carpometacarpal_3";
    case HanimJointNameValues::R_METACARPOPHALANGEAL_3: return "r_metacarpophalangeal_3";
    case HanimJointNameValues::R_CARPAL_PROXIMAL_INTERPHALANGEAL_3: return "r_carpal_proximal_interphalangeal_3";
    case HanimJointNameValues::R_CARPAL_DISTAL_INTERPHALANGEAL_3: return "r_carpal_distal_interphalangeal_3";
    case HanimJointNameValues::R_MIDCARPAL_4_5: return "r_midcarpal_4_5";
    case HanimJointNameValues::R_CARPOMETACARPAL_4: return "r_carpometacarpal_4";
    case HanimJointNameValues::R_METACARPOPHALANGEAL_4: return "r_metacarpophalangeal_4";
    case HanimJointNameValues::R_CARPAL_PROXIMAL_INTERPHALANGEAL_4: return "r_carpal_proximal_interphalangeal_4";
    case HanimJointNameValues::R_CARPAL_DISTAL_INTERPHALANGEAL_4: return "r_carpal_distal_interphalangeal_4";
    case HanimJointNameValues::R_CARPOMETACARPAL_5: return "r_carpometacarpal_5";
    case HanimJointNameValues::R_METACARPOPHALANGEAL_5: return "r_metacarpophalangeal_5";
    case HanimJointNameValues::R_CARPAL_PROXIMAL_INTERPHALANGEAL_5: return "r_carpal_proximal_interphalangeal_5";
    case HanimJointNameValues::R_CARPAL_DISTAL_INTERPHALANGEAL_5: return "r_carpal_distal_interphalangeal_5";
    }
    return "";
}

inline bool from_string(const std::string& token, HanimJointNameValues& out) {
    if (token == "humanoid_root") { out = HanimJointNameValues::HUMANOID_ROOT; return true; }
    if (token == "sacroiliac") { out = HanimJointNameValues::SACROILIAC; return true; }
    if (token == "l_hip") { out = HanimJointNameValues::L_HIP; return true; }
    if (token == "l_knee") { out = HanimJointNameValues::L_KNEE; return true; }
    if (token == "l_talocrural") { out = HanimJointNameValues::L_TALOCRURAL; return true; }
    if (token == "l_talocalcaneonavicular") { out = HanimJointNameValues::L_TALOCALCANEONAVICULAR; return true; }
    if (token == "l_cuneonavicular_1") { out = HanimJointNameValues::L_CUNEONAVICULAR_1; return true; }
    if (token == "l_tarsometatarsal_1") { out = HanimJointNameValues::L_TARSOMETATARSAL_1; return true; }
    if (token == "l_metatarsophalangeal_1") { out = HanimJointNameValues::L_METATARSOPHALANGEAL_1; return true; }
    if (token == "l_tarsal_interphalangeal_1") { out = HanimJointNameValues::L_TARSAL_INTERPHALANGEAL_1; return true; }
    if (token == "l_cuneonavicular_2") { out = HanimJointNameValues::L_CUNEONAVICULAR_2; return true; }
    if (token == "l_tarsometatarsal_2") { out = HanimJointNameValues::L_TARSOMETATARSAL_2; return true; }
    if (token == "l_metatarsophalangeal_2") { out = HanimJointNameValues::L_METATARSOPHALANGEAL_2; return true; }
    if (token == "l_tarsal_proximal_interphalangeal_2") { out = HanimJointNameValues::L_TARSAL_PROXIMAL_INTERPHALANGEAL_2; return true; }
    if (token == "l_tarsal_distal_interphalangeal_2") { out = HanimJointNameValues::L_TARSAL_DISTAL_INTERPHALANGEAL_2; return true; }
    if (token == "l_cuneonavicular_3") { out = HanimJointNameValues::L_CUNEONAVICULAR_3; return true; }
    if (token == "l_tarsometatarsal_3") { out = HanimJointNameValues::L_TARSOMETATARSAL_3; return true; }
    if (token == "l_metatarsophalangeal_3") { out = HanimJointNameValues::L_METATARSOPHALANGEAL_3; return true; }
    if (token == "l_tarsal_proximal_interphalangeal_3") { out = HanimJointNameValues::L_TARSAL_PROXIMAL_INTERPHALANGEAL_3; return true; }
    if (token == "l_tarsal_distal_interphalangeal_3") { out = HanimJointNameValues::L_TARSAL_DISTAL_INTERPHALANGEAL_3; return true; }
    if (token == "l_calcaneocuboid") { out = HanimJointNameValues::L_CALCANEOCUBOID; return true; }
    if (token == "l_transversetarsal") { out = HanimJointNameValues::L_TRANSVERSETARSAL; return true; }
    if (token == "l_tarsometatarsal_4") { out = HanimJointNameValues::L_TARSOMETATARSAL_4; return true; }
    if (token == "l_metatarsophalangeal_4") { out = HanimJointNameValues::L_METATARSOPHALANGEAL_4; return true; }
    if (token == "l_tarsal_proximal_interphalangeal_4") { out = HanimJointNameValues::L_TARSAL_PROXIMAL_INTERPHALANGEAL_4; return true; }
    if (token == "l_tarsal_distal_interphalangeal_4") { out = HanimJointNameValues::L_TARSAL_DISTAL_INTERPHALANGEAL_4; return true; }
    if (token == "l_tarsometatarsal_5") { out = HanimJointNameValues::L_TARSOMETATARSAL_5; return true; }
    if (token == "l_metatarsophalangeal_5") { out = HanimJointNameValues::L_METATARSOPHALANGEAL_5; return true; }
    if (token == "l_tarsal_proximal_interphalangeal_5") { out = HanimJointNameValues::L_TARSAL_PROXIMAL_INTERPHALANGEAL_5; return true; }
    if (token == "l_tarsal_distal_interphalangeal_5") { out = HanimJointNameValues::L_TARSAL_DISTAL_INTERPHALANGEAL_5; return true; }
    if (token == "r_hip") { out = HanimJointNameValues::R_HIP; return true; }
    if (token == "r_knee") { out = HanimJointNameValues::R_KNEE; return true; }
    if (token == "r_talocrural") { out = HanimJointNameValues::R_TALOCRURAL; return true; }
    if (token == "r_talocalcaneonavicular") { out = HanimJointNameValues::R_TALOCALCANEONAVICULAR; return true; }
    if (token == "r_cuneonavicular_1") { out = HanimJointNameValues::R_CUNEONAVICULAR_1; return true; }
    if (token == "r_tarsometatarsal_1") { out = HanimJointNameValues::R_TARSOMETATARSAL_1; return true; }
    if (token == "r_metatarsophalangeal_1") { out = HanimJointNameValues::R_METATARSOPHALANGEAL_1; return true; }
    if (token == "r_tarsal_interphalangeal_1") { out = HanimJointNameValues::R_TARSAL_INTERPHALANGEAL_1; return true; }
    if (token == "r_cuneonavicular_2") { out = HanimJointNameValues::R_CUNEONAVICULAR_2; return true; }
    if (token == "r_tarsometatarsal_2") { out = HanimJointNameValues::R_TARSOMETATARSAL_2; return true; }
    if (token == "r_metatarsophalangeal_2") { out = HanimJointNameValues::R_METATARSOPHALANGEAL_2; return true; }
    if (token == "r_tarsal_proximal_interphalangeal_2") { out = HanimJointNameValues::R_TARSAL_PROXIMAL_INTERPHALANGEAL_2; return true; }
    if (token == "r_tarsal_distal_interphalangeal_2") { out = HanimJointNameValues::R_TARSAL_DISTAL_INTERPHALANGEAL_2; return true; }
    if (token == "r_cuneonavicular_3") { out = HanimJointNameValues::R_CUNEONAVICULAR_3; return true; }
    if (token == "r_tarsometatarsal_3") { out = HanimJointNameValues::R_TARSOMETATARSAL_3; return true; }
    if (token == "r_metatarsophalangeal_3") { out = HanimJointNameValues::R_METATARSOPHALANGEAL_3; return true; }
    if (token == "r_tarsal_proximal_interphalangeal_3") { out = HanimJointNameValues::R_TARSAL_PROXIMAL_INTERPHALANGEAL_3; return true; }
    if (token == "r_tarsal_distal_interphalangeal_3") { out = HanimJointNameValues::R_TARSAL_DISTAL_INTERPHALANGEAL_3; return true; }
    if (token == "r_calcaneocuboid") { out = HanimJointNameValues::R_CALCANEOCUBOID; return true; }
    if (token == "r_transversetarsal") { out = HanimJointNameValues::R_TRANSVERSETARSAL; return true; }
    if (token == "r_tarsometatarsal_4") { out = HanimJointNameValues::R_TARSOMETATARSAL_4; return true; }
    if (token == "r_metatarsophalangeal_4") { out = HanimJointNameValues::R_METATARSOPHALANGEAL_4; return true; }
    if (token == "r_tarsal_proximal_interphalangeal_4") { out = HanimJointNameValues::R_TARSAL_PROXIMAL_INTERPHALANGEAL_4; return true; }
    if (token == "r_tarsal_distal_interphalangeal_4") { out = HanimJointNameValues::R_TARSAL_DISTAL_INTERPHALANGEAL_4; return true; }
    if (token == "r_tarsometatarsal_5") { out = HanimJointNameValues::R_TARSOMETATARSAL_5; return true; }
    if (token == "r_metatarsophalangeal_5") { out = HanimJointNameValues::R_METATARSOPHALANGEAL_5; return true; }
    if (token == "r_tarsal_proximal_interphalangeal_5") { out = HanimJointNameValues::R_TARSAL_PROXIMAL_INTERPHALANGEAL_5; return true; }
    if (token == "r_tarsal_distal_interphalangeal_5") { out = HanimJointNameValues::R_TARSAL_DISTAL_INTERPHALANGEAL_5; return true; }
    if (token == "vl5") { out = HanimJointNameValues::VL5; return true; }
    if (token == "vl4") { out = HanimJointNameValues::VL4; return true; }
    if (token == "vl3") { out = HanimJointNameValues::VL3; return true; }
    if (token == "vl2") { out = HanimJointNameValues::VL2; return true; }
    if (token == "vl1") { out = HanimJointNameValues::VL1; return true; }
    if (token == "vt12") { out = HanimJointNameValues::VT12; return true; }
    if (token == "vt11") { out = HanimJointNameValues::VT11; return true; }
    if (token == "vt10") { out = HanimJointNameValues::VT10; return true; }
    if (token == "vt9") { out = HanimJointNameValues::VT9; return true; }
    if (token == "vt8") { out = HanimJointNameValues::VT8; return true; }
    if (token == "vt7") { out = HanimJointNameValues::VT7; return true; }
    if (token == "vt6") { out = HanimJointNameValues::VT6; return true; }
    if (token == "vt5") { out = HanimJointNameValues::VT5; return true; }
    if (token == "vt4") { out = HanimJointNameValues::VT4; return true; }
    if (token == "vt3") { out = HanimJointNameValues::VT3; return true; }
    if (token == "vt2") { out = HanimJointNameValues::VT2; return true; }
    if (token == "vt1") { out = HanimJointNameValues::VT1; return true; }
    if (token == "vc7") { out = HanimJointNameValues::VC7; return true; }
    if (token == "vc6") { out = HanimJointNameValues::VC6; return true; }
    if (token == "vc5") { out = HanimJointNameValues::VC5; return true; }
    if (token == "vc4") { out = HanimJointNameValues::VC4; return true; }
    if (token == "vc3") { out = HanimJointNameValues::VC3; return true; }
    if (token == "vc2") { out = HanimJointNameValues::VC2; return true; }
    if (token == "vc1") { out = HanimJointNameValues::VC1; return true; }
    if (token == "skullbase") { out = HanimJointNameValues::SKULLBASE; return true; }
    if (token == "l_eyelid_joint") { out = HanimJointNameValues::L_EYELID_JOINT; return true; }
    if (token == "r_eyelid_joint") { out = HanimJointNameValues::R_EYELID_JOINT; return true; }
    if (token == "l_eyeball_joint") { out = HanimJointNameValues::L_EYEBALL_JOINT; return true; }
    if (token == "r_eyeball_joint") { out = HanimJointNameValues::R_EYEBALL_JOINT; return true; }
    if (token == "l_eyebrow_joint") { out = HanimJointNameValues::L_EYEBROW_JOINT; return true; }
    if (token == "r_eyebrow_joint") { out = HanimJointNameValues::R_EYEBROW_JOINT; return true; }
    if (token == "temporomandibular") { out = HanimJointNameValues::TEMPOROMANDIBULAR; return true; }
    if (token == "l_sternoclavicular") { out = HanimJointNameValues::L_STERNOCLAVICULAR; return true; }
    if (token == "l_acromioclavicular") { out = HanimJointNameValues::L_ACROMIOCLAVICULAR; return true; }
    if (token == "l_shoulder") { out = HanimJointNameValues::L_SHOULDER; return true; }
    if (token == "l_elbow") { out = HanimJointNameValues::L_ELBOW; return true; }
    if (token == "l_radiocarpal") { out = HanimJointNameValues::L_RADIOCARPAL; return true; }
    if (token == "l_midcarpal_1") { out = HanimJointNameValues::L_MIDCARPAL_1; return true; }
    if (token == "l_carpometacarpal_1") { out = HanimJointNameValues::L_CARPOMETACARPAL_1; return true; }
    if (token == "l_metacarpophalangeal_1") { out = HanimJointNameValues::L_METACARPOPHALANGEAL_1; return true; }
    if (token == "l_carpal_interphalangeal_1") { out = HanimJointNameValues::L_CARPAL_INTERPHALANGEAL_1; return true; }
    if (token == "l_midcarpal_2") { out = HanimJointNameValues::L_MIDCARPAL_2; return true; }
    if (token == "l_carpometacarpal_2") { out = HanimJointNameValues::L_CARPOMETACARPAL_2; return true; }
    if (token == "l_metacarpophalangeal_2") { out = HanimJointNameValues::L_METACARPOPHALANGEAL_2; return true; }
    if (token == "l_carpal_proximal_interphalangeal_2") { out = HanimJointNameValues::L_CARPAL_PROXIMAL_INTERPHALANGEAL_2; return true; }
    if (token == "l_carpal_distal_interphalangeal_2") { out = HanimJointNameValues::L_CARPAL_DISTAL_INTERPHALANGEAL_2; return true; }
    if (token == "l_midcarpal_3") { out = HanimJointNameValues::L_MIDCARPAL_3; return true; }
    if (token == "l_carpometacarpal_3") { out = HanimJointNameValues::L_CARPOMETACARPAL_3; return true; }
    if (token == "l_metacarpophalangeal_3") { out = HanimJointNameValues::L_METACARPOPHALANGEAL_3; return true; }
    if (token == "l_carpal_proximal_interphalangeal_3") { out = HanimJointNameValues::L_CARPAL_PROXIMAL_INTERPHALANGEAL_3; return true; }
    if (token == "l_carpal_distal_interphalangeal_3") { out = HanimJointNameValues::L_CARPAL_DISTAL_INTERPHALANGEAL_3; return true; }
    if (token == "l_midcarpal_4_5") { out = HanimJointNameValues::L_MIDCARPAL_4_5; return true; }
    if (token == "l_carpometacarpal_4") { out = HanimJointNameValues::L_CARPOMETACARPAL_4; return true; }
    if (token == "l_metacarpophalangeal_4") { out = HanimJointNameValues::L_METACARPOPHALANGEAL_4; return true; }
    if (token == "l_carpal_proximal_interphalangeal_4") { out = HanimJointNameValues::L_CARPAL_PROXIMAL_INTERPHALANGEAL_4; return true; }
    if (token == "l_carpal_distal_interphalangeal_4") { out = HanimJointNameValues::L_CARPAL_DISTAL_INTERPHALANGEAL_4; return true; }
    if (token == "l_carpometacarpal_5") { out = HanimJointNameValues::L_CARPOMETACARPAL_5; return true; }
    if (token == "l_metacarpophalangeal_5") { out = HanimJointNameValues::L_METACARPOPHALANGEAL_5; return true; }
    if (token == "l_carpal_proximal_interphalangeal_5") { out = HanimJointNameValues::L_CARPAL_PROXIMAL_INTERPHALANGEAL_5; return true; }
    if (token == "l_carpal_distal_interphalangeal_5") { out = HanimJointNameValues::L_CARPAL_DISTAL_INTERPHALANGEAL_5; return true; }
    if (token == "r_sternoclavicular") { out = HanimJointNameValues::R_STERNOCLAVICULAR; return true; }
    if (token == "r_acromioclavicular") { out = HanimJointNameValues::R_ACROMIOCLAVICULAR; return true; }
    if (token == "r_shoulder") { out = HanimJointNameValues::R_SHOULDER; return true; }
    if (token == "r_elbow") { out = HanimJointNameValues::R_ELBOW; return true; }
    if (token == "r_radiocarpal") { out = HanimJointNameValues::R_RADIOCARPAL; return true; }
    if (token == "r_midcarpal_1") { out = HanimJointNameValues::R_MIDCARPAL_1; return true; }
    if (token == "r_carpometacarpal_1") { out = HanimJointNameValues::R_CARPOMETACARPAL_1; return true; }
    if (token == "r_metacarpophalangeal_1") { out = HanimJointNameValues::R_METACARPOPHALANGEAL_1; return true; }
    if (token == "r_carpal_interphalangeal_1") { out = HanimJointNameValues::R_CARPAL_INTERPHALANGEAL_1; return true; }
    if (token == "r_midcarpal_2") { out = HanimJointNameValues::R_MIDCARPAL_2; return true; }
    if (token == "r_carpometacarpal_2") { out = HanimJointNameValues::R_CARPOMETACARPAL_2; return true; }
    if (token == "r_metacarpophalangeal_2") { out = HanimJointNameValues::R_METACARPOPHALANGEAL_2; return true; }
    if (token == "r_carpal_proximal_interphalangeal_2") { out = HanimJointNameValues::R_CARPAL_PROXIMAL_INTERPHALANGEAL_2; return true; }
    if (token == "r_carpal_distal_interphalangeal_2") { out = HanimJointNameValues::R_CARPAL_DISTAL_INTERPHALANGEAL_2; return true; }
    if (token == "r_midcarpal_3") { out = HanimJointNameValues::R_MIDCARPAL_3; return true; }
    if (token == "r_carpometacarpal_3") { out = HanimJointNameValues::R_CARPOMETACARPAL_3; return true; }
    if (token == "r_metacarpophalangeal_3") { out = HanimJointNameValues::R_METACARPOPHALANGEAL_3; return true; }
    if (token == "r_carpal_proximal_interphalangeal_3") { out = HanimJointNameValues::R_CARPAL_PROXIMAL_INTERPHALANGEAL_3; return true; }
    if (token == "r_carpal_distal_interphalangeal_3") { out = HanimJointNameValues::R_CARPAL_DISTAL_INTERPHALANGEAL_3; return true; }
    if (token == "r_midcarpal_4_5") { out = HanimJointNameValues::R_MIDCARPAL_4_5; return true; }
    if (token == "r_carpometacarpal_4") { out = HanimJointNameValues::R_CARPOMETACARPAL_4; return true; }
    if (token == "r_metacarpophalangeal_4") { out = HanimJointNameValues::R_METACARPOPHALANGEAL_4; return true; }
    if (token == "r_carpal_proximal_interphalangeal_4") { out = HanimJointNameValues::R_CARPAL_PROXIMAL_INTERPHALANGEAL_4; return true; }
    if (token == "r_carpal_distal_interphalangeal_4") { out = HanimJointNameValues::R_CARPAL_DISTAL_INTERPHALANGEAL_4; return true; }
    if (token == "r_carpometacarpal_5") { out = HanimJointNameValues::R_CARPOMETACARPAL_5; return true; }
    if (token == "r_metacarpophalangeal_5") { out = HanimJointNameValues::R_METACARPOPHALANGEAL_5; return true; }
    if (token == "r_carpal_proximal_interphalangeal_5") { out = HanimJointNameValues::R_CARPAL_PROXIMAL_INTERPHALANGEAL_5; return true; }
    if (token == "r_carpal_distal_interphalangeal_5") { out = HanimJointNameValues::R_CARPAL_DISTAL_INTERPHALANGEAL_5; return true; }
    return false;
}

inline std::string to_string(HanimSegmentNameValues value) {
    switch (value) {
    case HanimSegmentNameValues::SACRUM: return "sacrum";
    case HanimSegmentNameValues::PELVIS: return "pelvis";
    case HanimSegmentNameValues::L_THIGH: return "l_thigh";
    case HanimSegmentNameValues::L_CALF: return "l_calf";
    case HanimSegmentNameValues::L_TALUS: return "l_talus";
    case HanimSegmentNameValues::L_NAVICULAR: return "l_navicular";
    case HanimSegmentNameValues::L_CUNEIFORM_1: return "l_cuneiform_1";
    case HanimSegmentNameValues::L_METATARSAL_1: return "l_metatarsal_1";
    case HanimSegmentNameValues::L_TARSAL_PROXIMAL_PHALANX_1: return "l_tarsal_proximal_phalanx_1";
    case HanimSegmentNameValues::L_TARSAL_DISTAL_PHALANX_1: return "l_tarsal_distal_phalanx_1";
    case HanimSegmentNameValues::L_CUNEIFORM_2: return "l_cuneiform_2";
    case HanimSegmentNameValues::L_METATARSAL_2: return "l_metatarsal_2";
    case HanimSegmentNameValues::L_TARSAL_PROXIMAL_PHALANX_2: return "l_tarsal_proximal_phalanx_2";
    case HanimSegmentNameValues::L_TARSAL_MIDDLE_PHALANX_2: return "l_tarsal_middle_phalanx_2";
    case HanimSegmentNameValues::L_TARSAL_DISTAL_PHALANX_2: return "l_tarsal_distal_phalanx_2";
    case HanimSegmentNameValues::L_CUNEIFORM_3: return "l_cuneiform_3";
    case HanimSegmentNameValues::L_METATARSAL_3: return "l_metatarsal_3";
    case HanimSegmentNameValues::L_TARSAL_PROXIMAL_PHALANX_3: return "l_tarsal_proximal_phalanx_3";
    case HanimSegmentNameValues::L_TARSAL_MIDDLE_PHALANX_3: return "l_tarsal_middle_phalanx_3";
    case HanimSegmentNameValues::L_TARSAL_DISTAL_PHALANX_3: return "l_tarsal_distal_phalanx_3";
    case HanimSegmentNameValues::L_CALCANEUS: return "l_calcaneus";
    case HanimSegmentNameValues::L_CUBOID: return "l_cuboid";
    case HanimSegmentNameValues::L_METATARSAL_4: return "l_metatarsal_4";
    case HanimSegmentNameValues::L_TARSAL_PROXIMAL_PHALANX_4: return "l_tarsal_proximal_phalanx_4";
    case HanimSegmentNameValues::L_TARSAL_MIDDLE_PHALANX_4: return "l_tarsal_middle_phalanx_4";
    case HanimSegmentNameValues::L_TARSAL_DISTAL_PHALANX_4: return "l_tarsal_distal_phalanx_4";
    case HanimSegmentNameValues::L_METATARSAL_5: return "l_metatarsal_5";
    case HanimSegmentNameValues::L_TARSAL_PROXIMAL_PHALANX_5: return "l_tarsal_proximal_phalanx_5";
    case HanimSegmentNameValues::L_TARSAL_MIDDLE_PHALANX_5: return "l_tarsal_middle_phalanx_5";
    case HanimSegmentNameValues::L_TARSAL_DISTAL_PHALANX_5: return "l_tarsal_distal_phalanx_5";
    case HanimSegmentNameValues::R_THIGH: return "r_thigh";
    case HanimSegmentNameValues::R_CALF: return "r_calf";
    case HanimSegmentNameValues::R_TALUS: return "r_talus";
    case HanimSegmentNameValues::R_NAVICULAR: return "r_navicular";
    case HanimSegmentNameValues::R_CUNEIFORM_1: return "r_cuneiform_1";
    case HanimSegmentNameValues::R_METATARSAL_1: return "r_metatarsal_1";
    case HanimSegmentNameValues::R_TARSAL_PROXIMAL_PHALANX_1: return "r_tarsal_proximal_phalanx_1";
    case HanimSegmentNameValues::R_TARSAL_DISTAL_PHALANX_1: return "r_tarsal_distal_phalanx_1";
    case HanimSegmentNameValues::R_CUNEIFORM_2: return "r_cuneiform_2";
    case HanimSegmentNameValues::R_METATARSAL_2: return "r_metatarsal_2";
    case HanimSegmentNameValues::R_TARSAL_PROXIMAL_PHALANX_2: return "r_tarsal_proximal_phalanx_2";
    case HanimSegmentNameValues::R_TARSAL_MIDDLE_PHALANX_2: return "r_tarsal_middle_phalanx_2";
    case HanimSegmentNameValues::R_TARSAL_DISTAL_PHALANX_2: return "r_tarsal_distal_phalanx_2";
    case HanimSegmentNameValues::R_CUNEIFORM_3: return "r_cuneiform_3";
    case HanimSegmentNameValues::R_METATARSAL_3: return "r_metatarsal_3";
    case HanimSegmentNameValues::R_TARSAL_PROXIMAL_PHALANX_3: return "r_tarsal_proximal_phalanx_3";
    case HanimSegmentNameValues::R_TARSAL_MIDDLE_PHALANX_3: return "r_tarsal_middle_phalanx_3";
    case HanimSegmentNameValues::R_TARSAL_DISTAL_PHALANX_3: return "r_tarsal_distal_phalanx_3";
    case HanimSegmentNameValues::R_CALCANEUS: return "r_calcaneus";
    case HanimSegmentNameValues::R_CUBOID: return "r_cuboid";
    case HanimSegmentNameValues::R_METATARSAL_4: return "r_metatarsal_4";
    case HanimSegmentNameValues::R_TARSAL_PROXIMAL_PHALANX_4: return "r_tarsal_proximal_phalanx_4";
    case HanimSegmentNameValues::R_TARSAL_MIDDLE_PHALANX_4: return "r_tarsal_middle_phalanx_4";
    case HanimSegmentNameValues::R_TARSAL_DISTAL_PHALANX_4: return "r_tarsal_distal_phalanx_4";
    case HanimSegmentNameValues::R_METATARSAL_5: return "r_metatarsal_5";
    case HanimSegmentNameValues::R_TARSAL_PROXIMAL_PHALANX_5: return "r_tarsal_proximal_phalanx_5";
    case HanimSegmentNameValues::R_TARSAL_MIDDLE_PHALANX_5: return "r_tarsal_middle_phalanx_5";
    case HanimSegmentNameValues::R_TARSAL_DISTAL_PHALANX_5: return "r_tarsal_distal_phalanx_5";
    case HanimSegmentNameValues::L5: return "l5";
    case HanimSegmentNameValues::L4: return "l4";
    case HanimSegmentNameValues::L3: return "l3";
    case HanimSegmentNameValues::L2: return "l2";
    case HanimSegmentNameValues::L1: return "l1";
    case HanimSegmentNameValues::T12: return "t12";
    case HanimSegmentNameValues::T11: return "t11";
    case HanimSegmentNameValues::T10: return "t10";
    case HanimSegmentNameValues::T9: return "t9";
    case HanimSegmentNameValues::T8: return "t8";
    case HanimSegmentNameValues::T7: return "t7";
    case HanimSegmentNameValues::T6: return "t6";
    case HanimSegmentNameValues::T5: return "t5";
    case HanimSegmentNameValues::T4: return "t4";
    case HanimSegmentNameValues::T3: return "t3";
    case HanimSegmentNameValues::T2: return "t2";
    case HanimSegmentNameValues::T1: return "t1";
    case HanimSegmentNameValues::C7: return "c7";
    case HanimSegmentNameValues::C6: return "c6";
    case HanimSegmentNameValues::C5: return "c5";
    case HanimSegmentNameValues::C4: return "c4";
    case HanimSegmentNameValues::C3: return "c3";
    case HanimSegmentNameValues::C2: return "c2";
    case HanimSegmentNameValues::C1: return "c1";
    case HanimSegmentNameValues::SKULL: return "skull";
    case HanimSegmentNameValues::L_EYELID: return "l_eyelid";
    case HanimSegmentNameValues::R_EYELID: return "r_eyelid";
    case HanimSegmentNameValues::L_EYEBALL: return "l_eyeball";
    case HanimSegmentNameValues::R_EYEBALL: return "r_eyeball";
    case HanimSegmentNameValues::L_EYEBROW: return "l_eyebrow";
    case HanimSegmentNameValues::R_EYEBROW: return "r_eyebrow";
    case HanimSegmentNameValues::JAW: return "jaw";
    case HanimSegmentNameValues::L_CLAVICLE: return "l_clavicle";
    case HanimSegmentNameValues::L_SCAPULA: return "l_scapula";
    case HanimSegmentNameValues::L_UPPERARM: return "l_upperarm";
    case HanimSegmentNameValues::L_FOREARM: return "l_forearm";
    case HanimSegmentNameValues::L_CARPAL: return "l_carpal";
    case HanimSegmentNameValues::L_TRAPEZIUM: return "l_trapezium";
    case HanimSegmentNameValues::L_METACARPAL_1: return "l_metacarpal_1";
    case HanimSegmentNameValues::L_CARPAL_PROXIMAL_PHALANX_1: return "l_carpal_proximal_phalanx_1";
    case HanimSegmentNameValues::L_CARPAL_DISTAL_PHALANX_1: return "l_carpal_distal_phalanx_1";
    case HanimSegmentNameValues::L_TRAPEZOID: return "l_trapezoid";
    case HanimSegmentNameValues::L_METACARPAL_2: return "l_metacarpal_2";
    case HanimSegmentNameValues::L_CARPAL_PROXIMAL_PHALANX_2: return "l_carpal_proximal_phalanx_2";
    case HanimSegmentNameValues::L_CARPAL_MIDDLE_PHALANX_2: return "l_carpal_middle_phalanx_2";
    case HanimSegmentNameValues::L_CARPAL_DISTAL_PHALANX_2: return "l_carpal_distal_phalanx_2";
    case HanimSegmentNameValues::L_CAPITATE: return "l_capitate";
    case HanimSegmentNameValues::L_METACARPAL_3: return "l_metacarpal_3";
    case HanimSegmentNameValues::L_CARPAL_PROXIMAL_PHALANX_3: return "l_carpal_proximal_phalanx_3";
    case HanimSegmentNameValues::L_CARPAL_MIDDLE_PHALANX_3: return "l_carpal_middle_phalanx_3";
    case HanimSegmentNameValues::L_CARPAL_DISTAL_PHALANX_3: return "l_carpal_distal_phalanx_3";
    case HanimSegmentNameValues::L_HAMATE: return "l_hamate";
    case HanimSegmentNameValues::L_METACARPAL_4: return "l_metacarpal_4";
    case HanimSegmentNameValues::L_CARPAL_PROXIMAL_PHALANX_4: return "l_carpal_proximal_phalanx_4";
    case HanimSegmentNameValues::L_CARPAL_MIDDLE_PHALANX_4: return "l_carpal_middle_phalanx_4";
    case HanimSegmentNameValues::L_CARPAL_DISTAL_PHALANX_4: return "l_carpal_distal_phalanx_4";
    case HanimSegmentNameValues::L_METACARPAL_5: return "l_metacarpal_5";
    case HanimSegmentNameValues::L_CARPAL_PROXIMAL_PHALANX_5: return "l_carpal_proximal_phalanx_5";
    case HanimSegmentNameValues::L_CARPAL_MIDDLE_PHALANX_5: return "l_carpal_middle_phalanx_5";
    case HanimSegmentNameValues::L_CARPAL_DISTAL_PHALANX_5: return "l_carpal_distal_phalanx_5";
    case HanimSegmentNameValues::R_CLAVICLE: return "r_clavicle";
    case HanimSegmentNameValues::R_SCAPULA: return "r_scapula";
    case HanimSegmentNameValues::R_UPPERARM: return "r_upperarm";
    case HanimSegmentNameValues::R_FOREARM: return "r_forearm";
    case HanimSegmentNameValues::R_CARPAL: return "r_carpal";
    case HanimSegmentNameValues::R_TRAPEZIUM: return "r_trapezium";
    case HanimSegmentNameValues::R_METACARPAL_1: return "r_metacarpal_1";
    case HanimSegmentNameValues::R_CARPAL_PROXIMAL_PHALANX_1: return "r_carpal_proximal_phalanx_1";
    case HanimSegmentNameValues::R_CARPAL_DISTAL_PHALANX_1: return "r_carpal_distal_phalanx_1";
    case HanimSegmentNameValues::R_TRAPEZOID: return "r_trapezoid";
    case HanimSegmentNameValues::R_METACARPAL_2: return "r_metacarpal_2";
    case HanimSegmentNameValues::R_CARPAL_PROXIMAL_PHALANX_2: return "r_carpal_proximal_phalanx_2";
    case HanimSegmentNameValues::R_CARPAL_MIDDLE_PHALANX_2: return "r_carpal_middle_phalanx_2";
    case HanimSegmentNameValues::R_CARPAL_DISTAL_PHALANX_2: return "r_carpal_distal_phalanx_2";
    case HanimSegmentNameValues::R_CAPITATE: return "r_capitate";
    case HanimSegmentNameValues::R_METACARPAL_3: return "r_metacarpal_3";
    case HanimSegmentNameValues::R_CARPAL_PROXIMAL_PHALANX_3: return "r_carpal_proximal_phalanx_3";
    case HanimSegmentNameValues::R_CARPAL_MIDDLE_PHALANX_3: return "r_carpal_middle_phalanx_3";
    case HanimSegmentNameValues::R_CARPAL_DISTAL_PHALANX_3: return "r_carpal_distal_phalanx_3";
    case HanimSegmentNameValues::R_HAMATE: return "r_hamate";
    case HanimSegmentNameValues::R_METACARPAL_4: return "r_metacarpal_4";
    case HanimSegmentNameValues::R_CARPAL_PROXIMAL_PHALANX_4: return "r_carpal_proximal_phalanx_4";
    case HanimSegmentNameValues::R_CARPAL_MIDDLE_PHALANX_4: return "r_carpal_middle_phalanx_4";
    case HanimSegmentNameValues::R_CARPAL_DISTAL_PHALANX_4: return "r_carpal_distal_phalanx_4";
    case HanimSegmentNameValues::R_METACARPAL_5: return "r_metacarpal_5";
    case HanimSegmentNameValues::R_CARPAL_PROXIMAL_PHALANX_5: return "r_carpal_proximal_phalanx_5";
    case HanimSegmentNameValues::R_CARPAL_MIDDLE_PHALANX_5: return "r_carpal_middle_phalanx_5";
    case HanimSegmentNameValues::R_CARPAL_DISTAL_PHALANX_5: return "r_carpal_distal_phalanx_5";
    }
    return "";
}

inline bool from_string(const std::string& token, HanimSegmentNameValues& out) {
    if (token == "sacrum") { out = HanimSegmentNameValues::SACRUM; return true; }
    if (token == "pelvis") { out = HanimSegmentNameValues::PELVIS; return true; }
    if (token == "l_thigh") { out = HanimSegmentNameValues::L_THIGH; return true; }
    if (token == "l_calf") { out = HanimSegmentNameValues::L_CALF; return true; }
    if (token == "l_talus") { out = HanimSegmentNameValues::L_TALUS; return true; }
    if (token == "l_navicular") { out = HanimSegmentNameValues::L_NAVICULAR; return true; }
    if (token == "l_cuneiform_1") { out = HanimSegmentNameValues::L_CUNEIFORM_1; return true; }
    if (token == "l_metatarsal_1") { out = HanimSegmentNameValues::L_METATARSAL_1; return true; }
    if (token == "l_tarsal_proximal_phalanx_1") { out = HanimSegmentNameValues::L_TARSAL_PROXIMAL_PHALANX_1; return true; }
    if (token == "l_tarsal_distal_phalanx_1") { out = HanimSegmentNameValues::L_TARSAL_DISTAL_PHALANX_1; return true; }
    if (token == "l_cuneiform_2") { out = HanimSegmentNameValues::L_CUNEIFORM_2; return true; }
    if (token == "l_metatarsal_2") { out = HanimSegmentNameValues::L_METATARSAL_2; return true; }
    if (token == "l_tarsal_proximal_phalanx_2") { out = HanimSegmentNameValues::L_TARSAL_PROXIMAL_PHALANX_2; return true; }
    if (token == "l_tarsal_middle_phalanx_2") { out = HanimSegmentNameValues::L_TARSAL_MIDDLE_PHALANX_2; return true; }
    if (token == "l_tarsal_distal_phalanx_2") { out = HanimSegmentNameValues::L_TARSAL_DISTAL_PHALANX_2; return true; }
    if (token == "l_cuneiform_3") { out = HanimSegmentNameValues::L_CUNEIFORM_3; return true; }
    if (token == "l_metatarsal_3") { out = HanimSegmentNameValues::L_METATARSAL_3; return true; }
    if (token == "l_tarsal_proximal_phalanx_3") { out = HanimSegmentNameValues::L_TARSAL_PROXIMAL_PHALANX_3; return true; }
    if (token == "l_tarsal_middle_phalanx_3") { out = HanimSegmentNameValues::L_TARSAL_MIDDLE_PHALANX_3; return true; }
    if (token == "l_tarsal_distal_phalanx_3") { out = HanimSegmentNameValues::L_TARSAL_DISTAL_PHALANX_3; return true; }
    if (token == "l_calcaneus") { out = HanimSegmentNameValues::L_CALCANEUS; return true; }
    if (token == "l_cuboid") { out = HanimSegmentNameValues::L_CUBOID; return true; }
    if (token == "l_metatarsal_4") { out = HanimSegmentNameValues::L_METATARSAL_4; return true; }
    if (token == "l_tarsal_proximal_phalanx_4") { out = HanimSegmentNameValues::L_TARSAL_PROXIMAL_PHALANX_4; return true; }
    if (token == "l_tarsal_middle_phalanx_4") { out = HanimSegmentNameValues::L_TARSAL_MIDDLE_PHALANX_4; return true; }
    if (token == "l_tarsal_distal_phalanx_4") { out = HanimSegmentNameValues::L_TARSAL_DISTAL_PHALANX_4; return true; }
    if (token == "l_metatarsal_5") { out = HanimSegmentNameValues::L_METATARSAL_5; return true; }
    if (token == "l_tarsal_proximal_phalanx_5") { out = HanimSegmentNameValues::L_TARSAL_PROXIMAL_PHALANX_5; return true; }
    if (token == "l_tarsal_middle_phalanx_5") { out = HanimSegmentNameValues::L_TARSAL_MIDDLE_PHALANX_5; return true; }
    if (token == "l_tarsal_distal_phalanx_5") { out = HanimSegmentNameValues::L_TARSAL_DISTAL_PHALANX_5; return true; }
    if (token == "r_thigh") { out = HanimSegmentNameValues::R_THIGH; return true; }
    if (token == "r_calf") { out = HanimSegmentNameValues::R_CALF; return true; }
    if (token == "r_talus") { out = HanimSegmentNameValues::R_TALUS; return true; }
    if (token == "r_navicular") { out = HanimSegmentNameValues::R_NAVICULAR; return true; }
    if (token == "r_cuneiform_1") { out = HanimSegmentNameValues::R_CUNEIFORM_1; return true; }
    if (token == "r_metatarsal_1") { out = HanimSegmentNameValues::R_METATARSAL_1; return true; }
    if (token == "r_tarsal_proximal_phalanx_1") { out = HanimSegmentNameValues::R_TARSAL_PROXIMAL_PHALANX_1; return true; }
    if (token == "r_tarsal_distal_phalanx_1") { out = HanimSegmentNameValues::R_TARSAL_DISTAL_PHALANX_1; return true; }
    if (token == "r_cuneiform_2") { out = HanimSegmentNameValues::R_CUNEIFORM_2; return true; }
    if (token == "r_metatarsal_2") { out = HanimSegmentNameValues::R_METATARSAL_2; return true; }
    if (token == "r_tarsal_proximal_phalanx_2") { out = HanimSegmentNameValues::R_TARSAL_PROXIMAL_PHALANX_2; return true; }
    if (token == "r_tarsal_middle_phalanx_2") { out = HanimSegmentNameValues::R_TARSAL_MIDDLE_PHALANX_2; return true; }
    if (token == "r_tarsal_distal_phalanx_2") { out = HanimSegmentNameValues::R_TARSAL_DISTAL_PHALANX_2; return true; }
    if (token == "r_cuneiform_3") { out = HanimSegmentNameValues::R_CUNEIFORM_3; return true; }
    if (token == "r_metatarsal_3") { out = HanimSegmentNameValues::R_METATARSAL_3; return true; }
    if (token == "r_tarsal_proximal_phalanx_3") { out = HanimSegmentNameValues::R_TARSAL_PROXIMAL_PHALANX_3; return true; }
    if (token == "r_tarsal_middle_phalanx_3") { out = HanimSegmentNameValues::R_TARSAL_MIDDLE_PHALANX_3; return true; }
    if (token == "r_tarsal_distal_phalanx_3") { out = HanimSegmentNameValues::R_TARSAL_DISTAL_PHALANX_3; return true; }
    if (token == "r_calcaneus") { out = HanimSegmentNameValues::R_CALCANEUS; return true; }
    if (token == "r_cuboid") { out = HanimSegmentNameValues::R_CUBOID; return true; }
    if (token == "r_metatarsal_4") { out = HanimSegmentNameValues::R_METATARSAL_4; return true; }
    if (token == "r_tarsal_proximal_phalanx_4") { out = HanimSegmentNameValues::R_TARSAL_PROXIMAL_PHALANX_4; return true; }
    if (token == "r_tarsal_middle_phalanx_4") { out = HanimSegmentNameValues::R_TARSAL_MIDDLE_PHALANX_4; return true; }
    if (token == "r_tarsal_distal_phalanx_4") { out = HanimSegmentNameValues::R_TARSAL_DISTAL_PHALANX_4; return true; }
    if (token == "r_metatarsal_5") { out = HanimSegmentNameValues::R_METATARSAL_5; return true; }
    if (token == "r_tarsal_proximal_phalanx_5") { out = HanimSegmentNameValues::R_TARSAL_PROXIMAL_PHALANX_5; return true; }
    if (token == "r_tarsal_middle_phalanx_5") { out = HanimSegmentNameValues::R_TARSAL_MIDDLE_PHALANX_5; return true; }
    if (token == "r_tarsal_distal_phalanx_5") { out = HanimSegmentNameValues::R_TARSAL_DISTAL_PHALANX_5; return true; }
    if (token == "l5") { out = HanimSegmentNameValues::L5; return true; }
    if (token == "l4") { out = HanimSegmentNameValues::L4; return true; }
    if (token == "l3") { out = HanimSegmentNameValues::L3; return true; }
    if (token == "l2") { out = HanimSegmentNameValues::L2; return true; }
    if (token == "l1") { out = HanimSegmentNameValues::L1; return true; }
    if (token == "t12") { out = HanimSegmentNameValues::T12; return true; }
    if (token == "t11") { out = HanimSegmentNameValues::T11; return true; }
    if (token == "t10") { out = HanimSegmentNameValues::T10; return true; }
    if (token == "t9") { out = HanimSegmentNameValues::T9; return true; }
    if (token == "t8") { out = HanimSegmentNameValues::T8; return true; }
    if (token == "t7") { out = HanimSegmentNameValues::T7; return true; }
    if (token == "t6") { out = HanimSegmentNameValues::T6; return true; }
    if (token == "t5") { out = HanimSegmentNameValues::T5; return true; }
    if (token == "t4") { out = HanimSegmentNameValues::T4; return true; }
    if (token == "t3") { out = HanimSegmentNameValues::T3; return true; }
    if (token == "t2") { out = HanimSegmentNameValues::T2; return true; }
    if (token == "t1") { out = HanimSegmentNameValues::T1; return true; }
    if (token == "c7") { out = HanimSegmentNameValues::C7; return true; }
    if (token == "c6") { out = HanimSegmentNameValues::C6; return true; }
    if (token == "c5") { out = HanimSegmentNameValues::C5; return true; }
    if (token == "c4") { out = HanimSegmentNameValues::C4; return true; }
    if (token == "c3") { out = HanimSegmentNameValues::C3; return true; }
    if (token == "c2") { out = HanimSegmentNameValues::C2; return true; }
    if (token == "c1") { out = HanimSegmentNameValues::C1; return true; }
    if (token == "skull") { out = HanimSegmentNameValues::SKULL; return true; }
    if (token == "l_eyelid") { out = HanimSegmentNameValues::L_EYELID; return true; }
    if (token == "r_eyelid") { out = HanimSegmentNameValues::R_EYELID; return true; }
    if (token == "l_eyeball") { out = HanimSegmentNameValues::L_EYEBALL; return true; }
    if (token == "r_eyeball") { out = HanimSegmentNameValues::R_EYEBALL; return true; }
    if (token == "l_eyebrow") { out = HanimSegmentNameValues::L_EYEBROW; return true; }
    if (token == "r_eyebrow") { out = HanimSegmentNameValues::R_EYEBROW; return true; }
    if (token == "jaw") { out = HanimSegmentNameValues::JAW; return true; }
    if (token == "l_clavicle") { out = HanimSegmentNameValues::L_CLAVICLE; return true; }
    if (token == "l_scapula") { out = HanimSegmentNameValues::L_SCAPULA; return true; }
    if (token == "l_upperarm") { out = HanimSegmentNameValues::L_UPPERARM; return true; }
    if (token == "l_forearm") { out = HanimSegmentNameValues::L_FOREARM; return true; }
    if (token == "l_carpal") { out = HanimSegmentNameValues::L_CARPAL; return true; }
    if (token == "l_trapezium") { out = HanimSegmentNameValues::L_TRAPEZIUM; return true; }
    if (token == "l_metacarpal_1") { out = HanimSegmentNameValues::L_METACARPAL_1; return true; }
    if (token == "l_carpal_proximal_phalanx_1") { out = HanimSegmentNameValues::L_CARPAL_PROXIMAL_PHALANX_1; return true; }
    if (token == "l_carpal_distal_phalanx_1") { out = HanimSegmentNameValues::L_CARPAL_DISTAL_PHALANX_1; return true; }
    if (token == "l_trapezoid") { out = HanimSegmentNameValues::L_TRAPEZOID; return true; }
    if (token == "l_metacarpal_2") { out = HanimSegmentNameValues::L_METACARPAL_2; return true; }
    if (token == "l_carpal_proximal_phalanx_2") { out = HanimSegmentNameValues::L_CARPAL_PROXIMAL_PHALANX_2; return true; }
    if (token == "l_carpal_middle_phalanx_2") { out = HanimSegmentNameValues::L_CARPAL_MIDDLE_PHALANX_2; return true; }
    if (token == "l_carpal_distal_phalanx_2") { out = HanimSegmentNameValues::L_CARPAL_DISTAL_PHALANX_2; return true; }
    if (token == "l_capitate") { out = HanimSegmentNameValues::L_CAPITATE; return true; }
    if (token == "l_metacarpal_3") { out = HanimSegmentNameValues::L_METACARPAL_3; return true; }
    if (token == "l_carpal_proximal_phalanx_3") { out = HanimSegmentNameValues::L_CARPAL_PROXIMAL_PHALANX_3; return true; }
    if (token == "l_carpal_middle_phalanx_3") { out = HanimSegmentNameValues::L_CARPAL_MIDDLE_PHALANX_3; return true; }
    if (token == "l_carpal_distal_phalanx_3") { out = HanimSegmentNameValues::L_CARPAL_DISTAL_PHALANX_3; return true; }
    if (token == "l_hamate") { out = HanimSegmentNameValues::L_HAMATE; return true; }
    if (token == "l_metacarpal_4") { out = HanimSegmentNameValues::L_METACARPAL_4; return true; }
    if (token == "l_carpal_proximal_phalanx_4") { out = HanimSegmentNameValues::L_CARPAL_PROXIMAL_PHALANX_4; return true; }
    if (token == "l_carpal_middle_phalanx_4") { out = HanimSegmentNameValues::L_CARPAL_MIDDLE_PHALANX_4; return true; }
    if (token == "l_carpal_distal_phalanx_4") { out = HanimSegmentNameValues::L_CARPAL_DISTAL_PHALANX_4; return true; }
    if (token == "l_metacarpal_5") { out = HanimSegmentNameValues::L_METACARPAL_5; return true; }
    if (token == "l_carpal_proximal_phalanx_5") { out = HanimSegmentNameValues::L_CARPAL_PROXIMAL_PHALANX_5; return true; }
    if (token == "l_carpal_middle_phalanx_5") { out = HanimSegmentNameValues::L_CARPAL_MIDDLE_PHALANX_5; return true; }
    if (token == "l_carpal_distal_phalanx_5") { out = HanimSegmentNameValues::L_CARPAL_DISTAL_PHALANX_5; return true; }
    if (token == "r_clavicle") { out = HanimSegmentNameValues::R_CLAVICLE; return true; }
    if (token == "r_scapula") { out = HanimSegmentNameValues::R_SCAPULA; return true; }
    if (token == "r_upperarm") { out = HanimSegmentNameValues::R_UPPERARM; return true; }
    if (token == "r_forearm") { out = HanimSegmentNameValues::R_FOREARM; return true; }
    if (token == "r_carpal") { out = HanimSegmentNameValues::R_CARPAL; return true; }
    if (token == "r_trapezium") { out = HanimSegmentNameValues::R_TRAPEZIUM; return true; }
    if (token == "r_metacarpal_1") { out = HanimSegmentNameValues::R_METACARPAL_1; return true; }
    if (token == "r_carpal_proximal_phalanx_1") { out = HanimSegmentNameValues::R_CARPAL_PROXIMAL_PHALANX_1; return true; }
    if (token == "r_carpal_distal_phalanx_1") { out = HanimSegmentNameValues::R_CARPAL_DISTAL_PHALANX_1; return true; }
    if (token == "r_trapezoid") { out = HanimSegmentNameValues::R_TRAPEZOID; return true; }
    if (token == "r_metacarpal_2") { out = HanimSegmentNameValues::R_METACARPAL_2; return true; }
    if (token == "r_carpal_proximal_phalanx_2") { out = HanimSegmentNameValues::R_CARPAL_PROXIMAL_PHALANX_2; return true; }
    if (token == "r_carpal_middle_phalanx_2") { out = HanimSegmentNameValues::R_CARPAL_MIDDLE_PHALANX_2; return true; }
    if (token == "r_carpal_distal_phalanx_2") { out = HanimSegmentNameValues::R_CARPAL_DISTAL_PHALANX_2; return true; }
    if (token == "r_capitate") { out = HanimSegmentNameValues::R_CAPITATE; return true; }
    if (token == "r_metacarpal_3") { out = HanimSegmentNameValues::R_METACARPAL_3; return true; }
    if (token == "r_carpal_proximal_phalanx_3") { out = HanimSegmentNameValues::R_CARPAL_PROXIMAL_PHALANX_3; return true; }
    if (token == "r_carpal_middle_phalanx_3") { out = HanimSegmentNameValues::R_CARPAL_MIDDLE_PHALANX_3; return true; }
    if (token == "r_carpal_distal_phalanx_3") { out = HanimSegmentNameValues::R_CARPAL_DISTAL_PHALANX_3; return true; }
    if (token == "r_hamate") { out = HanimSegmentNameValues::R_HAMATE; return true; }
    if (token == "r_metacarpal_4") { out = HanimSegmentNameValues::R_METACARPAL_4; return true; }
    if (token == "r_carpal_proximal_phalanx_4") { out = HanimSegmentNameValues::R_CARPAL_PROXIMAL_PHALANX_4; return true; }
    if (token == "r_carpal_middle_phalanx_4") { out = HanimSegmentNameValues::R_CARPAL_MIDDLE_PHALANX_4; return true; }
    if (token == "r_carpal_distal_phalanx_4") { out = HanimSegmentNameValues::R_CARPAL_DISTAL_PHALANX_4; return true; }
    if (token == "r_metacarpal_5") { out = HanimSegmentNameValues::R_METACARPAL_5; return true; }
    if (token == "r_carpal_proximal_phalanx_5") { out = HanimSegmentNameValues::R_CARPAL_PROXIMAL_PHALANX_5; return true; }
    if (token == "r_carpal_middle_phalanx_5") { out = HanimSegmentNameValues::R_CARPAL_MIDDLE_PHALANX_5; return true; }
    if (token == "r_carpal_distal_phalanx_5") { out = HanimSegmentNameValues::R_CARPAL_DISTAL_PHALANX_5; return true; }
    return false;
}

inline std::string to_string(HanimVersionChoices value) {
    switch (value) {
    case HanimVersionChoices::_2_0: return "2.0";
    }
    return "";
}

inline bool from_string(const std::string& token, HanimVersionChoices& out) {
    if (token == "2.0") { out = HanimVersionChoices::_2_0; return true; }
    return false;
}

inline std::string to_string(IntersectionTypeValues value) {
    switch (value) {
    case IntersectionTypeValues::BOUNDS: return "BOUNDS";
    case IntersectionTypeValues::GEOMETRY: return "GEOMETRY";
    }
    return "";
}

inline bool from_string(const std::string& token, IntersectionTypeValues& out) {
    if (token == "BOUNDS") { out = IntersectionTypeValues::BOUNDS; return true; }
    if (token == "GEOMETRY") { out = IntersectionTypeValues::GEOMETRY; return true; }
    return false;
}

inline std::string to_string(JustifyChoices value) {
    switch (value) {
    case JustifyChoices::MIDDLE: return "MIDDLE";
    case JustifyChoices::MIDDLE_BEGIN: return "MIDDLE\" \"BEGIN";
    case JustifyChoices::MIDDLE_END: return "MIDDLE\" \"END";
    case JustifyChoices::MIDDLE_FIRST: return "MIDDLE\" \"FIRST";
    case JustifyChoices::MIDDLE_MIDDLE: return "MIDDLE\" \"MIDDLE";
    case JustifyChoices::BEGIN: return "BEGIN";
    case JustifyChoices::BEGIN_BEGIN: return "BEGIN\" \"BEGIN";
    case JustifyChoices::BEGIN_END: return "BEGIN\" \"END";
    case JustifyChoices::BEGIN_FIRST: return "BEGIN\" \"FIRST";
    case JustifyChoices::BEGIN_MIDDLE: return "BEGIN\" \"MIDDLE";
    case JustifyChoices::END: return "END";
    case JustifyChoices::END_BEGIN: return "END\" \"BEGIN";
    case JustifyChoices::END_END: return "END\" \"END";
    case JustifyChoices::END_FIRST: return "END\" \"FIRST";
    case JustifyChoices::END_MIDDLE: return "END\" \"MIDDLE";
    case JustifyChoices::FIRST: return "FIRST";
    case JustifyChoices::FIRST_BEGIN: return "FIRST\" \"BEGIN";
    case JustifyChoices::FIRST_END: return "FIRST\" \"END";
    case JustifyChoices::FIRST_FIRST: return "FIRST\" \"FIRST";
    case JustifyChoices::FIRST_MIDDLE: return "FIRST\" \"MIDDLE";
    }
    return "";
}

inline bool from_string(const std::string& token, JustifyChoices& out) {
    if (token == "MIDDLE") { out = JustifyChoices::MIDDLE; return true; }
    if (token == "MIDDLE\" \"BEGIN") { out = JustifyChoices::MIDDLE_BEGIN; return true; }
    if (token == "MIDDLE\" \"END") { out = JustifyChoices::MIDDLE_END; return true; }
    if (token == "MIDDLE\" \"FIRST") { out = JustifyChoices::MIDDLE_FIRST; return true; }
    if (token == "MIDDLE\" \"MIDDLE") { out = JustifyChoices::MIDDLE_MIDDLE; return true; }
    if (token == "BEGIN") { out = JustifyChoices::BEGIN; return true; }
    if (token == "BEGIN\" \"BEGIN") { out = JustifyChoices::BEGIN_BEGIN; return true; }
    if (token == "BEGIN\" \"END") { out = JustifyChoices::BEGIN_END; return true; }
    if (token == "BEGIN\" \"FIRST") { out = JustifyChoices::BEGIN_FIRST; return true; }
    if (token == "BEGIN\" \"MIDDLE") { out = JustifyChoices::BEGIN_MIDDLE; return true; }
    if (token == "END") { out = JustifyChoices::END; return true; }
    if (token == "END\" \"BEGIN") { out = JustifyChoices::END_BEGIN; return true; }
    if (token == "END\" \"END") { out = JustifyChoices::END_END; return true; }
    if (token == "END\" \"FIRST") { out = JustifyChoices::END_FIRST; return true; }
    if (token == "END\" \"MIDDLE") { out = JustifyChoices::END_MIDDLE; return true; }
    if (token == "FIRST") { out = JustifyChoices::FIRST; return true; }
    if (token == "FIRST\" \"BEGIN") { out = JustifyChoices::FIRST_BEGIN; return true; }
    if (token == "FIRST\" \"END") { out = JustifyChoices::FIRST_END; return true; }
    if (token == "FIRST\" \"FIRST") { out = JustifyChoices::FIRST_FIRST; return true; }
    if (token == "FIRST\" \"MIDDLE") { out = JustifyChoices::FIRST_MIDDLE; return true; }
    return false;
}

inline std::string to_string(LayoutAlignChoices value) {
    switch (value) {
    case LayoutAlignChoices::LEFT_BOTTOM: return "LEFT\" \"BOTTOM";
    case LayoutAlignChoices::LEFT_CENTER: return "LEFT\" \"CENTER";
    case LayoutAlignChoices::LEFT_TOP: return "LEFT\" \"TOP";
    case LayoutAlignChoices::CENTER_BOTTOM: return "CENTER\" \"BOTTOM";
    case LayoutAlignChoices::CENTER_CENTER: return "CENTER\" \"CENTER";
    case LayoutAlignChoices::CENTER_TOP: return "CENTER\" \"TOP";
    case LayoutAlignChoices::RIGHT_BOTTOM: return "RIGHT\" \"BOTTOM";
    case LayoutAlignChoices::RIGHT_CENTER: return "RIGHT\" \"CENTER";
    case LayoutAlignChoices::RIGHT_TOP: return "RIGHT\" \"TOP";
    }
    return "";
}

inline bool from_string(const std::string& token, LayoutAlignChoices& out) {
    if (token == "LEFT\" \"BOTTOM") { out = LayoutAlignChoices::LEFT_BOTTOM; return true; }
    if (token == "LEFT\" \"CENTER") { out = LayoutAlignChoices::LEFT_CENTER; return true; }
    if (token == "LEFT\" \"TOP") { out = LayoutAlignChoices::LEFT_TOP; return true; }
    if (token == "CENTER\" \"BOTTOM") { out = LayoutAlignChoices::CENTER_BOTTOM; return true; }
    if (token == "CENTER\" \"CENTER") { out = LayoutAlignChoices::CENTER_CENTER; return true; }
    if (token == "CENTER\" \"TOP") { out = LayoutAlignChoices::CENTER_TOP; return true; }
    if (token == "RIGHT\" \"BOTTOM") { out = LayoutAlignChoices::RIGHT_BOTTOM; return true; }
    if (token == "RIGHT\" \"CENTER") { out = LayoutAlignChoices::RIGHT_CENTER; return true; }
    if (token == "RIGHT\" \"TOP") { out = LayoutAlignChoices::RIGHT_TOP; return true; }
    return false;
}

inline std::string to_string(LayoutScaleModeChoices value) {
    switch (value) {
    case LayoutScaleModeChoices::NONE_NONE: return "NONE\" \"NONE";
    case LayoutScaleModeChoices::NONE_FRACTION: return "NONE\" \"FRACTION";
    case LayoutScaleModeChoices::NONE_STRETCH: return "NONE\" \"STRETCH";
    case LayoutScaleModeChoices::NONE_PIXEL: return "NONE\" \"PIXEL";
    case LayoutScaleModeChoices::FRACTION_NONE: return "FRACTION\" \"NONE";
    case LayoutScaleModeChoices::FRACTION_FRACTION: return "FRACTION\" \"FRACTION";
    case LayoutScaleModeChoices::FRACTION_STRETCH: return "FRACTION\" \"STRETCH";
    case LayoutScaleModeChoices::FRACTION_PIXEL: return "FRACTION\" \"PIXEL";
    case LayoutScaleModeChoices::STRETCH_NONE: return "STRETCH\" \"NONE";
    case LayoutScaleModeChoices::STRETCH_FRACTION: return "STRETCH\" \"FRACTION";
    case LayoutScaleModeChoices::STRETCH_STRETCH: return "STRETCH\" \"STRETCH";
    case LayoutScaleModeChoices::STRETCH_PIXEL: return "STRETCH\" \"PIXEL";
    case LayoutScaleModeChoices::PIXEL_NONE: return "PIXEL\" \"NONE";
    case LayoutScaleModeChoices::PIXEL_FRACTION: return "PIXEL\" \"FRACTION";
    case LayoutScaleModeChoices::PIXEL_STRETCH: return "PIXEL\" \"STRETCH";
    case LayoutScaleModeChoices::PIXEL_PIXEL: return "PIXEL\" \"PIXEL";
    }
    return "";
}

inline bool from_string(const std::string& token, LayoutScaleModeChoices& out) {
    if (token == "NONE\" \"NONE") { out = LayoutScaleModeChoices::NONE_NONE; return true; }
    if (token == "NONE\" \"FRACTION") { out = LayoutScaleModeChoices::NONE_FRACTION; return true; }
    if (token == "NONE\" \"STRETCH") { out = LayoutScaleModeChoices::NONE_STRETCH; return true; }
    if (token == "NONE\" \"PIXEL") { out = LayoutScaleModeChoices::NONE_PIXEL; return true; }
    if (token == "FRACTION\" \"NONE") { out = LayoutScaleModeChoices::FRACTION_NONE; return true; }
    if (token == "FRACTION\" \"FRACTION") { out = LayoutScaleModeChoices::FRACTION_FRACTION; return true; }
    if (token == "FRACTION\" \"STRETCH") { out = LayoutScaleModeChoices::FRACTION_STRETCH; return true; }
    if (token == "FRACTION\" \"PIXEL") { out = LayoutScaleModeChoices::FRACTION_PIXEL; return true; }
    if (token == "STRETCH\" \"NONE") { out = LayoutScaleModeChoices::STRETCH_NONE; return true; }
    if (token == "STRETCH\" \"FRACTION") { out = LayoutScaleModeChoices::STRETCH_FRACTION; return true; }
    if (token == "STRETCH\" \"STRETCH") { out = LayoutScaleModeChoices::STRETCH_STRETCH; return true; }
    if (token == "STRETCH\" \"PIXEL") { out = LayoutScaleModeChoices::STRETCH_PIXEL; return true; }
    if (token == "PIXEL\" \"NONE") { out = LayoutScaleModeChoices::PIXEL_NONE; return true; }
    if (token == "PIXEL\" \"FRACTION") { out = LayoutScaleModeChoices::PIXEL_FRACTION; return true; }
    if (token == "PIXEL\" \"STRETCH") { out = LayoutScaleModeChoices::PIXEL_STRETCH; return true; }
    if (token == "PIXEL\" \"PIXEL") { out = LayoutScaleModeChoices::PIXEL_PIXEL; return true; }
    return false;
}

inline std::string to_string(LayoutUnitsChoices value) {
    switch (value) {
    case LayoutUnitsChoices::WORLD_WORLD: return "WORLD\" \"WORLD";
    case LayoutUnitsChoices::WORLD_FRACTION: return "WORLD\" \"FRACTION";
    case LayoutUnitsChoices::WORLD_PIXEL: return "WORLD\" \"PIXEL";
    case LayoutUnitsChoices::FRACTION_WORLD: return "FRACTION\" \"WORLD";
    case LayoutUnitsChoices::FRACTION_FRACTION: return "FRACTION\" \"FRACTION";
    case LayoutUnitsChoices::FRACTION_PIXEL: return "FRACTION\" \"PIXEL";
    case LayoutUnitsChoices::PIXEL_WORLD: return "PIXEL\" \"WORLD";
    case LayoutUnitsChoices::PIXEL_FRACTION: return "PIXEL\" \"FRACTION";
    case LayoutUnitsChoices::PIXEL_PIXEL: return "PIXEL\" \"PIXEL";
    }
    return "";
}

inline bool from_string(const std::string& token, LayoutUnitsChoices& out) {
    if (token == "WORLD\" \"WORLD") { out = LayoutUnitsChoices::WORLD_WORLD; return true; }
    if (token == "WORLD\" \"FRACTION") { out = LayoutUnitsChoices::WORLD_FRACTION; return true; }
    if (token == "WORLD\" \"PIXEL") { out = LayoutUnitsChoices::WORLD_PIXEL; return true; }
    if (token == "FRACTION\" \"WORLD") { out = LayoutUnitsChoices::FRACTION_WORLD; return true; }
    if (token == "FRACTION\" \"FRACTION") { out = LayoutUnitsChoices::FRACTION_FRACTION; return true; }
    if (token == "FRACTION\" \"PIXEL") { out = LayoutUnitsChoices::FRACTION_PIXEL; return true; }
    if (token == "PIXEL\" \"WORLD") { out = LayoutUnitsChoices::PIXEL_WORLD; return true; }
    if (token == "PIXEL\" \"FRACTION") { out = LayoutUnitsChoices::PIXEL_FRACTION; return true; }
    if (token == "PIXEL\" \"PIXEL") { out = LayoutUnitsChoices::PIXEL_PIXEL; return true; }
    return false;
}

inline std::string to_string(MetaDirectionChoices value) {
    switch (value) {
    case MetaDirectionChoices::RTL: return "rtl";
    case MetaDirectionChoices::LTR: return "ltr";
    }
    return "";
}

inline bool from_string(const std::string& token, MetaDirectionChoices& out) {
    if (token == "rtl") { out = MetaDirectionChoices::RTL; return true; }
    if (token == "ltr") { out = MetaDirectionChoices::LTR; return true; }
    return false;
}

inline std::string to_string(MetaNameValues value) {
    switch (value) {
    case MetaNameValues::ACCESSRIGHTS: return "accessRights";
    case MetaNameValues::AUTHOR: return "author";
    case MetaNameValues::CML_VERSION: return "CML-version";
    case MetaNameValues::CONTRIBUTOR: return "contributor";
    case MetaNameValues::CREATED: return "created";
    case MetaNameValues::CREATOR: return "creator";
    case MetaNameValues::DESCRIPTION: return "description";
    case MetaNameValues::DISCLAIMER: return "disclaimer";
    case MetaNameValues::DRAWING: return "drawing";
    case MetaNameValues::ERROR: return "error";
    case MetaNameValues::GENERATOR: return "generator";
    case MetaNameValues::HINT: return "hint";
    case MetaNameValues::IDENTIFIER: return "identifier";
    case MetaNameValues::IMAGE: return "Image";
    case MetaNameValues::INFO: return "info";
    case MetaNameValues::INFORMATION: return "information";
    case MetaNameValues::ISVERSIONOF: return "isVersionOf";
    case MetaNameValues::KEYWORDS: return "keywords";
    case MetaNameValues::LICENSE: return "license";
    case MetaNameValues::MEDIATOR: return "mediator";
    case MetaNameValues::MODIFIED: return "modified";
    case MetaNameValues::MOVIE: return "movie";
    case MetaNameValues::MOVINGIMAGE: return "MovingImage";
    case MetaNameValues::ORIGINAL: return "original";
    case MetaNameValues::PHOTO: return "photo";
    case MetaNameValues::PHOTOGRAPH: return "photograph";
    case MetaNameValues::PUBLISHER: return "publisher";
    case MetaNameValues::REFERENCE: return "reference";
    case MetaNameValues::REQUIRES: return "requires";
    case MetaNameValues::RIGHTS: return "rights";
    case MetaNameValues::ROBOTS: return "robots";
    case MetaNameValues::SOUND: return "Sound";
    case MetaNameValues::SOURCE: return "source";
    case MetaNameValues::SPECIFICATIONSECTION: return "specificationSection";
    case MetaNameValues::SPECIFICATIONURL: return "specificationUrl";
    case MetaNameValues::SUBJECT: return "subject";
    case MetaNameValues::TEXT: return "Text";
    case MetaNameValues::TITLE: return "title";
    case MetaNameValues::TODO: return "TODO";
    case MetaNameValues::TRANSLATOR: return "translator";
    case MetaNameValues::TRANSLATED: return "translated";
    case MetaNameValues::VERSION: return "version";
    case MetaNameValues::WARNING: return "warning";
    }
    return "";
}

inline bool from_string(const std::string& token, MetaNameValues& out) {
    if (token == "accessRights") { out = MetaNameValues::ACCESSRIGHTS; return true; }
    if (token == "author") { out = MetaNameValues::AUTHOR; return true; }
    if (token == "CML-version") { out = MetaNameValues::CML_VERSION; return true; }
    if (token == "contributor") { out = MetaNameValues::CONTRIBUTOR; return true; }
    if (token == "created") { out = MetaNameValues::CREATED; return true; }
    if (token == "creator") { out = MetaNameValues::CREATOR; return true; }
    if (token == "description") { out = MetaNameValues::DESCRIPTION; return true; }
    if (token == "disclaimer") { out = MetaNameValues::DISCLAIMER; return true; }
    if (token == "drawing") { out = MetaNameValues::DRAWING; return true; }
    if (token == "error") { out = MetaNameValues::ERROR; return true; }
    if (token == "generator") { out = MetaNameValues::GENERATOR; return true; }
    if (token == "hint") { out = MetaNameValues::HINT; return true; }
    if (token == "identifier") { out = MetaNameValues::IDENTIFIER; return true; }
    if (token == "Image") { out = MetaNameValues::IMAGE; return true; }
    if (token == "info") { out = MetaNameValues::INFO; return true; }
    if (token == "information") { out = MetaNameValues::INFORMATION; return true; }
    if (token == "isVersionOf") { out = MetaNameValues::ISVERSIONOF; return true; }
    if (token == "keywords") { out = MetaNameValues::KEYWORDS; return true; }
    if (token == "license") { out = MetaNameValues::LICENSE; return true; }
    if (token == "mediator") { out = MetaNameValues::MEDIATOR; return true; }
    if (token == "modified") { out = MetaNameValues::MODIFIED; return true; }
    if (token == "movie") { out = MetaNameValues::MOVIE; return true; }
    if (token == "MovingImage") { out = MetaNameValues::MOVINGIMAGE; return true; }
    if (token == "original") { out = MetaNameValues::ORIGINAL; return true; }
    if (token == "photo") { out = MetaNameValues::PHOTO; return true; }
    if (token == "photograph") { out = MetaNameValues::PHOTOGRAPH; return true; }
    if (token == "publisher") { out = MetaNameValues::PUBLISHER; return true; }
    if (token == "reference") { out = MetaNameValues::REFERENCE; return true; }
    if (token == "requires") { out = MetaNameValues::REQUIRES; return true; }
    if (token == "rights") { out = MetaNameValues::RIGHTS; return true; }
    if (token == "robots") { out = MetaNameValues::ROBOTS; return true; }
    if (token == "Sound") { out = MetaNameValues::SOUND; return true; }
    if (token == "source") { out = MetaNameValues::SOURCE; return true; }
    if (token == "specificationSection") { out = MetaNameValues::SPECIFICATIONSECTION; return true; }
    if (token == "specificationUrl") { out = MetaNameValues::SPECIFICATIONURL; return true; }
    if (token == "subject") { out = MetaNameValues::SUBJECT; return true; }
    if (token == "Text") { out = MetaNameValues::TEXT; return true; }
    if (token == "title") { out = MetaNameValues::TITLE; return true; }
    if (token == "TODO") { out = MetaNameValues::TODO; return true; }
    if (token == "translator") { out = MetaNameValues::TRANSLATOR; return true; }
    if (token == "translated") { out = MetaNameValues::TRANSLATED; return true; }
    if (token == "version") { out = MetaNameValues::VERSION; return true; }
    if (token == "warning") { out = MetaNameValues::WARNING; return true; }
    return false;
}

inline std::string to_string(NavigationTransitionTypeValues value) {
    switch (value) {
    case NavigationTransitionTypeValues::TELEPORT: return "TELEPORT";
    case NavigationTransitionTypeValues::LINEAR: return "LINEAR";
    case NavigationTransitionTypeValues::ANIMATE: return "ANIMATE";
    }
    return "";
}

inline bool from_string(const std::string& token, NavigationTransitionTypeValues& out) {
    if (token == "TELEPORT") { out = NavigationTransitionTypeValues::TELEPORT; return true; }
    if (token == "LINEAR") { out = NavigationTransitionTypeValues::LINEAR; return true; }
    if (token == "ANIMATE") { out = NavigationTransitionTypeValues::ANIMATE; return true; }
    return false;
}

inline std::string to_string(NavigationTypeValues value) {
    switch (value) {
    case NavigationTypeValues::ANY: return "ANY";
    case NavigationTypeValues::WALK: return "WALK";
    case NavigationTypeValues::EXAMINE: return "EXAMINE";
    case NavigationTypeValues::FLY: return "FLY";
    case NavigationTypeValues::LOOKAT: return "LOOKAT";
    case NavigationTypeValues::NONE: return "NONE";
    case NavigationTypeValues::EXPLORE: return "EXPLORE";
    }
    return "";
}

inline bool from_string(const std::string& token, NavigationTypeValues& out) {
    if (token == "ANY") { out = NavigationTypeValues::ANY; return true; }
    if (token == "WALK") { out = NavigationTypeValues::WALK; return true; }
    if (token == "EXAMINE") { out = NavigationTypeValues::EXAMINE; return true; }
    if (token == "FLY") { out = NavigationTypeValues::FLY; return true; }
    if (token == "LOOKAT") { out = NavigationTypeValues::LOOKAT; return true; }
    if (token == "NONE") { out = NavigationTypeValues::NONE; return true; }
    if (token == "EXPLORE") { out = NavigationTypeValues::EXPLORE; return true; }
    return false;
}

inline std::string to_string(NetworkModeChoices value) {
    switch (value) {
    case NetworkModeChoices::STANDALONE: return "standAlone";
    case NetworkModeChoices::NETWORKREADER: return "networkReader";
    case NetworkModeChoices::NETWORKWRITER: return "networkWriter";
    }
    return "";
}

inline bool from_string(const std::string& token, NetworkModeChoices& out) {
    if (token == "standAlone") { out = NetworkModeChoices::STANDALONE; return true; }
    if (token == "networkReader") { out = NetworkModeChoices::NETWORKREADER; return true; }
    if (token == "networkWriter") { out = NetworkModeChoices::NETWORKWRITER; return true; }
    return false;
}

inline std::string to_string(ParticleSystemGeometryTypeValues value) {
    switch (value) {
    case ParticleSystemGeometryTypeValues::LINE: return "LINE";
    case ParticleSystemGeometryTypeValues::POINT: return "POINT";
    case ParticleSystemGeometryTypeValues::QUAD: return "QUAD";
    case ParticleSystemGeometryTypeValues::SPRITE: return "SPRITE";
    case ParticleSystemGeometryTypeValues::TRIANGLE: return "TRIANGLE";
    case ParticleSystemGeometryTypeValues::GEOMETRY: return "GEOMETRY";
    }
    return "";
}

inline bool from_string(const std::string& token, ParticleSystemGeometryTypeValues& out) {
    if (token == "LINE") { out = ParticleSystemGeometryTypeValues::LINE; return true; }
    if (token == "POINT") { out = ParticleSystemGeometryTypeValues::POINT; return true; }
    if (token == "QUAD") { out = ParticleSystemGeometryTypeValues::QUAD; return true; }
    if (token == "SPRITE") { out = ParticleSystemGeometryTypeValues::SPRITE; return true; }
    if (token == "TRIANGLE") { out = ParticleSystemGeometryTypeValues::TRIANGLE; return true; }
    if (token == "GEOMETRY") { out = ParticleSystemGeometryTypeValues::GEOMETRY; return true; }
    return false;
}

inline std::string to_string(PeriodicWaveTypeChoices value) {
    switch (value) {
    case PeriodicWaveTypeChoices::SINE: return "SINE";
    case PeriodicWaveTypeChoices::SQUARE: return "SQUARE";
    case PeriodicWaveTypeChoices::SAWTOOTH: return "SAWTOOTH";
    case PeriodicWaveTypeChoices::TRIANGLE: return "TRIANGLE";
    case PeriodicWaveTypeChoices::CUSTOM: return "CUSTOM";
    }
    return "";
}

inline bool from_string(const std::string& token, PeriodicWaveTypeChoices& out) {
    if (token == "SINE") { out = PeriodicWaveTypeChoices::SINE; return true; }
    if (token == "SQUARE") { out = PeriodicWaveTypeChoices::SQUARE; return true; }
    if (token == "SAWTOOTH") { out = PeriodicWaveTypeChoices::SAWTOOTH; return true; }
    if (token == "TRIANGLE") { out = PeriodicWaveTypeChoices::TRIANGLE; return true; }
    if (token == "CUSTOM") { out = PeriodicWaveTypeChoices::CUSTOM; return true; }
    return false;
}

inline std::string to_string(PhaseFunctionValues value) {
    switch (value) {
    case PhaseFunctionValues::HENYEY_GREENSTEIN: return "Henyey-Greenstein";
    case PhaseFunctionValues::NONE: return "NONE";
    }
    return "";
}

inline bool from_string(const std::string& token, PhaseFunctionValues& out) {
    if (token == "Henyey-Greenstein") { out = PhaseFunctionValues::HENYEY_GREENSTEIN; return true; }
    if (token == "NONE") { out = PhaseFunctionValues::NONE; return true; }
    return false;
}

inline std::string to_string(PickSensorMatchCriterionChoices value) {
    switch (value) {
    case PickSensorMatchCriterionChoices::MATCH_ANY: return "MATCH_ANY";
    case PickSensorMatchCriterionChoices::MATCH_EVERY: return "MATCH_EVERY";
    case PickSensorMatchCriterionChoices::MATCH_ONLY_ONE: return "MATCH_ONLY_ONE";
    }
    return "";
}

inline bool from_string(const std::string& token, PickSensorMatchCriterionChoices& out) {
    if (token == "MATCH_ANY") { out = PickSensorMatchCriterionChoices::MATCH_ANY; return true; }
    if (token == "MATCH_EVERY") { out = PickSensorMatchCriterionChoices::MATCH_EVERY; return true; }
    if (token == "MATCH_ONLY_ONE") { out = PickSensorMatchCriterionChoices::MATCH_ONLY_ONE; return true; }
    return false;
}

inline std::string to_string(PickSensorSortOrderValues value) {
    switch (value) {
    case PickSensorSortOrderValues::ANY: return "ANY";
    case PickSensorSortOrderValues::CLOSEST: return "CLOSEST";
    case PickSensorSortOrderValues::ALL: return "ALL";
    case PickSensorSortOrderValues::ALL_SORTED: return "ALL_SORTED";
    }
    return "";
}

inline bool from_string(const std::string& token, PickSensorSortOrderValues& out) {
    if (token == "ANY") { out = PickSensorSortOrderValues::ANY; return true; }
    if (token == "CLOSEST") { out = PickSensorSortOrderValues::CLOSEST; return true; }
    if (token == "ALL") { out = PickSensorSortOrderValues::ALL; return true; }
    if (token == "ALL_SORTED") { out = PickSensorSortOrderValues::ALL_SORTED; return true; }
    return false;
}

inline std::string to_string(PickableObjectTypeValues value) {
    switch (value) {
    case PickableObjectTypeValues::ALL: return "ALL";
    case PickableObjectTypeValues::NONE: return "NONE";
    case PickableObjectTypeValues::TERRAIN: return "TERRAIN";
    }
    return "";
}

inline bool from_string(const std::string& token, PickableObjectTypeValues& out) {
    if (token == "ALL") { out = PickableObjectTypeValues::ALL; return true; }
    if (token == "NONE") { out = PickableObjectTypeValues::NONE; return true; }
    if (token == "TERRAIN") { out = PickableObjectTypeValues::TERRAIN; return true; }
    return false;
}

inline std::string to_string(ProfileNameChoices value) {
    switch (value) {
    case ProfileNameChoices::CORE: return "Core";
    case ProfileNameChoices::INTERCHANGE: return "Interchange";
    case ProfileNameChoices::CADINTERCHANGE: return "CADInterchange";
    case ProfileNameChoices::INTERACTIVE: return "Interactive";
    case ProfileNameChoices::IMMERSIVE: return "Immersive";
    case ProfileNameChoices::MEDICALINTERCHANGE: return "MedicalInterchange";
    case ProfileNameChoices::MPEG4INTERACTIVE: return "MPEG4Interactive";
    case ProfileNameChoices::FULL: return "Full";
    }
    return "";
}

inline bool from_string(const std::string& token, ProfileNameChoices& out) {
    if (token == "Core") { out = ProfileNameChoices::CORE; return true; }
    if (token == "Interchange") { out = ProfileNameChoices::INTERCHANGE; return true; }
    if (token == "CADInterchange") { out = ProfileNameChoices::CADINTERCHANGE; return true; }
    if (token == "Interactive") { out = ProfileNameChoices::INTERACTIVE; return true; }
    if (token == "Immersive") { out = ProfileNameChoices::IMMERSIVE; return true; }
    if (token == "MedicalInterchange") { out = ProfileNameChoices::MEDICALINTERCHANGE; return true; }
    if (token == "MPEG4Interactive") { out = ProfileNameChoices::MPEG4INTERACTIVE; return true; }
    if (token == "Full") { out = ProfileNameChoices::FULL; return true; }
    return false;
}

inline std::string to_string(ProjectionVolumeStyleTypeChoices value) {
    switch (value) {
    case ProjectionVolumeStyleTypeChoices::MAX: return "MAX";
    case ProjectionVolumeStyleTypeChoices::MIN: return "MIN";
    case ProjectionVolumeStyleTypeChoices::AVERAGE: return "AVERAGE";
    }
    return "";
}

inline bool from_string(const std::string& token, ProjectionVolumeStyleTypeChoices& out) {
    if (token == "MAX") { out = ProjectionVolumeStyleTypeChoices::MAX; return true; }
    if (token == "MIN") { out = ProjectionVolumeStyleTypeChoices::MIN; return true; }
    if (token == "AVERAGE") { out = ProjectionVolumeStyleTypeChoices::AVERAGE; return true; }
    return false;
}

inline std::string to_string(ShaderLanguageValues value) {
    switch (value) {
    case ShaderLanguageValues::CG: return "Cg";
    case ShaderLanguageValues::GLSL: return "GLSL";
    case ShaderLanguageValues::HLSL: return "HLSL";
    }
    return "";
}

inline bool from_string(const std::string& token, ShaderLanguageValues& out) {
    if (token == "Cg") { out = ShaderLanguageValues::CG; return true; }
    if (token == "GLSL") { out = ShaderLanguageValues::GLSL; return true; }
    if (token == "HLSL") { out = ShaderLanguageValues::HLSL; return true; }
    return false;
}

inline std::string to_string(ShaderPartTypeValues value) {
    switch (value) {
    case ShaderPartTypeValues::VERTEX: return "VERTEX";
    case ShaderPartTypeValues::FRAGMENT: return "FRAGMENT";
    }
    return "";
}

inline bool from_string(const std::string& token, ShaderPartTypeValues& out) {
    if (token == "VERTEX") { out = ShaderPartTypeValues::VERTEX; return true; }
    if (token == "FRAGMENT") { out = ShaderPartTypeValues::FRAGMENT; return true; }
    return false;
}

inline std::string to_string(TextureBoundaryModeChoices value) {
    switch (value) {
    case TextureBoundaryModeChoices::CLAMP: return "CLAMP";
    case TextureBoundaryModeChoices::CLAMP_TO_EDGE: return "CLAMP_TO_EDGE";
    case TextureBoundaryModeChoices::CLAMP_TO_BOUNDARY: return "CLAMP_TO_BOUNDARY";
    case TextureBoundaryModeChoices::MIRRORED_REPEAT: return "MIRRORED_REPEAT";
    case TextureBoundaryModeChoices::REPEAT: return "REPEAT";
    }
    return "";
}

inline bool from_string(const std::string& token, TextureBoundaryModeChoices& out) {
    if (token == "CLAMP") { out = TextureBoundaryModeChoices::CLAMP; return true; }
    if (token == "CLAMP_TO_EDGE") { out = TextureBoundaryModeChoices::CLAMP_TO_EDGE; return true; }
    if (token == "CLAMP_TO_BOUNDARY") { out = TextureBoundaryModeChoices::CLAMP_TO_BOUNDARY; return true; }
    if (token == "MIRRORED_REPEAT") { out = TextureBoundaryModeChoices::MIRRORED_REPEAT; return true; }
    if (token == "REPEAT") { out = TextureBoundaryModeChoices::REPEAT; return true; }
    return false;
}

inline std::string to_string(TextureCompressionModeChoices value) {
    switch (value) {
    case TextureCompressionModeChoices::DEFAULT: return "DEFAULT";
    case TextureCompressionModeChoices::FASTEST: return "FASTEST";
    case TextureCompressionModeChoices::HIGH: return "HIGH";
    case TextureCompressionModeChoices::LOW: return "LOW";
    case TextureCompressionModeChoices::MEDIUM: return "MEDIUM";
    case TextureCompressionModeChoices::NICEST: return "NICEST";
    }
    return "";
}

inline bool from_string(const std::string& token, TextureCompressionModeChoices& out) {
    if (token == "DEFAULT") { out = TextureCompressionModeChoices::DEFAULT; return true; }
    if (token == "FASTEST") { out = TextureCompressionModeChoices::FASTEST; return true; }
    if (token == "HIGH") { out = TextureCompressionModeChoices::HIGH; return true; }
    if (token == "LOW") { out = TextureCompressionModeChoices::LOW; return true; }
    if (token == "MEDIUM") { out = TextureCompressionModeChoices::MEDIUM; return true; }
    if (token == "NICEST") { out = TextureCompressionModeChoices::NICEST; return true; }
    return false;
}

inline std::string to_string(TextureCoordinateGeneratorModeChoices value) {
    switch (value) {
    case TextureCoordinateGeneratorModeChoices::SPHERE: return "SPHERE";
    case TextureCoordinateGeneratorModeChoices::CAMERASPACENORMAL: return "CAMERASPACENORMAL";
    case TextureCoordinateGeneratorModeChoices::CAMERASPACEPOSITION: return "CAMERASPACEPOSITION";
    case TextureCoordinateGeneratorModeChoices::CAMERASPACEREFLECTIONVECTOR: return "CAMERASPACEREFLECTIONVECTOR";
    case TextureCoordinateGeneratorModeChoices::SPHERE_LOCAL: return "SPHERE-LOCAL";
    case TextureCoordinateGeneratorModeChoices::COORD: return "COORD";
    case TextureCoordinateGeneratorModeChoices::COORD_EYE: return "COORD-EYE";
    case TextureCoordinateGeneratorModeChoices::NOISE: return "NOISE";
    case TextureCoordinateGeneratorModeChoices::NOISE_EYE: return "NOISE-EYE";
    case TextureCoordinateGeneratorModeChoices::SPHERE_REFLECT: return "SPHERE-REFLECT";
    case TextureCoordinateGeneratorModeChoices::SPHERE_REFLECT_LOCAL: return "SPHERE-REFLECT-LOCAL";
    }
    return "";
}

inline bool from_string(const std::string& token, TextureCoordinateGeneratorModeChoices& out) {
    if (token == "SPHERE") { out = TextureCoordinateGeneratorModeChoices::SPHERE; return true; }
    if (token == "CAMERASPACENORMAL") { out = TextureCoordinateGeneratorModeChoices::CAMERASPACENORMAL; return true; }
    if (token == "CAMERASPACEPOSITION") { out = TextureCoordinateGeneratorModeChoices::CAMERASPACEPOSITION; return true; }
    if (token == "CAMERASPACEREFLECTIONVECTOR") { out = TextureCoordinateGeneratorModeChoices::CAMERASPACEREFLECTIONVECTOR; return true; }
    if (token == "SPHERE-LOCAL") { out = TextureCoordinateGeneratorModeChoices::SPHERE_LOCAL; return true; }
    if (token == "COORD") { out = TextureCoordinateGeneratorModeChoices::COORD; return true; }
    if (token == "COORD-EYE") { out = TextureCoordinateGeneratorModeChoices::COORD_EYE; return true; }
    if (token == "NOISE") { out = TextureCoordinateGeneratorModeChoices::NOISE; return true; }
    if (token == "NOISE-EYE") { out = TextureCoordinateGeneratorModeChoices::NOISE_EYE; return true; }
    if (token == "SPHERE-REFLECT") { out = TextureCoordinateGeneratorModeChoices::SPHERE_REFLECT; return true; }
    if (token == "SPHERE-REFLECT-LOCAL") { out = TextureCoordinateGeneratorModeChoices::SPHERE_REFLECT_LOCAL; return true; }
    return false;
}

inline std::string to_string(TextureMagnificationModeChoices value) {
    switch (value) {
    case TextureMagnificationModeChoices::AVG_PIXEL: return "AVG_PIXEL";
    case TextureMagnificationModeChoices::DEFAULT: return "DEFAULT";
    case TextureMagnificationModeChoices::FASTEST: return "FASTEST";
    case TextureMagnificationModeChoices::NEAREST_PIXEL: return "NEAREST_PIXEL";
    case TextureMagnificationModeChoices::NICEST: return "NICEST";
    }
    return "";
}

inline bool from_string(const std::string& token, TextureMagnificationModeChoices& out) {
    if (token == "AVG_PIXEL") { out = TextureMagnificationModeChoices::AVG_PIXEL; return true; }
    if (token == "DEFAULT") { out = TextureMagnificationModeChoices::DEFAULT; return true; }
    if (token == "FASTEST") { out = TextureMagnificationModeChoices::FASTEST; return true; }
    if (token == "NEAREST_PIXEL") { out = TextureMagnificationModeChoices::NEAREST_PIXEL; return true; }
    if (token == "NICEST") { out = TextureMagnificationModeChoices::NICEST; return true; }
    return false;
}

inline std::string to_string(TextureMinificationModeChoices value) {
    switch (value) {
    case TextureMinificationModeChoices::AVG_PIXEL: return "AVG_PIXEL";
    case TextureMinificationModeChoices::AVG_PIXEL_AVG_MIPMAP: return "AVG_PIXEL_AVG_MIPMAP";
    case TextureMinificationModeChoices::AVG_PIXEL_NEAREST_MIPMAP: return "AVG_PIXEL_NEAREST_MIPMAP";
    case TextureMinificationModeChoices::DEFAULT: return "DEFAULT";
    case TextureMinificationModeChoices::FASTEST: return "FASTEST";
    case TextureMinificationModeChoices::NEAREST_PIXEL: return "NEAREST_PIXEL";
    case TextureMinificationModeChoices::NEAREST_PIXEL_AVG_MIPMAP: return "NEAREST_PIXEL_AVG_MIPMAP";
    case TextureMinificationModeChoices::NEAREST_PIXEL_NEAREST_MIPMAP: return "NEAREST_PIXEL_NEAREST_MIPMAP";
    case TextureMinificationModeChoices::NICEST: return "NICEST";
    }
    return "";
}

inline bool from_string(const std::string& token, TextureMinificationModeChoices& out) {
    if (token == "AVG_PIXEL") { out = TextureMinificationModeChoices::AVG_PIXEL; return true; }
    if (token == "AVG_PIXEL_AVG_MIPMAP") { out = TextureMinificationModeChoices::AVG_PIXEL_AVG_MIPMAP; return true; }
    if (token == "AVG_PIXEL_NEAREST_MIPMAP") { out = TextureMinificationModeChoices::AVG_PIXEL_NEAREST_MIPMAP; return true; }
    if (token == "DEFAULT") { out = TextureMinificationModeChoices::DEFAULT; return true; }
    if (token == "FASTEST") { out = TextureMinificationModeChoices::FASTEST; return true; }
    if (token == "NEAREST_PIXEL") { out = TextureMinificationModeChoices::NEAREST_PIXEL; return true; }
    if (token == "NEAREST_PIXEL_AVG_MIPMAP") { out = TextureMinificationModeChoices::NEAREST_PIXEL_AVG_MIPMAP; return true; }
    if (token == "NEAREST_PIXEL_NEAREST_MIPMAP") { out = TextureMinificationModeChoices::NEAREST_PIXEL_NEAREST_MIPMAP; return true; }
    if (token == "NICEST") { out = TextureMinificationModeChoices::NICEST; return true; }
    return false;
}

inline std::string to_string(UnitCategoryChoices value) {
    switch (value) {
    case UnitCategoryChoices::ANGLE: return "angle";
    case UnitCategoryChoices::FORCE: return "force";
    case UnitCategoryChoices::LENGTH: return "length";
    case UnitCategoryChoices::MASS: return "mass";
    }
    return "";
}

inline bool from_string(const std::string& token, UnitCategoryChoices& out) {
    if (token == "angle") { out = UnitCategoryChoices::ANGLE; return true; }
    if (token == "force") { out = UnitCategoryChoices::FORCE; return true; }
    if (token == "length") { out = UnitCategoryChoices::LENGTH; return true; }
    if (token == "mass") { out = UnitCategoryChoices::MASS; return true; }
    return false;
}

inline std::string to_string(WaveShaperOversampleChoices value) {
    switch (value) {
    case WaveShaperOversampleChoices::NONE: return "NONE";
    case WaveShaperOversampleChoices::_2X: return "2X";
    case WaveShaperOversampleChoices::_4X: return "4X";
    }
    return "";
}

inline bool from_string(const std::string& token, WaveShaperOversampleChoices& out) {
    if (token == "NONE") { out = WaveShaperOversampleChoices::NONE; return true; }
    if (token == "2X") { out = WaveShaperOversampleChoices::_2X; return true; }
    if (token == "4X") { out = WaveShaperOversampleChoices::_4X; return true; }
    return false;
}

inline std::string to_string(X3dVersionChoices value) {
    switch (value) {
    case X3dVersionChoices::_3_0: return "3.0";
    case X3dVersionChoices::_3_1: return "3.1";
    case X3dVersionChoices::_3_2: return "3.2";
    case X3dVersionChoices::_3_3: return "3.3";
    case X3dVersionChoices::_4_0: return "4.0";
    }
    return "";
}

inline bool from_string(const std::string& token, X3dVersionChoices& out) {
    if (token == "3.0") { out = X3dVersionChoices::_3_0; return true; }
    if (token == "3.1") { out = X3dVersionChoices::_3_1; return true; }
    if (token == "3.2") { out = X3dVersionChoices::_3_2; return true; }
    if (token == "3.3") { out = X3dVersionChoices::_3_3; return true; }
    if (token == "4.0") { out = X3dVersionChoices::_4_0; return true; }
    return false;
}

#endif // X3D_ENUMS_HPP
