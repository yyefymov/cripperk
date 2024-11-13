#include "../header/model.h"
#include <iostream>
#include <fstream>

Model::Model(std::shared_ptr<const AttributeManager> attribute_manager)
  : attribute_manager(attribute_manager)
{}

void Model::add(const std::string& class_name, const Ruleset& ruleset, bool auto_set_order) {
  this->model[class_name] = std::move(ruleset);
  if (auto_set_order) {
    class_order.push_back(class_name);
  }
}

Ruleset& Model::get(const std::string& class_name) {
  return this->model[class_name];
}

const std::list<std::string>& Model::getClassOrder() const {
  return this->class_order;
}

void Model::write(const std::string& path_to_model_txt, const std::string& path_to_model_bin) const {
  std::ofstream model_txt(path_to_model_txt);

  // write to a text file in a human-readable format
  if (model_txt.is_open()) {
    for (const auto& class_name: class_order) {
      model_txt << this->model.at(class_name).toString() << " THEN " << class_name << std::endl;
      model_txt << "ELSE ";
    }
    model_txt <<  this->default_class_name << std::endl;

    model_txt.close();
  }

  // write to a bin file
  std::ofstream model_bin(path_to_model_bin);

  // bin file structure
  //
  // number of classes
  // class1 name length | class1 name
  //  number of rules
  //    number of conditions (rule1)
  //      |condition1|condition2|...|conditionN|
  //    number_of_conditions (rulei)
  //      |condition1|condition2|...|conditionN|
  // classi name length | classi name
  //  ...
  // default class name length | default class name

  if (model_bin.is_open()) {
    size_t number_of_classes = class_order.size();
    model_bin.write(reinterpret_cast<const char*>(&number_of_classes), sizeof(number_of_classes));

    for (const auto& class_name: class_order) {
      size_t class_name_len = class_name.size();
      const Ruleset& ruleset = this->model.at(class_name);
      size_t number_of_rules = ruleset.size();
      auto rules = ruleset.get();

      model_bin.write(reinterpret_cast<const char*>(&class_name_len), sizeof(class_name_len));
      model_bin.write(class_name.c_str(), class_name_len);
      model_bin.write(reinterpret_cast<const char*>(&number_of_rules), sizeof(number_of_rules));

      for (const auto& rule_handle: rules)
        ruleset.getRule(rule_handle).write_bin(model_bin);
    }

    size_t default_class_name_len = this->default_class_name.size();
    model_bin.write(reinterpret_cast<const char*>(&default_class_name_len), sizeof(default_class_name_len));
    model_bin.write(this->default_class_name.c_str(), default_class_name_len);

    model_bin.close();
  }
}

void Model::read(const std::string& path_to_model_bin) {
  std::ifstream model_bin(path_to_model_bin, std::ios::binary);

  if (model_bin.is_open()) {
    size_t number_of_classes = 0;
    unsigned class_priority = 1;
    std::string class_name;
    size_t class_name_len = 0;
    size_t number_of_rules = 0;

    model_bin.read(reinterpret_cast<char*>(&number_of_classes), sizeof(number_of_classes));

    while (number_of_classes--) { // write all but default
      Ruleset ruleset;
      model_bin.read(reinterpret_cast<char*>(&class_name_len), sizeof(class_name_len));

      std::vector<char> buf(class_name_len + 1);
      model_bin.read(buf.data(), class_name_len);
      buf.push_back('\0');

      class_name = buf.data();
      class_order.push_back(class_name);

      model_bin.read(reinterpret_cast<char*>(&number_of_rules), sizeof(number_of_rules));
      while (number_of_rules--) {
        Rule rule(this->attribute_manager);
        rule.read_bin(model_bin);
        ruleset.addRule(rule);
      }
      this->model[class_name] = std::move(ruleset);
    }

    size_t default_class_name_len = 0;
    model_bin.read(reinterpret_cast<char*>(&default_class_name_len), sizeof(default_class_name_len));

    std::vector<char> buf(default_class_name_len + 1);
    model_bin.read(buf.data(), default_class_name_len);
    buf.push_back('\0');
    this->default_class_name = buf.data();

    model_bin.close();
  } else {
    std::cerr << "Failed to open the model file" << std::endl;
  }
}