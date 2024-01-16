#pragma once

#include "quoll/io/SerializerBase.h"

namespace quoll {

class NameSerializer {
public:
  static void serialize(YAML::Node &node, EntityDatabase &entityDatabase,
                        Entity entity);

  static void deserialize(const YAML::Node &node,
                          EntityDatabase &entityDatabase, Entity entity,
                          const EntityIdCache &cache);
};

} // namespace quoll
