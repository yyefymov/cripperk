#ifndef RIPPERK_H
#define RIPPERK_H

#include <string>
#include <list>
#include <memory>

#include "../internal/header/dataset.h"
#include "../internal/header/rule.h"

class RIPPERk {
public:
  RIPPERk(const std::string& path_to_dataset,
          const std::string& path_to_model_txt,
          const std::string& path_to_model_bin,
          float pruning_ratio=2/(float)3,
          int k=2);

  void fit(); // throw if dataset is missing
  void evaluate(); // throw if dataset or model is missing
  void classify();

private:
  // TODO: pimpl (consider during the refactoring stage)
  std::string path_to_dataset;
  std::string path_to_model_txt;
  std::string path_to_model_bin;

  std::list<Instance> dataset;
  std::shared_ptr<const AttributeManager> attr_manager;
  float pruning_ratio;
  int k;

  Ruleset IREP(std::list<Instance> pos, std::list<Instance> neg);
  void optimize(Ruleset& ruleset, std::list<Instance> pos, std::list<Instance> neg); // move to Ruleset?
  void produceDataset();
};

#endif