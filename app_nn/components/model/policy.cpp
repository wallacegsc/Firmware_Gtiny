#include <vector>
#include <algorithm>
#include <numeric>
#include <cstdio>
#include <cmath>

#include <policy.h>

using namespace std;

vector<vector<float>> center_norm(vector<vector<float>> x)
{
    vector<vector<float>> r;
    r.insert(r.begin(),x.begin(),x.end());
    for (int channel = 0; channel < x[0].size(); channel++)
    {
        double c_mean = 0;
        
        for (int i = 0 ; i < x.size();i++)
        {
            c_mean+= static_cast<double>(x[i][channel]);
        }
        c_mean /= x.size();
        for (int i = 0 ; i < x.size();i++)
        {
            r[i][channel] = static_cast<float>(static_cast<double>(x[i][channel]) - c_mean);
        }
    }
    return r;
}

vector<int> argmax(vector<vector<float>> l)
{
    vector<int> r;

    for (vector<float> el : l)
    {
        r.push_back(distance(el.begin(), max_element(el.begin(), el.end())));
    }

    return r;
}

Weighted_Policies::Weighted_Policies(
    float c, float rf,
    float p, bool r,
    float mpf, float mrf,
    float mincs, float maxcs) : max_penalty_factor(mpf), max_reward_factor(mrf),
                                             min_confidence_score(mincs), max_confidence_score(maxcs)
{
    Weighted_Policies::confidence = c;
    Weighted_Policies::reward_factor = rf;
    Weighted_Policies::penalty_factor = p;
    Weighted_Policies::rele_activation = r;
}

bool Weighted_Policies::prediction(vector<vector<float>> predictions, float *conf)
{
    Weighted_Policies::rele_activation = weight_policy(predictions, conf);
    return Weighted_Policies::rele_activation ;
}

bool Weighted_Policies::weight_policy(vector<vector<float>> predictions, float *conf)
{
    vector<int> cat_predictions = argmax(predictions);
    int last_prediction = cat_predictions.back();

    if (last_prediction != 0)
    {
        //printf("-------last prediction--------- \n");
        float penalty_score = Weighted_Policies::calculate_penalty(predictions);
        //printf("penalty score %f \n", penalty_score);
        Weighted_Policies::calculate_penalty(predictions);
        float penalty = pow(2, (penalty_score * Weighted_Policies::penalty_factor));

        if (Weighted_Policies::confidence > 0)
        {
            //printf("valor do penalty %f \n", penalty);
            Weighted_Policies::confidence -= penalty;
            //printf("Valor de confidence apos penalty %f \n", Weighted_Policies::confidence);
            if (Weighted_Policies::confidence < 0)
            {
                Weighted_Policies::confidence = 0;
            }
        }

        if (Weighted_Policies::penalty_factor < Weighted_Policies::max_penalty_factor)
            Weighted_Policies::penalty_factor++;
    }
    else
    { // last_prediction == 0
        if ((Weighted_Policies::confidence >= Weighted_Policies::min_confidence_score) && (Weighted_Policies::confidence < Weighted_Policies::max_confidence_score))
        {
            //printf("-------reward--------- \n");
            float reward_score = Weighted_Policies::calculate_reward(predictions);
            float reward = 2 * (reward_score * Weighted_Policies::reward_factor);
            Weighted_Policies::confidence += reward;

            if (Weighted_Policies::confidence > Weighted_Policies::max_confidence_score)
            {
                Weighted_Policies::confidence = Weighted_Policies::max_confidence_score;
            }

            if (Weighted_Policies::reward_factor < Weighted_Policies::max_reward_factor)
            {
                Weighted_Policies::reward_factor++;
            }
        }
        else
        {
            //printf("-------reward 2--------- \n");
            if (Weighted_Policies::confidence < Weighted_Policies::min_confidence_score)
            {
                float reward_score = Weighted_Policies::calculate_reward(predictions);
                float reward = 2 * (reward_score * Weighted_Policies::reward_factor);
                Weighted_Policies::confidence += reward;

                if (Weighted_Policies::confidence > Weighted_Policies::max_confidence_score)
                {
                    Weighted_Policies::confidence = Weighted_Policies::max_confidence_score;
                }

                if (Weighted_Policies::reward_factor < Weighted_Policies::max_reward_factor)
                {
                    Weighted_Policies::reward_factor++;
                }
            }
            else
            {
                //printf("-------penalty--------- \n");
                if (Weighted_Policies::confidence == Weighted_Policies::max_confidence_score)
                {
                    Weighted_Policies::reward_factor = 1;
                    if (Weighted_Policies::penalty_factor > 1)
                    {
                        Weighted_Policies::penalty_factor--;
                    }
                }
                else
                {
                    printf("Out of confidence range...");
                }
            }
        }
        //else { printf("Prediction Error"); }
    }
    //printf("valor de confidence - %f e a referencia - %f e o rele - %d", (Weighted_Policies::confidence),(Weighted_Policies::min_confidence_score), (Weighted_Policies::rele_activation)); 
    //printf("\n");
    *conf = Weighted_Policies::confidence;
    if ((Weighted_Policies::confidence < Weighted_Policies::min_confidence_score) && (!Weighted_Policies::rele_activation))
    {
        Weighted_Policies::rele_activation = true;
    }
    else
    {
        if ((Weighted_Policies::confidence >= Weighted_Policies::min_confidence_score) && 
            (Weighted_Policies::confidence <= Weighted_Policies::max_confidence_score) && (Weighted_Policies::rele_activation))
        {
            Weighted_Policies::rele_activation = false;
        }
    }

    return Weighted_Policies::rele_activation;
}

float Weighted_Policies::calculate_penalty(vector<vector<float>> pred)
{
    vector<vector<float>> attack_proba;

    for (auto el : pred)
    {
        vector<float> t = {el[1], el[2]}; // <- preds[:, 1:3]?
        attack_proba.push_back(t);
    }

    vector<float> attack_sum_proba;

    for (auto el : attack_proba)
    {
        attack_sum_proba.push_back(accumulate(el.begin(), el.end(), 0.0));
    }

    vector<float> weights_;

    for (int i = 1; i < attack_sum_proba.size() + 1; i++)
    {
        weights_.push_back(1.0 / i);
    }

    std::reverse(weights_.begin(), weights_.end());

    float sum = 0;

    for (int i = 0; i < attack_sum_proba.size(); i++)
    {
        sum += attack_sum_proba[i] * weights_[i];
    }

    //float _avg = sum / attack_sum_proba.size();
    float _avg = sum / accumulate(weights_.begin(),weights_.end(), 0.0);

    return _avg;
}

float Weighted_Policies::calculate_reward(vector<vector<float>> preds)
{
    vector<float> normal_proba;

    for (auto el : preds)
    {
        normal_proba.push_back(el[0]);
    }

    vector<float> weights_;

    for (int i = 1; i < normal_proba.size() + 1; i++)
    {
        weights_.push_back(1 / i);
    }

    std::reverse(weights_.begin(), weights_.end());

    float sum = 0;

    for (int i = 0; i < normal_proba.size(); i++)
    {
        sum += normal_proba[i] * weights_[i];
    }

    float _avg = sum /  accumulate(weights_.begin(),weights_.end(), 0.0);

    return _avg;
}
