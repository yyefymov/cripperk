#include "../header/rule.h"
#include <numeric>
#include <stdexcept>
#include <algorithm>
#include <limits>

Ruleset::RuleHandle Ruleset::addRule(Rule rule)
{
  RuleHandle handle;

  handle.id = rules.size();
  rules.push_back(rule);

  return handle;
}


const Rule &Ruleset::getRule(Ruleset::RuleHandle handle) const
{
  if (handle.id >= this->rules.size())
    ; // throw something

  return rules[handle.id];
}

void Ruleset::replaceRule(RuleHandle handle, const Rule& rule) {
  this->rules[handle.id].copy(rule);
}

float Ruleset::dl(std::list<Instance> pos, std::list<Instance> neg) const
{
  float dl_sum = 0.0f;

  for (const auto& rule: this->rules) {
    dl_sum += rule.dl() + rule.dl_err(pos, neg);

    pos.remove_if([&rule](const Instance& instance){return rule.cover(instance);});
    neg.remove_if([&rule](const Instance& instance){return rule.cover(instance);});
  }

  return dl_sum;
}

std::vector<Ruleset::RuleHandle> Ruleset::get() const {
  std::vector<RuleHandle> result;
  for (size_t i = 0; i < this->rules.size(); ++i) {
    RuleHandle handle;
    handle.id = i;
    result.emplace_back(std::move(handle));
  }

  return result;
}

unsigned Ruleset::size() const {
  return this->rules.size();
}

std::string Ruleset::toString() const
{
  std::string rules_str;
  for (auto cit = this->rules.cbegin(); cit != this->rules.cend(); std::advance(cit, 1)) {
    auto rule = *cit;
    rules_str.append(rule.toString());
    if (cit != this->rules.cend() - 1)
      rules_str.append(" OR");
    rules_str.append("\n");
  }
  return rules_str;
}

void Ruleset::pruneRule(RuleHandle handle, std::list<Instance> pos, std::list<Instance> neg) {
  // TODO: there is a lot of copying in this function, please optimize it

  if (handle.id >= this->rules.size())
    throw std::runtime_error("Rule is not present in the rule set");

  Rule rule(this->rules[handle.id]);
  float min_metric = std::numeric_limits<float>::max();
  unsigned conditions_removed = 0;
  unsigned conditions_to_remove = 0;

  while (!this->rules[handle.id].empty()) {
    float dl_err = 0.0f;
    for (const auto& rule: this->rules) {
      dl_err += rule.dl_err(pos, neg);

      pos.remove_if([&rule](const Instance& instance){return rule.cover(instance);});
      neg.remove_if([&rule](const Instance& instance){return rule.cover(instance);});
    }
    if (dl_err < min_metric) {
      min_metric = dl_err;
      conditions_to_remove = conditions_removed;
    }

    this->rules[handle.id].removeLastCondition();
    ++conditions_removed;
  }

  // swap the temporary with the original rule
  this->rules[handle.id].copy(rule);

  // prune
  for (size_t i = 0; i < conditions_to_remove; ++i)
    this->rules[handle.id].removeLastCondition();
}

bool Ruleset::cover(const Instance& instance) {
  for (const auto& rule: this->rules) {
    if (rule.cover(instance))
      return true;
  }

  return false;
}

void Ruleset::simplify(std::list<Instance> pos, std::list<Instance> neg) {
  if (this->rules.size() <= 1)
    return;

  // remove redundant rules
  // for example:
  //   IF petal_length <= 1 OR
  //   IF petal_length <= 1.100000
  // rule 1 is completely unnecessary
  // this may be handled by checking which rule adds to the overall coverage

  // (consider) if the ruleset covers more instances (or has less error?) after removing a rule - then remove this rule

  
}