// X3DSemanticMetadataRegistry.hpp
// Auto-generated: owning, instance-free X3D semantic descriptors.
#pragma once

#include "x3d/core/X3DReflection.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace x3d::nodes {

struct SemanticFieldDescriptor {
    std::string name;
    x3d::core::X3DFieldType type;
    x3d::core::AccessType access;
    std::optional<std::string> defaultValue;
    std::vector<std::string> acceptableNodeTypes;
    std::optional<std::string> unitCategory;
};

struct SemanticNodeDescriptor {
    std::string name;
    bool abstract = false;
    std::string component;
    int level = 0;
    std::vector<std::string> interfaces;
    std::vector<SemanticFieldDescriptor> fields;
};

class X3DSemanticMetadataRegistry {
public:
    static std::string_view specificationVersion() noexcept;
    static std::string_view modelFingerprint() noexcept;
    static std::string_view generatorVersion() noexcept;
    static bool unitCategoriesComplete() noexcept;
    static std::span<const SemanticNodeDescriptor> nodes();
    static const SemanticNodeDescriptor* find(const std::string& name);
};

} // namespace x3d::nodes
