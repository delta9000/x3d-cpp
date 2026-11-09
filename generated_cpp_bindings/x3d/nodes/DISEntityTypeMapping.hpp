#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

#include "x3d/nodes/X3DInfoNode.hpp"

#include "x3d/nodes/X3DUrlObject.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class DISEntityTypeMapping
 * @brief DISEntityTypeMapping provides a best-match mapping from DIS ESPDU
 * entity type information to a specific X3D model, thus providing a visual and
 * behavioral representation that best matches the entity type.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/dis.html#DISEntityTypeMapping
 */
class DISEntityTypeMapping : public virtual X3DInfoNode,
                             public virtual X3DUrlObject {
public:
  DISEntityTypeMapping() = default;

  ~DISEntityTypeMapping() = default;

  static SFInt32 getDefaultCategory() { return 0; }

  static SFInt32 getDefaultCountry() { return 0; }

  static SFInt32 getDefaultDomain() { return 0; }

  static SFInt32 getDefaultExtra() { return 0; }

  static SFInt32 getDefaultKind() { return 0; }

  static SFInt32 getDefaultSpecific() { return 0; }

  static SFInt32 getDefaultSubcategory() { return 0; }

  static std::string getContainerFieldType() {
    return "containerFieldChoicesDISEntityTypeMapping";
  }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "DIS"; }

  static int componentLevel() { return 2; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Integer enumerations value for main category that describes the
   * entity, semantics of each code varies according to domain.
   */
  const SFInt32 &getCategory() const { return _category; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCategoryUnchecked(const SFInt32 &value) { _category = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Integer enumerations value for country to which the design of the
   * entity or its design specification is attributed.
   */
  const SFInt32 &getCountry() const { return _country; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCountryUnchecked(const SFInt32 &value) { _country = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Integer enumerations value for domain in which the entity
   * operates: LAND, AIR, SURFACE, SUBSURFACE, SPACE or OTHER.
   */
  const SFInt32 &getDomain() const { return _domain; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setDomainUnchecked(const SFInt32 &value) { _domain = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Any extra information required to describe a particular entity.
   */
  const SFInt32 &getExtra() const { return _extra; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setExtraUnchecked(const SFInt32 &value) { _extra = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Integer enumerations value for whether entity is a PLATFORM,
   * MUNITION, LIFE_FORM, ENVIRONMENTAL, CULTURAL_FEATURE, SUPPLY, RADIO,
   * EXPENDABLE, SENSOR_EMITTER or OTHER.
   */
  const SFInt32 &getKind() const { return _kind; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setKindUnchecked(const SFInt32 &value) { _kind = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Specific information about an entity based on the subcategory
   * field.
   */
  const SFInt32 &getSpecific() const { return _specific; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSpecificUnchecked(const SFInt32 &value) { _specific = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Integer enumerations value for particular subcategory to which an
   * entity belongs based on the category field.
   */
  const SFInt32 &getSubcategory() const { return _subcategory; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setSubcategoryUnchecked(const SFInt32 &value) { _subcategory = value; }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesCategory(const SFInt32 &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesCountry(const SFInt32 &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesDomain(const SFInt32 &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesExtra(const SFInt32 &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesKind(const SFInt32 &value, const std::string &nodeType,
                              const std::string &defName,
                              std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSpecific(const SFInt32 &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSubcategory(const SFInt32 &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  SFInt32 _category{0};

  SFInt32 _country{0};

  SFInt32 _domain{0};

  SFInt32 _extra{0};

  SFInt32 _kind{0};

  SFInt32 _specific{0};

  SFInt32 _subcategory{0};
};

} // namespace x3d::nodes
