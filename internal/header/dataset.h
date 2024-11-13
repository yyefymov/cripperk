#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include <string>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <variant>

enum AttributeType {
  NONE, // invalid type
  DISCRETE,
  CONTINUOUS
};

using AttributeValue = std::variant<std::string, float>; // holds a discrete value as string or a continuous value as float

struct Attribute {
  std::string name;
  AttributeType type;
  AttributeValue value;
};

struct Instance {
  std::string class_value;
  std::vector<Attribute> attributes;
};

class AttributeManager {
public:
  AttributeManager() = default;
  AttributeManager(const std::list<Instance>& dataset);
  std::list<AttributeValue> getPossibleValues(const std::string& attr_name) const;
  std::list<std::string> getAttributeNames() const;
  AttributeType getAttributeType(const std::string &attr_name) const;

private:
  std::map<std::string, std::set<AttributeValue>> possible_attr_values;
  std::map<std::string, AttributeType> attribute_types;
};

#endif