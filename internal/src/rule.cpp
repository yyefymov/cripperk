#include "../header/rule.h"
#include "../header/mathutils.h"
#include <limits>
#include <cmath>
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <fstream>
#include <memory>
#include <iostream>

Rule::Rule()
  : attribute_manager(nullptr)
{}

Rule::Rule(std::shared_ptr<const AttributeManager> attribute_manager)
  : attribute_manager(attribute_manager)
{}

Rule::Rule(const Rule& rule)
  : attribute_manager(rule.attribute_manager)
  , conditions(rule.conditions)
{}

Rule& Rule::operator=(const Rule& rhs) {
  if (this == &rhs)
    return *this;

  this->attribute_manager = rhs.attribute_manager;
  return *this;
}

float Rule::foil_gain(const Condition &condition, const std::list<Instance> &pos, const std::list<Instance> &neg) const
{
  Rule rule_with_condition{this->attribute_manager};
  rule_with_condition.conditions.push_back(condition);

  float p = cover(pos);
  float n = cover(neg);
  float p_new = rule_with_condition.cover(pos);
  float n_new = rule_with_condition.cover(neg);

  if (((p + n) == 0) || (p == 0))
    return 0.0f;
  if (((p_new + n_new) == 0) || (p_new == 0))
    return 0.0f;

  auto calc_val = [](float p, float n) {
    return std::log2(p / (p + n));
  };

  // for debug - remove later
  auto a = calc_val(p,n);
  auto b = calc_val(p_new, n_new);

  float result = p * (calc_val(p_new, n_new) - calc_val(p, n));

  if (result < 0.0f)
    return 0.0f;
  return result;

  return p * (calc_val(p_new, n_new) - calc_val(p, n));
}

void Rule::grow(std::list<Instance> pos, std::list<Instance> neg)
{
  if (!attribute_manager) {
    std::cout << "Attribute manager is not initialized. Can't grow rules without attributes!" << std::endl;
    return;
  }

  while (true) {
    std::optional<float> max_gain;
    Condition next_condition{};
    auto attr_names = attribute_manager->getAttributeNames();
    for (const auto& attr_name: attr_names) {
      // do not allow duplicate conditions in one rule
      if (attribute_manager->getAttributeType(attr_name) != CONTINUOUS && std::find_if(conditions.begin(), conditions.end(), [&attr_name](const auto& condition){return condition.attr_name == attr_name;}) != conditions.end())
        continue;

      auto attr_values = attribute_manager->getPossibleValues(attr_name);
      for (const auto& attr_value: attr_values) {
        Condition condition[2];
        size_t num_of_conditions = (attribute_manager->getAttributeType(attr_name) == CONTINUOUS) ? 2 : 1;

        if (attribute_manager->getAttributeType(attr_name) == CONTINUOUS) {
          condition[0].attr_name = attr_name;
          condition[0].attr_value = attr_value;
          condition[0].cond_operator = LESS_EQ;

          condition[1].attr_name = attr_name;
          condition[1].attr_value = attr_value;
          condition[1].cond_operator = MORE_EQ;
        } else {
          condition[0].attr_name = attr_name;
          condition[0].attr_value = attr_value;
          condition[0].cond_operator = EQ;
        }

        for (auto i = 0; i < num_of_conditions; ++i) {
          auto gain = foil_gain(condition[i], pos, neg);
          if (gain <= 0.0f)
            continue; // this condition does not increase coverage - don't even consider

          if (!max_gain.has_value() || gain > max_gain.value()) {
            next_condition = condition[i];
            max_gain = gain;
          }
        }
      }
    }
    if (next_condition.attr_name.empty())
      return; // all possible conditions are added to the rule
      // throw std::runtime_error("no condition was selected for a rule");

    conditions.push_back(next_condition);

    if (cover(neg) == 0)
      return;
  }
}

float Rule::dl() const {
  float n = 0;
  float k = 0;
  float p_r = 0;
  float k_bits = 0;

  for (const auto& attr_name: attribute_manager->getAttributeNames())
    for (const auto& attr_value: attribute_manager->getPossibleValues(attr_name))
      ++n;

  k = conditions.size();
  p_r = k / n;
  k_bits = std::ceil(std::log2(k + 1));

  return std::ceil((k * std::log2(1/p_r) + (n - k) * std::log2(1 / (1 + p_r)) + k_bits) * 0.5);
}

float Rule::dl_err(const std::list<Instance>& pos, const std::list<Instance>& neg) const
{
  float p = 0;
  float fp = 0;
  float n = 0;
  float fn = 0;

  p = cover(pos) + cover(neg);
  fp = cover(neg);
  n = (pos.size() + neg.size()) - p;
  fn = pos.size() - cover(pos);

  // TODO: store the result for debug, remove later
  auto result = MathUtils::log2_combination(p, fp) + MathUtils::log2_combination(n, fn);
  return result;

  // old formula - overflows when factorial is too big
  // return std::log2(factorial(p) / (factorial(fp) * factorial(p-fp))) + std::log2(factorial(n) / (factorial(fn) * factorial(n-fn)));
}

std::string Rule::toString() const
{
  std::string rule_str;
  size_t pos = 0;

  rule_str.append("IF ");
  for (auto it = this->conditions.cbegin(); it != this->conditions.cend(); std::advance(it, 1)) {
    auto condition = *it;
    rule_str.append(condition.attr_name);

    switch (condition.cond_operator) {
      case EQ:
        rule_str.append(" == ");
        break;
      case LESS_EQ:
        rule_str.append(" <= ");
        break;
      case MORE_EQ:
        rule_str.append(" >= ");
        break;
      default:
        break;
    }

    if (attribute_manager->getAttributeType(condition.attr_name) == DISCRETE) {
      rule_str.append(std::get<std::string>(condition.attr_value));
    }
    else {
      float val = std::get<float>(condition.attr_value);
      if (std::floor(val) == val)
        rule_str.append(std::to_string((int)val));
      else
        rule_str.append(std::to_string(val));
    }

    if (it != this->conditions.end() - 1)
      rule_str.append(" AND ");
  }

  return rule_str;
}

bool Rule::empty() const {
  return this->conditions.empty();
}

void Rule::prune(std::list<Instance> pos, std::list<Instance> neg)
{
  Rule tmp_rule(*this);
  float p = cover(pos);
  float n = cover(neg);
  float max_metric = (p - n) / (p + n); // prune metric --> move to function

  if (this->conditions.size() == 1)
    return;

  auto size = tmp_rule.conditions.size();
  for (auto i = 0; i < size; ++i) {
    tmp_rule.conditions.pop_back();

    p = tmp_rule.cover(pos);
    n = tmp_rule.cover(neg);
    float metric = (p - n) / (p + n); // prune metric

    if (metric > max_metric) {
      max_metric = metric;
      conditions = tmp_rule.conditions;
    }
  }
}

void Rule::write_bin(std::ofstream& model_bin) const {
  if (!model_bin.is_open())
    throw std::runtime_error("Model file must be open!");

  // structure:
  //
  // number of conditions
  // |condition1|condition2|...|conditionN|
  size_t number_of_conditions = this->conditions.size();
  model_bin.write(reinterpret_cast<const char*>(&number_of_conditions), sizeof(this->conditions.size()));

  for (const auto& condition: this->conditions) {
    size_t name_len = condition.attr_name.size();

    // |operator|name_len|name|value|
    model_bin.write(reinterpret_cast<const char*>(&condition.cond_operator), sizeof(condition.cond_operator));
    model_bin.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
    model_bin.write(condition.attr_name.c_str(), name_len);

    if (attribute_manager->getAttributeType(condition.attr_name) == CONTINUOUS)
      model_bin.write(reinterpret_cast<const char*>(&condition.attr_value), sizeof(condition.attr_value));
    else {
      size_t value_len = std::get<std::string>(condition.attr_value).size();

      model_bin.write(reinterpret_cast<const char*>(&value_len), sizeof(value_len));
      model_bin.write(std::get<std::string>(condition.attr_value).c_str(), value_len);
    }
  }
}


void Rule::read_bin(std::ifstream& model_bin) {
  if (!model_bin.is_open())
    throw std::runtime_error("Model file must be open!");

  size_t number_of_conditions = 0;

  model_bin.read(reinterpret_cast<char*>(&number_of_conditions), sizeof(number_of_conditions));
  while (number_of_conditions--) {
    Condition condition;
    size_t name_len = 0;

    model_bin.read(reinterpret_cast<char*>(&condition.cond_operator), sizeof(condition.cond_operator));

    model_bin.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
    std::vector<char> buf(name_len);
    model_bin.read(buf.data(), name_len);
    buf.push_back('\0');
    condition.attr_name = buf.data();

    if (attribute_manager->getAttributeType(condition.attr_name) == CONTINUOUS)
      model_bin.read(reinterpret_cast<char*>(&condition.attr_value), sizeof(condition.attr_value));
    else {
      size_t value_len = 0;
      model_bin.read(reinterpret_cast<char*>(&value_len), sizeof(value_len));

      buf = std::vector<char>(value_len);
      model_bin.read(buf.data(), value_len);
      buf.push_back('\0');
      condition.attr_value = buf.data();
    }

    this->conditions.emplace_back(condition);
  }
}

void Rule::addCondition(const Condition& condition) {
  this->conditions.push_back(condition);
}

void Rule::removeLastCondition() {
  this->conditions.pop_back();
}

void Rule::removeAllConditions() {
  this->conditions.clear();
}

unsigned Rule::cover(const std::list<Instance>& instances) const
{
  unsigned count = 0;

  // an emptry rule covers all instances
  if (this->conditions.empty())
    return instances.size();

  for (const auto& instance: instances) {
    // instance is covered if all conditions applied on this instance return true
    // attributes not present in the list of conditions are ignored

    bool covers = false;

    // TOOD: consider iterating over the single instance version of cover
    for (const auto& condition: conditions) {
      for (const auto& attr: instance.attributes) {
        if (attr.name != condition.attr_name)
          continue;

        covers = condition.apply(attr.value);
        break;
      }
      if (!covers)
        break;
    }
    count += covers;
  }

  return count;
}

void Rule::copy(const Rule& anotherRule) {
  this->conditions.clear();
  this->conditions = anotherRule.conditions;
}

unsigned Rule::cover(const Instance& instance) const {
  // instance is covered if all conditions applied on this instance return true
  // attributes not present in the list of conditions are ignored

  for (const auto& condition: conditions) {
    for (const auto& attr: instance.attributes) {
      if (attr.name != condition.attr_name)
        continue;

      if (!condition.apply(attr.value))
        return false;
    }
  }

  return true;
}

bool Condition::apply(const AttributeValue attr_value) const
{
  switch (this->cond_operator) {
    case EQ:
      return attr_value == this->attr_value;
    case LESS_EQ:
      return attr_value <= this->attr_value;
    case MORE_EQ:
      return attr_value >= this->attr_value;
    default:
      // throw something
      return false;
  }
  return false;
}
