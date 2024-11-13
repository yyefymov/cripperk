#include "../header/dataset.h"

AttributeManager::AttributeManager(const std::list<Instance>& dataset)
{
  // std::map<std::string, std::set<AttributeValue>> possible_attr_values;
  for (const auto& instance: dataset) {
    for (const auto& attr: instance.attributes) {
      this->possible_attr_values[attr.name].insert(attr.value);
      this->attribute_types[attr.name] = attr.type;
    }
  }
}

std::list<AttributeValue> AttributeManager::getPossibleValues(const std::string &attr_name) const
{
  std::list<AttributeValue> list_of_values;
  const auto values = this->possible_attr_values.at(attr_name); // throws if not found!

  for (const auto& value: values) {
    list_of_values.push_back(value);
  }

  return list_of_values;
}

std::list<std::string> AttributeManager::getAttributeNames() const
{
  std::list<std::string> list_of_names;

  for (const auto& kv: this->possible_attr_values) {
    list_of_names.push_back(kv.first);
  }

  return list_of_names;
}

AttributeType AttributeManager::getAttributeType(const std::string &attr_name) const
{
  return this->attribute_types.at(attr_name); // throws if not found
}
