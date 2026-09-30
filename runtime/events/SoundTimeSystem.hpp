// SoundTimeSystem.hpp — shared §8.2.4 clock lifecycle for time-dependent
// source and processing nodes in the §16 audio graph.
#ifndef X3D_RUNTIME_SOUND_TIME_SYSTEM_HPP
#define X3D_RUNTIME_SOUND_TIME_SYSTEM_HPP

#include "X3DExecutionContext.hpp"
#include "X3DTimeDependentSystem.hpp"

#include "x3d/nodes/BiquadFilter.hpp"
#include "x3d/nodes/Delay.hpp"
#include "x3d/nodes/DynamicsCompressor.hpp"
#include "x3d/nodes/Gain.hpp"
#include "x3d/nodes/OscillatorSource.hpp"

#include <limits>

namespace x3d::runtime {

class SoundTimeSystem : public X3DTimeDependentSystem {
public:
  void attach(x3d::nodes::X3DNode *node, X3DExecutionContext &ctx) override {
    if (dynamic_cast<x3d::nodes::OscillatorSource *>(node) ||
        dynamic_cast<x3d::nodes::Gain *>(node) ||
        dynamic_cast<x3d::nodes::BiquadFilter *>(node) ||
        dynamic_cast<x3d::nodes::Delay *>(node) ||
        dynamic_cast<x3d::nodes::DynamicsCompressor *>(node))
      X3DTimeDependentSystem::attach(node, ctx);
  }

protected:
  bool readEnabled(x3d::nodes::X3DTimeDependentNode *node) const override {
    if (auto *n = dynamic_cast<x3d::nodes::OscillatorSource *>(node)) return n->getEnabled();
    if (auto *n = dynamic_cast<x3d::nodes::Gain *>(node)) return n->getEnabled();
    if (auto *n = dynamic_cast<x3d::nodes::BiquadFilter *>(node)) return n->getEnabled();
    if (auto *n = dynamic_cast<x3d::nodes::Delay *>(node)) return n->getEnabled();
    if (auto *n = dynamic_cast<x3d::nodes::DynamicsCompressor *>(node)) return n->getEnabled();
    return true;
  }
  double readCycleInterval(x3d::nodes::X3DTimeDependentNode *) const override {
    return std::numeric_limits<double>::infinity();
  }
};

} // namespace x3d::runtime

#endif // X3D_RUNTIME_SOUND_TIME_SYSTEM_HPP
