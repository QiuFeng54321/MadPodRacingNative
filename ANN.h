//
// Created by mac on 2022/12/24.
//

#ifndef MADPODRACING_ANN_H
#define MADPODRACING_ANN_H

#include <array>
#include <memory>
#include <random>
#include <cmath>

template<int Layers>
class ANN {
    using carr = std::unique_ptr<float[]>;
public:
    std::array<int, Layers + 1> NeuronOffset;
    std::array<int, Layers> WeightOffset;
    carr Weights;
    carr Neurons;
    carr Biases;
    int WeightCount = 0, NeuronCount = 0, BiasCount = 0;
    std::array<int, Layers> Nodes;
private:
    std::random_device RandomDevice;
    std::mt19937 RNG;
    std::uniform_real_distribution<float> Distribution;
    std::uniform_real_distribution<float> Distribution01;
public:
    ANN() : RNG(RandomDevice()), Distribution(-1.0, 1.0), Distribution01(0.0, 1.0) {}

    ANN(const ANN& ann) {
        Nodes = ann.Nodes;
        RandomDevice = ann.RandomDevice;
        RNG = ann.RNG;
        Distribution = ann.Distribution;
        Distribution01 = ann.Distribution01;
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
                Neurons[j] = tanh(Neurons[j]);
            }
        }
    };

    void Randomize() {
        for (int i = 0; i < WeightCount; i++) {
            Weights[i] = Distribution(RNG);
        }
        for (int i = 0; i < BiasCount; i++) {
            Biases[i] = Distribution(RNG);
        }
    };

    void Crossover(const ANN<Layers>& another, const ANN<Layers>& to, float probability) {
        Crossover(Weights, another.Weights, to.Weights, WeightCount, probability);
        Crossover(Biases, another.Biases, to.Biases, BiasCount, probability);
    }

    void Crossover(const carr& a1, const carr& a2, const carr& to, int len, float probability) {
        for (int i = 0; i < len; i++) {
            if (Distribution01(RNG) <= probability) {
                to[i] = a2[i];
            } else {
                to[i] = a1[i];
            }
        }
    }

    void Mutate(const ANN<Layers>& to, float probability) {
        Mutate(Weights, to.Weights, WeightCount, probability);
        Mutate(Biases, to.Biases, BiasCount, probability);
    }

    void Mutate(const carr& a, const carr& to, int len, float probability) {
        for (int i = 0; i < len; i++) {
            if (Distribution01(RNG) <= probability) {
                to[i] = Distribution(RNG);
            } else {
                to[i] = a[i];
            }
        }
    }
};

typedef ANN<4> ANNUsed;

#endif //MADPODRACING_ANN_H