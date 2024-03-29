//
// Created by mac on 2022/12/24.
//

#ifndef MADPODRACING_ANN_H
#define MADPODRACING_ANN_H

#include <array>
#include <memory>
#include <random>
#include <cmath>
#include <fstream>

template<int Layers>
class ANN {
    using carr = std::unique_ptr<float[]>;
public:
    typedef std::shared_ptr<ANN<Layers>> Pointer;
    constexpr static const int LayersCount = Layers;
    constexpr static const float WeightPower = 1.0f, BiasPower = 1.0f;
    std::array<int, Layers + 1> NeuronOffset;
    std::array<int, Layers> WeightOffset;
    carr Weights;
    carr Neurons;
    carr Biases;
    int WeightCount = 0, NeuronCount = 0, BiasCount = 0;
    std::array<int, Layers> Nodes;
    constexpr static const std::array<int, Layers> DefaultNodes = {18, 31, 7, 2};
private:
    std::random_device RandomDevice;
    std::minstd_rand RNG;
    std::uniform_real_distribution<float> Distribution;
    std::uniform_real_distribution<float> Distribution01;
    std::uniform_int_distribution<int> WeightDistribution, BiasDistribution;
public:
    ANN() : RNG(RandomDevice()), Distribution(-1.0, 1.0), Distribution01(0.0, 1.0) {}

    ANN(const ANN& ann) {
        Nodes = ann.Nodes;
        RandomDevice = ann.RandomDevice;
        RNG = ann.RNG;
        Distribution = ann.Distribution;
        Distribution01 = ann.Distribution01;
        WeightDistribution = ann.WeightDistribution;
        BiasDistribution = ann.BiasDistribution;
        std::memcpy(Weights, ann.Weights, WeightCount);
        std::memcpy(Neurons, ann.Neurons, NeuronCount);
        std::memcpy(Biases, ann.Biases, BiasCount);
    }

    void InitializeSpace(std::array<int, Layers> nodes) {
        Nodes = nodes;
        InitializeSpace();
    }

    void InitializeSpace() {
        NeuronOffset[0] = WeightOffset[0] = 0;
        for (int i = 0; i < Layers; i++) {
            NeuronCount += Nodes[i];
            NeuronOffset[i + 1] = NeuronCount;
            if (i != Layers - 1) {
                WeightCount += Nodes[i] * Nodes[i + 1];
                WeightOffset[i + 1] = WeightCount;
            }
        }
        BiasCount = NeuronCount - NeuronOffset[1];
        Weights.reset(new float[WeightCount]);
        Neurons.reset(new float[NeuronCount]);
        Biases.reset(new float[BiasCount]);
        WeightDistribution = std::uniform_int_distribution(0, WeightCount - 1);
        BiasDistribution = std::uniform_int_distribution(0, BiasCount - 1);
    }

    void Compute() {
        for (int i = 0; i < Layers - 1; i++) {
            int weightOffset = WeightOffset[i];
            for (int j = NeuronOffset[i + 1]; j < NeuronOffset[i + 2]; j++) {
                Neurons[j] = 0;
                for (int k = NeuronOffset[i]; k < NeuronOffset[i + 1]; k++) {
                    Neurons[j] += Neurons[k] * Weights[weightOffset];
                    weightOffset++;
                }
                Neurons[j] += Biases[j - NeuronOffset[1]];
                Neurons[j] = tanh(Neurons[j]);;
            }
        }
    };

    void Randomize() {
        for (int i = 0; i < WeightCount; i++) {
            Weights[i] = Distribution(RNG) * WeightPower;
        }
        for (int i = 0; i < BiasCount; i++) {
            Biases[i] = Distribution(RNG) * BiasPower;
        }
    };

    void Mate(const ANN<Layers>& another, const ANN<Layers>& to, float mut_p) {
        Crossover(another, to);
        Mutate(to, mut_p);
    }

    void Crossover(const ANN<Layers>& another, const ANN<Layers>& to) {
        Crossover(Weights, another.Weights, to.Weights, WeightDistribution);
        Crossover(Biases, another.Biases, to.Biases, BiasDistribution);
    }

    void Crossover(const carr& a1, const carr& a2, const carr& to, std::uniform_int_distribution<int>& intDistrib) {
        auto idx = intDistrib(RNG);
        std::memcpy(to.get(), a1.get(), idx);
        std::memcpy(to.get() + idx, a2.get() + idx, intDistrib.b() + 1 - idx);
    }

    void Mutate(const ANN<Layers>& to, float probability) {
        Mutate(Weights, to.Weights, WeightCount, probability, WeightDistribution, WeightPower);
        Mutate(Biases, to.Biases, BiasCount, probability, BiasDistribution, BiasPower);
    }

    void Mutate(const carr& a, const carr& to, int len, float probability, std::uniform_int_distribution<int>& distrib,
                float power) {
        std::memcpy(to.get(), a.get(), sizeof(float) * len);
        int count = (int) (len * probability);
        for (int i = 0; i < count; i++) {
            auto index = distrib(RNG);
            auto val = Distribution(RNG) * power;
            to[index] = val;
        }
    }

    void Write(std::ostream& os) {
        os.write((const char *) Weights.get(), sizeof(float) * WeightCount);
        os.write((const char *) Biases.get(), sizeof(float) * BiasCount);
    }

    void WritePlain(std::ostream& os) {
        for (int i = 0; i < WeightCount; i++) {
            os << Weights[i] << " ";
        }
        for (int i = 0; i < BiasCount; i++) {
            os << Biases[i] << " ";
        }
    }

    void WriteCode(std::ostream& os) {
        os << "float weights[] = {";
        for (int i = 0; i < WeightCount; i++) {
            os << Weights[i];
            if (i != WeightCount - 1) os << ", ";
        }
        os << "};\nfloat biases[] = {";
        for (int i = 0; i < BiasCount; i++) {
            os << Biases[i];
            if (i != BiasCount - 1) os << ", ";
        }
        os << "};\n";
    }

    bool WriteCode(const std::string& path) {
        std::ofstream os(path);
        WriteCode(os);
        os.close();
        return os.good();
    }

    void ReadPlain(std::istream& is) {
        for (int i = 0; i < WeightCount; i++) {
            is >> Weights[i];
        }
        for (int i = 0; i < BiasCount; i++) {
            is >> Biases[i];
        }
    }

    void Read(std::istream& is) {
        is.read((char *) Weights.get(), sizeof(float) * WeightCount);
        is.read((char *) Biases.get(), sizeof(float) * BiasCount);
    }
};

typedef ANN<4> ANNUsed;

#endif //MADPODRACING_ANN_H
