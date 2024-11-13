#include "../../header/ripperk.h"
#include "../header/rule.h"
#include "../header/mathutils.h"
#include "../header/model.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <memory>

const int bit_len_treshold = 64;

float baseline_dl(const std::list<Instance> dataset, const std::string& default_class) {
  // assign all instances to teh default class
  // use the DL_err formula. No positive or false positive entries will be covered, so the formula is simplified.
  unsigned n = dataset.size();
  unsigned fn = 0;

  std::for_each(dataset.begin(), dataset.end(), [&fn, &default_class](const auto& instance){fn += (instance.class_value != default_class);});

  // return std::log2((float)factorial(n) / ((float)factorial(fn) * (float)factorial(n-fn)));
  auto result = MathUtils::log2_combination(n, fn);
  return result;
}

Ruleset RIPPERk::IREP(std::list<Instance> pos, std::list<Instance> neg) { // pass by ref
  auto ruleset = Ruleset();
  // copy pos and neg - needed to calculate the DL
  auto pos_copy = pos;
  auto neg_copy = neg;
  float min_dl = std::max(baseline_dl(this->dataset, this->dataset.rbegin()->class_value), 0.0f);

  while (!pos.empty()) {
    auto rule = Rule(this->attr_manager);
    unsigned split_index_pos = std::floor(pos.size() * this->pruning_ratio);
    unsigned split_index_neg = std::floor(neg.size() * this->pruning_ratio);
    std::list<Instance> grow_pos(pos.begin(), std::next(pos.begin(), split_index_pos + 1));
    std::list<Instance> prune_pos(std::next(pos.begin(), split_index_pos + 1), pos.end());
    std::list<Instance> grow_neg(neg.begin(), std::next(neg.begin(), split_index_neg + 1));
    std::list<Instance> prune_neg(std::next(neg.begin(), split_index_neg + 1), neg.end());

    rule.grow(grow_pos, grow_neg);
    rule.prune(prune_pos, prune_neg);

    // stop adding rules if the grown and pruned rule is empty
    // otherwise, since empry rule has to be discarded (it adds no value), there will be an empty loop,
    // because no instances will be covered and removed and the DL of the ruleset will not increase
    if (rule.empty())
      return ruleset;

    ruleset.addRule(rule);

    pos.remove_if([&rule](const Instance& instance){return rule.cover(instance);});
    neg.remove_if([&rule](const Instance& instance){return rule.cover(instance);});

    // simplify the ruleset

    // check MDL of the ruleset
    // pass copied pos and neg sets to dl in order to calculate the error dl
    auto dl = ruleset.dl(pos_copy, neg_copy);
    if (dl > min_dl + bit_len_treshold) {
      return ruleset;
    }

    min_dl = std::min(min_dl, dl);
  }

  return ruleset;
}

void RIPPERk::optimize(Ruleset& ruleset, std::list<Instance> pos, std::list<Instance> neg) {
  unsigned split_index_pos = std::floor(pos.size() * this->pruning_ratio);
  unsigned split_index_neg = std::floor(neg.size() * this->pruning_ratio);
  std::list<Instance> grow_pos(pos.begin(), std::next(pos.begin(), split_index_pos + 1));
  std::list<Instance> prune_pos(std::next(pos.begin(), split_index_pos + 1), pos.end());
  std::list<Instance> grow_neg(neg.begin(), std::next(neg.begin(), split_index_neg + 1));
  std::list<Instance> prune_neg(std::next(neg.begin(), split_index_neg + 1), neg.end());
  // iterate through each rule (in order)
  //   construct a replacement rule - grown from scratch
  //     the replacement rule has to be pruned too, "pruning is guided so as to minimize error of the entire rule set R Ri Rk on the pruning data". whatever that means...
  //   construct a revision rule - conditions are added to the current rule
  //   save the current rule too
  //   calculate the DL of the three versions of the ruleset: with the original rule, with the replacement rule and with the revision rule
  //   keep the one rule that gives the smallest DL when inserted in the ruleset
  auto rule_handles = ruleset.get();
  for (const auto& rule_handle: rule_handles) {
    Rule original(ruleset.getRule(rule_handle));
    Rule replacement(original);
    Rule revision(original);
    Rule* final_rule = &original;

    // calculate the DL with the original rule
    float min_dl = ruleset.dl(pos, neg);

    // grow a replacement rule
    replacement.removeAllConditions();
    replacement.grow(grow_pos, grow_neg);

    // replace the original rule with the replacement rule
    ruleset.replaceRule(rule_handle, replacement);

    // prune the rule with the relation to the whole ruleset
    ruleset.pruneRule(rule_handle, prune_pos, prune_neg);

    // calculate the DL
    float replacement_dl = ruleset.dl(pos, neg);
    if (replacement_dl < min_dl) {
      min_dl = replacement_dl;
      final_rule = &replacement;
    }

    // grow and prune a revision rule
    revision.grow(grow_pos, grow_neg);
    revision.prune(prune_pos, prune_neg);

    // replace the rule with the revision rule
    ruleset.replaceRule(rule_handle, revision);

    //calculate the DL
    float revision_dl = ruleset.dl(pos, neg);
    if (revision_dl < min_dl) {
      final_rule = &revision;
    }

    // keep the rule with the smallest DL out of three in the ruleset permanently
    ruleset.replaceRule(rule_handle, *final_rule);
  }
}

void RIPPERk::produceDataset() { // create class named Utils that takes a RIPPERk object, move this function there
  std::ifstream input(this->path_to_dataset);
  std::vector<std::string> keys;

  for (std::string line; std::getline(input, line);) {
    std::istringstream ss(std::move(line));

    if (keys.empty()) {
      for (std::string value; std::getline(ss, value, ',');)
        keys.emplace_back(std::move(value));
    } else {
      auto i = keys.begin();
      Instance instance{};

      for (std::string value; std::getline(ss, value, ',');) {
        Attribute attribute{};
        size_t pos = 0;
        attribute.name = *i;
        if (value.empty()) {
          i = std::next(i);
          continue;
        }
        try {
          attribute.value = std::stof(value, &pos);
          attribute.type = CONTINUOUS;
        } catch (const std::invalid_argument&) {
          // ????
        }

        if (pos != value.size()) {
          attribute.value = value;
          attribute.type = DISCRETE;
        }

        instance.attributes.emplace_back(attribute);
        i = std::next(i);
      }

      try {
        instance.class_value = std::get<std::string>(instance.attributes.back().value);
      } catch (const std::exception&){
        instance.class_value = std::get<float>(instance.attributes.back().value);
      }
      instance.attributes.pop_back(); // remove the class from the list of attributes
      this->dataset.push_back(instance);
    }
  }
}

RIPPERk::RIPPERk(const std::string &path_to_dataset, const std::string &path_to_model_txt, const std::string &path_to_model_bin, float pruning_ratio, int k)
  : path_to_dataset(path_to_dataset)
  , path_to_model_txt(path_to_model_txt)
  , path_to_model_bin(path_to_model_bin)
  , pruning_ratio(pruning_ratio)
  , k(k)
  , attr_manager(nullptr)
{
  produceDataset();
  this->attr_manager = std::make_shared<const AttributeManager>(this->dataset);
}

void RIPPERk::fit()
{
  Model model(this->attr_manager);
  std::map<std::string, unsigned> class_count;
  std::map<std::string, unsigned> class_order;
  unsigned order = 1;
  auto max_prevalence = class_count.end();

  for (const auto& instance: this->dataset)
    class_count[instance.class_value]++;

  while (!class_count.empty()) {
    max_prevalence = std::max_element(class_count.begin(), class_count.end(),[](const auto& kv1, const auto& kv2){return kv1.second < kv2.second;});
    class_order[max_prevalence->first] = order++;
    class_count.erase(max_prevalence);
  }
  std::string default_class_name = std::max_element(class_order.begin(), class_order.end(),[](const auto& kv1, const auto& kv2){return kv1.second < kv2.second;})->first;
  model.setDefaultClass(default_class_name);

  // iterate from the most prevalent to the least prevalent class
  //   pos = all isntances classified as the current class
  //   neg = all instances classified as classes after the current class
  auto order_copy = class_order;
  while (!class_order.empty()) {
    std::list<Instance> pos;
    std::list<Instance> neg;
    auto max_class_it = std::min_element(class_order.begin(), class_order.end(), [](const auto& kv1, const auto& kv2){return kv1.second < kv2.second;});
    // max_class_it should never be 0
    std::string pos_class = max_class_it->first;

    // last class is the default class
    if (class_order.size() > 1) {
      for (const auto& instance: this->dataset) {
        if (instance.class_value == pos_class)
          pos.push_back(instance);
        else if (class_order.find(instance.class_value) != class_order.end())
          neg.push_back(instance);
      }

      model.add(pos_class, std::move(IREP(pos, neg)));

      // optimize k times
      int k = this->k;
      while (k--)
        optimize(model.get(pos_class), pos, neg);
    }

    class_order.erase(max_class_it);
  }

  model.write(this->path_to_model_txt, this->path_to_model_bin);
}

void RIPPERk::evaluate()
{
  Model model(this->attr_manager);
  std::string derived_class;
  unsigned match = 0;
  unsigned mismatch = 0;

  model.read(this->path_to_model_bin);

  // apply the model to the dataset
  // compare the derived class to the one present in the instance
  for (const auto& instance: this->dataset) {
    derived_class.clear();

    for (const auto& class_name: model.getClassOrder()) {
      if (model.get(class_name).cover(instance)) {
        derived_class = class_name;
        break;
      }
    }
    if (derived_class.empty()) {
      derived_class = model.getDefaultClass();
    }

    if (derived_class == instance.class_value)
      ++match;
    else
      ++mismatch;
  }

  std::cout << "Analyzed " << this->dataset.size() << " entries" << std::endl;
  std::cout << "Correctly predicted classes: " << match << std::endl;
  std::cout << "Incorrectly predicted classes: " << mismatch << std::endl;
  std::cout << "Success rate: " << ((float)match / (float)this->dataset.size()) * 100 << "%" << std::endl;
}


void RIPPERk::classify() {
  Model model(this->attr_manager);
  std::string derived_class;
  size_t i = 0;

  model.read(this->path_to_model_bin);

  for (const auto& instance: this->dataset) {
    derived_class.clear();
    for (const auto& class_name: model.getClassOrder()) {
      if (model.get(class_name).cover(instance)) {
        derived_class = class_name;
        break;
      }
    }
    if (derived_class.empty()) {
      derived_class = model.getDefaultClass();
    }

    std::cout << "Instance " << i++ << " assigned to class " << derived_class << std::endl;
  }
}