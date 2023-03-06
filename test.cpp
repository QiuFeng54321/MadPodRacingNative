//
// Created by mac on 2023/2/1.
//

#include "GameSimulator.h"

bool sig_caught = false;

int main() {
    GA ga{};
    ga.Initialize();
    auto start = high_resolution_clock::now();
    const std::string SaveName = "test0305";
    const auto SaveBinPath = SaveName + ".bin";
    const auto SaveWeightsPath = SaveName + ".weights.txt";
    const auto SaveCodePath = SaveName + ".cpp";
    std::cout << ga.Load(SaveBinPath) << std::endl;
//    ga.ANNs[0]->WriteCode("out.cpp");
    while (true) {
        ga.Generation();
        auto end = high_resolution_clock::now();
        auto durHrs = duration_cast<hours>(end - start).count();
        auto totalMin = duration_cast<minutes>(end - start).count();
        auto durMin = totalMin - durHrs * 60;
        auto durSec = duration_cast<seconds>(end - start).count() - durHrs * 3600 - durMin * 60;
        std::cout << "Time total: " << durHrs << " hr " << durMin << " min " << durSec << " s" << std::endl;
        if (totalMin >= 30) break;
    }
    ga.Save(SaveBinPath);
    ga.SavePlain(SaveWeightsPath);
    ga.ANNs[0]->WriteCode(SaveCodePath);
    return 0;
}