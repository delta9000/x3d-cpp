#include "UnitSerialization.hpp"
#include "XmlWriter.hpp"

#include "FieldValueIO.hpp"
#include "VersionHeader.hpp"
#include "X3DRuntime.hpp"
#include "XmlGraphBuilder.hpp"
#include "XmlLite.hpp"

#include <sstream>

namespace x3d::codec {

std::string XmlWriter::writeDocument(const runtime::X3DDocument &doc) {
  requireUnnormalizedUnits(doc.scene);
  XmlGraphBuilder graph({.formatValue = formatValue,
                         .authoredChildOrder = true,
                         .sortAttributes = false});
  auto root = std::make_unique<xml::Element>();
  root->name = "X3D";
  root->setAttr("profile", doc.profileToken());
  root->setAttr("version", headerVersion(doc.version));

  // <head>
  if (!doc.head.empty()) {
    xml::Element *head = root->addChild("head");
    for (const auto &c : doc.head.components) {
      xml::Element *e = head->addChild("component");
      e->setAttr("name", c.name);
      e->setAttr("level", std::to_string(c.level));
    }
    for (const auto &u : doc.head.units) {
      xml::Element *e = head->addChild("unit");
      e->setAttr("category", u.category);
      e->setAttr("name", u.name);
      e->setAttr("conversionFactor", fmtDouble(u.conversionFactor));
    }
    for (const auto &m : doc.head.meta) {
      xml::Element *e = head->addChild("meta");
      e->setAttr("name", m.name);
      e->setAttr("content", m.content);
      if (!m.dir.empty())
        e->setAttr("dir", m.dir);
      if (!m.httpEquiv.empty())
        e->setAttr("http-equiv", m.httpEquiv);
      if (!m.lang.empty())
        e->setAttr("lang", m.lang);
      if (!m.scheme.empty())
        e->setAttr("scheme", m.scheme);
    }
  }

  // <Scene>
  xml::Element *scene = root->addChild("Scene");
  graph.writeSceneInto(scene, doc.scene);

  std::ostringstream os;
  os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  graph.orderAttributes(*root);
  render(*root, os, 0);
  return os.str();
}

std::string XmlWriter::writeScene(const runtime::Scene &scene) {
  requireUnnormalizedUnits(scene);
  XmlGraphBuilder graph({.formatValue = formatValue,
                         .authoredChildOrder = true,
                         .sortAttributes = false});
  auto el = std::make_unique<xml::Element>();
  el->name = "Scene";
  graph.writeSceneInto(el.get(), scene);
  std::ostringstream os;
  graph.orderAttributes(*el);
  render(*el, os, 0);
  return os.str();
}

std::string
XmlWriter::writeNode(const std::shared_ptr<X3DNode> &node,
                     const runtime::DynamicFieldStore *authorFields) {
  XmlGraphBuilder graph({.formatValue = formatValue,
                         .authoredChildOrder = true,
                         .sortAttributes = false},
                        authorFields);
  auto el = graph.writeNodeElement(node, "");
  std::ostringstream os;
  if (el) {
    graph.orderAttributes(*el);
    render(*el, os, 0);
  }
  return os.str();
}

void XmlWriter::indent(std::ostringstream &os, int depth) {
  for (int i = 0; i < depth; ++i)
    os << "  ";
}

void XmlWriter::render(const xml::Element &el, std::ostringstream &os,
                       int depth) {
  indent(os, depth);
  os << "<" << el.name;
  for (const auto &a : el.attributes) {
    os << " " << a.first << "=\"" << xml::escape(a.second) << "\"";
  }
  if (el.children.empty() && el.text.empty()) {
    os << "/>\n";
    return;
  }
  os << ">\n";
  for (const auto &c : el.children) {
    render(*c, os, depth + 1);
  }
  // Character content (e.g. a Script's inline source) is emitted as a CDATA
  // section so special characters need no escaping and the reader recovers it
  // verbatim from Element.text . Empty text is skipped above.
  if (!el.text.empty()) {
    indent(os, depth + 1);
    os << "<![CDATA[" << xml::cdataEscape(el.text) << "]]>\n";
  }
  indent(os, depth);
  os << "</" << el.name << ">\n";
}

} // namespace x3d::codec
