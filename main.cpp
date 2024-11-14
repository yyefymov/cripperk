#include <iostream>
#include <filesystem>
#include <map>
#include <string>
#include "header/ripperk.h"

int main(int argc, char* argv[])
{
    // store the executionable path. might be useful
    std::filesystem::path exe_path = argv[0];
    exe_path.remove_filename();

    if (argc < 2) {
        std::cerr << "No arguments provided" << std::endl;
        return 1;
    }

    // create a map of flags
    // pars the arguments, save the flag values separated by whitespace
    // check for the mandatory arguments
    // use std::filesystem::path to store paths

    std::map< std::string, std::vector<std::string> > params;
    std::string last_parameter;
    for (size_t i = 1; i < argc; ++i) {
        std::string_view sv{argv[i], 2};
        if (sv == "--") {
            last_parameter = sv.data();
            params[last_parameter] = std::vector<std::string>{};
        } else {
            if (last_parameter.empty()) {
                std::cerr << "Please provide a parameter" << std::endl;
                return 1;
            }

            params[last_parameter].emplace_back(argv[i]);
        }
    }

    // output help if requested. Non-mandatory
    // only output help if it is the only parameter
    // do not process further
    if (params.size() == 1 && params.find("--help") != params.end()) {
        // TODO: specific help for each argument?

        std::cout << "--mode:" << std::endl;
            std::cout << "\tlearn - train and output the model. Paths to the dataset CSV and the model output file are requred" << std::endl;
            std::cout << "\tevaluate - check the accuracy of the model. Paths to the model and the test dataset CSV are required" << std::endl;
            std::cout << "\tclassify - classify a dataset. Paths to the model and the dataset CSV are required" << std::endl;

        std::cout << "--dataset - path to the CSV file holding the data instances. Should be formatted appropriately" << std::endl;
        std::cout << "--model - path to the binary file storing the model. The model will be created in the learn mode; evaluate and classify modes require the existing and valid model file" << std::endl;
        std::cout << "--model-txt - path to the text file holding the model in the human-readable format. Non-mandatory" << std::endl;
        std::cout << "--ratio - ratio of grow to prune dataset. Non-mandatory. Default is 2/3" << std::endl;
        std::cout << "--k - number of times the optimization is performed. Non-mandatory. Default is 2" << std::endl;

        return 0;
    }

    // validate and save mode
    if (params.find("--mode") == params.end()) {
        std::cerr << "Mandatory parameter mode is missing" << std::endl;
        return 1;
    }
    if (params["--mode"].empty()) {
        std::cerr << "No mode is provided" << std::endl;
        return 1;
    }
    std::string mode = params["--mode"][0];
    if ((mode != "learn") && (mode != "evaluate") && (mode != "classify")) {
        std::cerr << "Incorrect mode " << mode << " is provided" << std::endl;
        return 1;
    }

    // validate and save path to dataset
    if (params.find("--dataset") == params.end()) {
        std::cerr << "Mandatory parameter dataset is missing" << std::endl;
        return 1;
    }
    if (params["--dataset"].empty()) {
        std::cerr << "No dataset is provided" << std::endl;
        return 1;
    }
    std::filesystem::path path_to_dataset = params["--dataset"][0];
    if (path_to_dataset.is_relative())
        path_to_dataset = exe_path.generic_string() + path_to_dataset.generic_string();

    // validate and save path to model bin
    if (params.find("--model") == params.end()) {
        std::cerr << "Mandatory parameter model is missing" << std::endl;
        return 1;
    }
    if (params["--model"].empty()) {
        std::cerr << "No model is provided" << std::endl;
        return 1;
    }
    std::filesystem::path path_to_model_bin = params["--model"][0];
    if (path_to_model_bin.is_relative())
        path_to_model_bin = exe_path.generic_string() + path_to_model_bin.generic_string();

    // validate and save path to model txt. Non-mandatory
    std::filesystem::path path_to_model_txt = "";
    if (params.find("--model-txt") == params.end() || params["--model-txt"].empty()) {
        std::cout << "Path to the human-readable model is not provided." << std::endl;
        std::cout << "If you wish to generate a human-readable model, please provide the valid path with the --model-txt parameter." << std::endl;
        std::cout << std::endl;
    } else {
        path_to_model_txt = params["--model-txt"][0];
        if (path_to_model_txt.is_relative())
            path_to_model_txt = exe_path.generic_string() + path_to_model_txt.generic_string();
    }

    // validate and save pruning ratio. Non-mandatory
    float pruning_ratio = 2/(float)3;
    if (params.find("--ratio") == params.end() || params["--ratio"].empty()) {
        std::cout << "Using default pruning ratio of 2/3" << std::endl;
        std::cout << "If you wish to use a different ratio, provide the value with the --ratio parameter" << std::endl;
        std::cout << std::endl;
    } else {
        size_t pos = 0;
        pruning_ratio = std::stof(params.at("--ratio")[0], &pos);
    }

    // validate and save k. Non-mandatory
    int k = 2;
    if (params.find("--k") == params.end() || params["--k"].empty()) {
        std::cout << "Using default k of 2" << std::endl;
        std::cout << "If you wish to use a different k, provide the value with the --k parameter" << std::endl;
        std::cout << std::endl;
    } else {
        size_t pos = 0;
        k = std::stoi(params.at("--k")[0], &pos);
    }

    auto ripperk = RIPPERk(path_to_dataset.generic_string(), path_to_model_txt.generic_string(), path_to_model_bin.generic_string(), pruning_ratio, k);

    if (mode == "learn")
        ripperk.fit();
    else if (mode == "evaluate")
        ripperk.evaluate();
    else
        ripperk.classify();

    return 0;
}