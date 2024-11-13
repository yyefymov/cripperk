#ifndef MODEL_H
#define MODEL_H

#include "rule.h"
#include <map>
#include <list>
#include <memory>

class Model {
public:
  Model(std::shared_ptr<const AttributeManager> attribute_manager);

  void add(const std::string& class_name, const Ruleset& ruleset, bool auto_set_order=true);
  Ruleset& get(const std::string& class_name);
  void setDefaultClass(const std::string& class_name);
  std::string getDefaultClass() const;
  void setClassOrder(const std::map<std::string, size_t>& class_order);
  const std::list<std::string>& getClassOrder() const;
  void write(const std::string& path_to_model_txt, const std::string& path_to_model_bin) const;
  void read(const std::string& path_to_model_bin);
private:
  std::map<std::string, Ruleset> model;
  std::list<std::string> class_order;
  std::string default_class_name;
  std::shared_ptr<const AttributeManager> attribute_manager;
};

void inline Model::setDefaultClass(const std::string& class_name) {
  this->default_class_name = class_name;
}

std::string inline Model::getDefaultClass() const {
  return this->default_class_name;
}

// void inline Model::setClassOrder(const std::map<std::string, size_t>& class_order) {
//   this->class_order = class_order;
// }

#endif