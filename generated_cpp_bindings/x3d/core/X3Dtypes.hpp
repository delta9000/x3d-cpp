#pragma once

#include <vector>
#include <memory>
#include <string>

namespace x3d::nodes { class X3DNode; }

namespace x3d::core {

// Special structs

    struct SFVec2d {
        double x, y;
    };

    struct SFVec2f {
        float x, y;
    };

    struct SFVec3d {
        double x, y, z;
    };

    struct SFVec3f {
        float x, y, z;
    };

    struct SFVec4d {
        double x, y, z, w;
    };

    struct SFVec4f {
        float x, y, z, w;
    };

    struct SFColor {
        float r, g, b;
    };

    struct SFColorRGBA {
        float r, g, b, a;
    };

    struct SFRotation {
        float x, y, z, angle;
    };

    struct SFMatrix3d {
        double matrix[3][3];
    };

    struct SFMatrix3f {
        float matrix[3][3];
    };

    struct SFMatrix4d {
        double matrix[4][4];
    };

    struct SFMatrix4f {
        float matrix[4][4];
    };

    struct SFImage {
        int width, height, numComponents;
        std::vector<unsigned char> data;
    };

typedef bool SFBool;
typedef double SFDouble;
typedef float SFFloat;
typedef int SFInt32;
using SFNode = std::shared_ptr<nodes::X3DNode>;
typedef std::string SFString;
typedef double SFTime;
typedef std::vector<bool> MFBool;
typedef std::vector<SFColor> MFColor;
typedef std::vector<SFColorRGBA> MFColorRGBA;
typedef std::vector<double> MFDouble;
typedef std::vector<float> MFFloat;
typedef std::vector<SFImage> MFImage;
typedef std::vector<int> MFInt32;
typedef std::vector<SFMatrix3d> MFMatrix3d;
typedef std::vector<SFMatrix3f> MFMatrix3f;
typedef std::vector<SFMatrix4d> MFMatrix4d;
typedef std::vector<SFMatrix4f> MFMatrix4f;
using MFNode = std::vector<std::shared_ptr<nodes::X3DNode>>;
typedef std::vector<SFRotation> MFRotation;
typedef std::vector<std::string> MFString;
typedef std::vector<double> MFTime;
typedef std::vector<SFVec2d> MFVec2d;
typedef std::vector<SFVec2f> MFVec2f;
typedef std::vector<SFVec3d> MFVec3d;
typedef std::vector<SFVec3f> MFVec3f;
typedef std::vector<SFVec4d> MFVec4d;
typedef std::vector<SFVec4f> MFVec4f;
typedef std::string xs_nmtoken;


} // namespace x3d::core
