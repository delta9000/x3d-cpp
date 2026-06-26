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

// FontStyleChoices: fontStyleChoices are strictly allowed enumeration values for FontStyle/ScreenFontStyle node style field. This list is bounded, no additional values are allowed.
enum class FontStyleChoices {
    PLAIN,  // "PLAIN"
    BOLD,  // "BOLD"
    ITALIC,  // "ITALIC"
    BOLDITALIC,  // "BOLDITALIC"
};

// GeneratedCubeMapTextureUpdateChoices: generatedCubeMapTextureUpdateChoices are strictly allowed enumeration values for GeneratedCubeMapTexture field named 'update'. This list is bounded, no additional values are allowed.
enum class GeneratedCubeMapTextureUpdateChoices {
    NONE,  // "NONE"
    NEXT_FRAME_ONLY,  // "NEXT_FRAME_ONLY"
    ALWAYS,  // "ALWAYS"
};

// HanimVersionChoices: hanimVersionChoices enumeration constants are used to identify the allowed versions for an HAnimHumanoid node. Note that default HAnimHumanoid version is 2.0 for X3D version 4, and default is 1.0 for X3D version 3. This list is bounded, no additional values are allowed.
enum class HanimVersionChoices {
    _2_0,  // "2.0"
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

// NetworkModeChoices: networkModeChoices are strictly allowed enumeration values for DIS field networkMode. This list is bounded, no additional values are allowed.
enum class NetworkModeChoices {
    STANDALONE,  // "standAlone"
    NETWORKREADER,  // "networkReader"
    NETWORKWRITER,  // "networkWriter"
};

// PeriodicWaveTypeChoices: Permitted values for PeriodicWave type. X3D enumeration naming conventions are capitalized versions of Web Audio API enumerations, also changing hyphens to underscores. This list is bounded, no additional values are allowed.
enum class PeriodicWaveTypeChoices {
    SINE,  // "SINE"
    SQUARE,  // "SQUARE"
    SAWTOOTH,  // "SAWTOOTH"
    TRIANGLE,  // "TRIANGLE"
    CUSTOM,  // "CUSTOM"
};

// PickSensorMatchCriterionChoices: pickSensorMatchCriterionChoices are strictly allowed enumeration values for X3DPickSensorNode node matchCriterion field. This list is bounded, no additional values are allowed.
enum class PickSensorMatchCriterionChoices {
    MATCH_ANY,  // "MATCH_ANY"
    MATCH_EVERY,  // "MATCH_EVERY"
    MATCH_ONLY_ONE,  // "MATCH_ONLY_ONE"
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
