#ifndef RULE_H
#define RULE_H

// TODO: consider pimpl
#include <string>
#include <vector>
#include <memory>

#include "dataset.h"

enum ConditionOperator {
  EQ,       // ==
  LESS_EQ,  // <=
  MORE_EQ   // >=
};

struct Condition {
  ConditionOperator cond_operator;
  std::string attr_name;
  AttributeValue attr_value;

  bool apply(const AttributeValue attribute_value) const;
};

class Rule {
public:
  Rule();
  Rule(std::shared_ptr<const AttributeManager> attribute_manager);
  Rule(const Rule& rule);
  Rule& operator=(const Rule& rhs);

  void grow(std::list<Instance> pos, std::list<Instance> neg);
  void prune(std::list<Instance> pos, std::list<Instance> neg);
  void addCondition(const Condition& condition);
  void removeLastCondition();
  void removeAllConditions();
  void copy(const Rule& anotherRule);
  unsigned cover(const std::list<Instance>& instances) const;
  unsigned cover(const Instance& instance) const;
  float dl() const;
  float dl_err(const std::list<Instance>& pos, const std::list<Instance>& neg) const;
  std::string toString() const;
  bool empty() const;
  void write_bin(std::ofstream& model_bin) const;
  void read_bin(std::ifstream& model_bin);
private:
  std::vector<Condition> conditions;
  // const AttributeManager& attribute_manager;
  std::shared_ptr<const AttributeManager> attribute_manager;

  float foil_gain(const Condition& condition, const std::list<Instance>& pos, const std::list<Instance>& neg) const;
};

class Ruleset {
public:
  struct RuleHandle {
    unsigned id;
  };

  RuleHandle addRule(Rule rule); // accept unique_ptr to move into vector? accept rvalue somehow and move to rules?
  const Rule& getRule(RuleHandle handle) const;
  void replaceRule(RuleHandle handle, const Rule& rule);
  std::vector<RuleHandle> get() const;
  unsigned size() const;
  float dl(std::list<Instance> pos, std::list<Instance> neg) const;
  std::string toString() const;
  void pruneRule(RuleHandle handle, std::list<Instance> pos, std::list<Instance> neg);
  bool cover(const Instance& instance);
  void simplify(std::list<Instance> pos, std::list<Instance> neg);

private:
  std::vector<Rule> rules; // vector of unique_ptrs ? this must be the ONLY place where the rules are stored
};

#endif