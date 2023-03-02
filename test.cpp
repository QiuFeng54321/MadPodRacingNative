//
// Created by mac on 2023/2/1.
//

#include "GameSimulator.h"

bool sig_caught = false;

int main() {
    GA ga{};
    ga.Initialize();
    auto start = high_resolution_clock::now();
    const std::string SaveName = "test0302";
    const auto SaveBinPath = SaveName + ".bin";
    const auto SaveWeightsPath = SaveName + ".weights.txt";
    const auto SaveCodePath = SaveName + ".cpp";
    std::cout << ga.Load(SaveBinPath) << std::endl;
//    ga.ANNs[0]->WriteCode("out.cpp");
    while (true) {
        ga.Generation();
        auto end = high_resolution_clock::now();
        auto dur = duration_cast<minutes>(end - start);
        if (dur.count() > 2) break;
    }
    ga.Save(SaveBinPath);
    ga.SavePlain(SaveWeightsPath);
    ga.ANNs[0]->WriteCode(SaveCodePath);
    return 0;
}