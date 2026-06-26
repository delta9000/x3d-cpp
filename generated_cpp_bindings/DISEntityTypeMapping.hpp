// DISEntityTypeMapping.hpp
#ifndef DISENTITYTYPEMAPPING_HPP
#define DISENTITYTYPEMAPPING_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DInfoNode.hpp"

#include "X3DUrlObject.hpp"

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
  /**
   * @brief Default constructor for DISEntityTypeMapping
   */
  DISEntityTypeMapping() = default;

  /**
   * @brief Destructor for DISEntityTypeMapping
   */
  ~DISEntityTypeMapping() = default;

  /**
   * @brief Get the default value for category
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultCategory() { return 0; }

  /**
   * @brief Get the default value for country
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultCountry() { return 0; }

  /**
   * @brief Get the default value for domain
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultDomain() { return 0; }

  /**
   * @brief Get the default value for extra
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultExtra() { return 0; }

  /**
   * @brief Get the default value for kind
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultKind() { return 0; }

  /**
   * @brief Get the default value for specific
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultSpecific() { return 0; }

  /**
   * @brief Get the default value for subcategory
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultSubcategory() { return 0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() {
    return "containerFieldChoicesDISEntityTypeMapping";
  }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "DIS"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of category. AccessType: initializeOnly
   * @details Integer enumerations value for main category that describes the
   * entity, semantics of each code varies according to domain.
   * @return SFInt32 The current value of category.
   */
  SFInt32 getCategory() const { return _category; }
  /**
   * @brief Data-layer write of category (reader/init ingest path).
   * @details category is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCategory().
   */
  void setCategoryUnchecked(const SFInt32 &value) { _category = value; }

  /**
   * @brief Gets the value of country. AccessType: initializeOnly
   * @details Integer enumerations value for country to which the design of the
   * entity or its design specification is attributed.
   * @return SFInt32 The current value of country.
   */
  SFInt32 getCountry() const { return _country; }
  /**
   * @brief Data-layer write of country (reader/init ingest path).
   * @details country is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCountry().
   */
  void setCountryUnchecked(const SFInt32 &value) { _country = value; }

  /**
   * @brief Gets the value of domain. AccessType: initializeOnly
   * @details Integer enumerations value for domain in which the entity
   * operates: LAND, AIR, SURFACE, SUBSURFACE, SPACE or OTHER.
   * @return SFInt32 The current value of domain.
   */
  SFInt32 getDomain() const { return _domain; }
  /**
   * @brief Data-layer write of domain (reader/init ingest path).
   * @details domain is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setDomain().
   */
  void setDomainUnchecked(const SFInt32 &value) { _domain = value; }

  /**
   * @brief Gets the value of extra. AccessType: initializeOnly
   * @details Any extra information required to describe a particular entity.
   * @return SFInt32 The current value of extra.
   */
  SFInt32 getExtra() const { return _extra; }
  /**
   * @brief Data-layer write of extra (reader/init ingest path).
   * @details extra is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setExtra().
   */
  void setExtraUnchecked(const SFInt32 &value) { _extra = value; }

  /**
   * @brief Gets the value of kind. AccessType: initializeOnly
   * @details Integer enumerations value for whether entity is a PLATFORM,
   * MUNITION, LIFE_FORM, ENVIRONMENTAL, CULTURAL_FEATURE, SUPPLY, RADIO,
   * EXPENDABLE, SENSOR_EMITTER or OTHER.
   * @return SFInt32 The current value of kind.
   */
  SFInt32 getKind() const { return _kind; }
  /**
   * @brief Data-layer write of kind (reader/init ingest path).
   * @details kind is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setKind().
   */
  void setKindUnchecked(const SFInt32 &value) { _kind = value; }

  /**
   * @brief Gets the value of specific. AccessType: initializeOnly
   * @details Specific information about an entity based on the subcategory
   * field.
   * @return SFInt32 The current value of specific.
   */
  SFInt32 getSpecific() const { return _specific; }
  /**
   * @brief Data-layer write of specific (reader/init ingest path).
   * @details specific is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setSpecific().
   */
  void setSpecificUnchecked(const SFInt32 &value) { _specific = value; }

  /**
   * @brief Gets the value of subcategory. AccessType: initializeOnly
   * @details Integer enumerations value for particular subcategory to which an
   * entity belongs based on the category field.
   * @return SFInt32 The current value of subcategory.
   */
  SFInt32 getSubcategory() const { return _subcategory; }
  /**
   * @brief Data-layer write of subcategory (reader/init ingest path).
   * @details subcategory is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setSubcategory().
   */
  void setSubcategoryUnchecked(const SFInt32 &value) { _subcategory = value; }

  /**
   * @brief The X3D type name of this node (e.g. "DISEntityTypeMapping").
   */
  std::string nodeTypeName() const override;

  /**
   * @brief This node's default containerField: the parent field it attaches
   *        to when an X3D-XML element gives no explicit containerField. Virtual
   *        so codecs can resolve it polymorphically through an X3DNode base
   *        pointer (the static getDefaultContainerField() is not reachable that
   *        way). Mirrors getDefaultContainerField().
   */
  std::string defaultContainerField() const override;

  /**
   * @brief Reflected field table for this node (own + inherited fields).
   * @details Built once (function-local static) from this node's descriptors.
   *          Each FieldInfo carries type-erased get/set thunks bound to this
   *          node's strongly-typed accessors so codecs need no per-node code.
   */
  const FieldTable &fields() const override;

  /**
   * @brief Visitor double-dispatch entry point.
   */
  void accept(NodeVisitor &visitor) const override;

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  /**
   * @brief Member variable for category.
   */

  SFInt32 _category{0};

  /**
   * @brief Member variable for country.
   */

  SFInt32 _country{0};

  /**
   * @brief Member variable for domain.
   */

  SFInt32 _domain{0};

  /**
   * @brief Member variable for extra.
   */

  SFInt32 _extra{0};

  /**
   * @brief Member variable for kind.
   */

  SFInt32 _kind{0};

  /**
   * @brief Member variable for specific.
   */

  SFInt32 _specific{0};

  /**
   * @brief Member variable for subcategory.
   */

  SFInt32 _subcategory{0};
};

#endif // DISENTITYTYPEMAPPING_HPP