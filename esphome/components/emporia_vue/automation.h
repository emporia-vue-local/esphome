#pragma once

#include "emporia_vue.h"

#include "esphome/core/component.h"
#include "esphome/core/automation.h"

namespace esphome {
namespace emporia_vue {

// Trigger for after statistics sensors are updated
class EmporiaVueUpdateTrigger : public Trigger<> {
 public:
  explicit EmporiaVueUpdateTrigger(EmporiaVueComponent *parent) {
    parent->add_on_update_callback([this]() { this->trigger(); });
  }
};

}  // namespace statistics
}  // namespace esphome
