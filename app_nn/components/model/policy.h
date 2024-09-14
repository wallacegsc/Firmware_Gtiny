#ifndef POLICY_H
#define POLICY_H

#include <vector>

std::vector<std::vector<float>> center_norm(std::vector<std::vector<float>> x);
std::vector<int> argmax(std::vector<std::vector<float>> l);

class Weighted_Policies
{

public:
    Weighted_Policies(
        float c, float rf,
        float p, bool r,
        float mpf, float mrf,
        float mincs, float maxcs);
    bool prediction(std::vector<std::vector<float>> predictions, float *conf);

    bool weight_policy(std::vector<std::vector<float>> predictions, float *conf);

    static float calculate_penalty(std::vector<std::vector<float>> pred);

    static float calculate_reward(std::vector<std::vector<float>> preds);

private:
    //int *ca = Null;
    //police config;
    float confidence;
    float reward_factor;
    float penalty_factor;
    bool rele_activation;

    const float max_penalty_factor;
    const float max_reward_factor;
    const float min_confidence_score;
    const float max_confidence_score;
};

#endif