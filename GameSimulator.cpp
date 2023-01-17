//
// Created by mac on 2022/12/25.
//

#include "GameSimulator.h"

#include <utility>

GameSimulator::GameSimulator(int podsPerSide, int totalLaps = 3) : PodsPerSide(podsPerSide), TotalLaps(totalLaps) {

}

void GameSimulator::Setup(std::vector<Vec> checkpoints) {
    Checkpoints = std::move(checkpoints);
    Pods.reset(new Pod[PodsPerSide * 2]);
    for (int i = 0; i < PodsPerSide * 2; i++) {
        Vec pos = Checkpoints[0] + UnitRight.Rotate(M_PI / PodsPerSide * i) * Pod::Diameter * 1.1;
        Pods[i] = Pod{
                pos,
                pos,
                {0, 0},
                0,
                {0, 0},
                100,
                false,
                false,
                0,
                i < PodsPerSide,
                1,
                1,
                0};
    }
}

void GameSimulator::UpdatePodAI(int podIndex) {
    int currentNeuron = 0;
    Pod& currentPod = Pods[podIndex];
    for (int i = 0; i < PodsPerSide * 2; i++) {
        // If enemy, start filling enemy data first
        int j = podIndex < PodsPerSide ? i : (i + PodsPerSide) % (PodsPerSide * 2);
        if (j == podIndex) continue;
        currentPod.Encode().Write(*ANNController, currentNeuron);
    }
    ANNController->Compute();
    float angle = ANNController->Neurons[ANNController->NeuronCount - 2];
    float thrust = ANNController->Neurons[ANNController->NeuronCount - 1];
    angle = angle * DegToRad(20); // Scale to [-20..20]
    bool boost = 0.5 <= thrust && thrust < 0;
    bool shield = thrust < 0.5;
    if (currentPod.ShieldCD == 0) {
        if (shield) {
            currentPod.ShieldCD = 3;
            currentPod.Mass = Pod::ShieldMass;
        } else if (boost && !currentPod.Boosted) {
            currentPod.Boost = currentPod.Boosted = true;
        }
    }
    currentPod.Thrust = thrust;
    currentPod.TargetPosition = currentPod.Position + UnitRight.Rotate(angle) * 1000;
}

void Pod::UpdateVelocity() {
    // Update position
    auto idealDirection = TargetPosition - Position;
    auto rotateDegree = Bound<double>(ClampRadian(idealDirection.Angle() - Facing), DegToRad(-18), DegToRad(18));
    auto thrust = Thrust;
    if (Boost) {
        thrust = Boosted ? 100 : 650;
        Boosted = true;
    }
    if (ShieldCD) {
        thrust = 0;
        ShieldCD--;
    }
    Mass = ShieldCD ? Pod::ShieldMass : Pod::NormalMass;
    Facing += rotateDegree;
    Velocity += UnitRight.Rotate(Facing) * thrust;
}


double Pod::CheckCollision(const Pod& other) const {
    return CollisionTime(Velocity, other.Velocity, Position, other.Position, Radius, Radius);
}

void GameSimulator::MoveAndCollide() {
    const double maxTime = 1e9;
    double minNextCldTime = maxTime;
    int mctI, mctJ; // if min exists, the two collided object index
    int lastI = -1, lastJ = -1;
    double remainTime = 1;
    // O(N^2?)
    while (remainTime > 0) {
        for (int i = 0; i < PodsPerSide * 2; i++) {
            auto& pod = Pods[i];
            if (!pod.IsEnabled()) continue;
            for (int j = i + 1; j < PodsPerSide * 2; j++) {
                auto& anotherPod = Pods[j];
                if (!pod.IsEnabled()) continue;
                if (i == lastI && j == lastJ) continue;
                auto collideTime = pod.CheckCollision(anotherPod);
                if (collideTime <= 0 || collideTime > remainTime) continue;
                if (collideTime < minNextCldTime) {
                    mctI = i;
                    mctJ = j;
                    minNextCldTime = collideTime;
                }
            }
        }
        if (minNextCldTime > remainTime) { // No more collisions
            for (int i = 0; i < PodsPerSide * 2; i++) {
                auto& pod = Pods[i];
                if (!pod.IsEnabled()) continue;
                pod.Position += pod.Velocity * remainTime;
            }
            break;
        }
        for (int i = 0; i < PodsPerSide * 2; i++) {
            auto& pod = Pods[i];
            if (!pod.IsEnabled()) continue;
            pod.Position += pod.Velocity * minNextCldTime;
        }
        auto& pi = Pods[mctI], & pj = Pods[mctJ];
        auto [v1, v2] = ElasticCollision(pi.Mass, pj.Mass, pi.Velocity, pj.Velocity, pi.Position, pj.Position);
        pi.Velocity = v1;
        pj.Velocity = v2;
        remainTime -= minNextCldTime;
        minNextCldTime = maxTime;
        lastI = mctI;
        lastJ = mctJ;
    }
}


bool GameSimulator::Tick() {
    for (int i = 0; i < PodsPerSide * 2; i++) {
        auto& pod = Pods[i];
        // 100+ turns not reaching a checkpoint: dead
        if (!pod.IsEnabled()) continue;
        // Check checkpoint collision
        if ((pod.Position - Checkpoints[pod.NextCheckpointIndex]).SqDist() <= CPRadiusSq) {
            pod.NextCheckpointIndex++;
            if (pod.NextCheckpointIndex >= Checkpoints.size()) {
                pod.NextCheckpointIndex = 0;
                pod.Lap++;
            }
            // Goes back to first checkpoint after all laps
            if (pod.Lap >= TotalLaps && pod.NextCheckpointIndex != 0) {
                pod.Finished = true;
                continue;
            }
            pod.NonCPTicks = 0;
        } else {
            pod.NonCPTicks++;
            if (pod.NonCPTicks >= 100) {
                pod.IsOut = true;
                continue;
            }
        }
        pod.UpdateVelocity();
        pod.IsCollided = false;
    }
    MoveAndCollide();
    // Finishing work
    for (int i = 0; i < PodsPerSide * 2; i++) {
        auto& pod = Pods[i];
        pod.Velocity *= 0.85;
        pod.Position = pod.Position.Floor();
    }
    // If all pods are out or one pod has finished: return false
    // Else return true
    bool allOut = true;
    for (int i = 0; i < PodsPerSide * 2; i++) {
        auto& pod = Pods[i];
        if (pod.Finished) return false;
        if (!pod.IsOut) allOut = false;
    }
    if (!allOut) CurrentTick++;
    return !allOut;
}

void GameSimulator::SetANN(std::shared_ptr<ANNUsed> ann) {
    ANNController = std::move(ann);
}

double GameSimulator::Fitness() {
    if (CalculatedFitness != -1) return CalculatedFitness;
    CalculatedFitness = 0;
    for (int i = 0; i < PodsPerSide * 2; i++) {
        auto& pod = Pods[i];
        CalculatedFitness += (pod.Lap * Checkpoints.size() + pod.NextCheckpointIndex) / (double) CurrentTick;
        if (pod.IsOut) continue;
        if (pod.Finished) CalculatedFitness += 0.02; // ?
        if (pod.Boosted) CalculatedFitness += 0.01;
    }
    return CalculatedFitness;
}

bool GameSimulator::Compare(GameSimulator& a, GameSimulator& b) {
    return a.Fitness() < b.Fitness();
}

void GameSimulator::Reset(std::vector<Vec> cp) {
    Setup(cp);
    CurrentTick = 1;
    CalculatedFitness = -1;
}

double GameSimulator::RecalculateFitness() {
    CalculatedFitness = -1;
    return Fitness();
}

GameSimulator::GameSimulator() = default;

void PodEncodeInfo::Write(ANNUsed& ann, int& currentNeuron) {
    ann.Neurons[currentNeuron++] = x;
    ann.Neurons[currentNeuron++] = y;
    ann.Neurons[currentNeuron++] = vx;
    ann.Neurons[currentNeuron++] = vy;
    ann.Neurons[currentNeuron++] = m;
}

PodEncodeInfo Pod::Encode() {
    PodEncodeInfo res{};
    res.x = Position.x / GameSimulator::FieldSize.x;
    res.y = Position.y / GameSimulator::FieldSize.y;
    res.vx = Velocity.x / GameSimulator::FieldSize.x;
    res.vy = Velocity.y / GameSimulator::FieldSize.y;
    res.m = Mass / ShieldMass;
    return res;
}

bool Pod::IsEnabled() const {
    return !IsOut && !Finished;
}
