"""X3D field-type tables consumed by the UOM parser.

Copied from x3d-cpp's generator at the 2026-07-19 split. These map X3D field
types to the C++ spellings the parser records; the SAI emitters carry their own
value-type mapping in emit/sai_bindings.py.
"""

FIELD_TYPE_MAPPING = {
    # Single-value types
    "SFBool": "bool",
    "SFColor": "SFColor",
    "SFColorRGBA": "SFColorRGBA",
    "SFDouble": "double",
    "SFFloat": "float",
    "SFImage": "SFImage",
    "SFInt32": "int",
    "SFMatrix3d": "SFMatrix3d",
    "SFMatrix3f": "SFMatrix3f",
    "SFMatrix4d": "SFMatrix4d",
    "SFMatrix4f": "SFMatrix4f",
    "SFNode": "std::shared_ptr<X3DNode>",
    "SFRotation": "SFRotation",
    "SFString": "std::string",
    "SFTime": "double",
    "SFVec2d": "SFVec2d",
    "SFVec2f": "SFVec2f",
    "SFVec3d": "SFVec3d",
    "SFVec3f": "SFVec3f",
    "SFVec4d": "SFVec4d",
    "SFVec4f": "SFVec4f",
    # Multi-value types
    "MFBool": "std::vector<bool>",
    "MFColor": "std::vector<SFColor>",
    "MFColorRGBA": "std::vector<SFColorRGBA>",
    "MFDouble": "std::vector<double>",
    "MFFloat": "std::vector<float>",
    "MFImage": "std::vector<SFImage>",
    "MFInt32": "std::vector<int>",
    "MFMatrix3d": "std::vector<SFMatrix3d>",
    "MFMatrix3f": "std::vector<SFMatrix3f>",
    "MFMatrix4d": "std::vector<SFMatrix4d>",
    "MFMatrix4f": "std::vector<SFMatrix4f>",
    "MFNode": "std::vector<std::shared_ptr<X3DNode>>",
    "MFRotation": "std::vector<SFRotation>",
    "MFString": "std::vector<std::string>",
    "MFTime": "std::vector<double>",
    "MFVec2d": "std::vector<SFVec2d>",
    "MFVec2f": "std::vector<SFVec2f>",
    "MFVec3d": "std::vector<SFVec3d>",
    "MFVec3f": "std::vector<SFVec3f>",
    "MFVec4d": "std::vector<SFVec4d>",
    "MFVec4f": "std::vector<SFVec4f>",
}

XS_TYPES = {
    "xs:NMTOKEN": ("xs_nmtoken","std::string"),
}
