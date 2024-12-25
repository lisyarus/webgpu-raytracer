#include <webgpu-raytracer/alias.hpp>

// Generate the data for sampling values in proportion to input probabilities
// using the alias method
std::vector<AliasRecord> generateAlias(std::vector<float> const & probabilities)
{
    struct IndexAndProbability
    {
        std::uint32_t index;
        float probability;
    };

    std::vector<AliasRecord> result(probabilities.size());

    std::vector<IndexAndProbability> underValues;
    std::vector<IndexAndProbability> overValues;

    for (std::uint32_t i = 0; i < probabilities.size(); ++i)
    {
        float p = probabilities[i] * probabilities.size();
        if (p < 1.f)
            underValues.push_back({i, p});
        else
            overValues.push_back({i, p});
    }

    while (!underValues.empty() && !overValues.empty())
    {
        auto under = underValues.back();
        auto over = overValues.back();

        underValues.pop_back();
        overValues.pop_back();

        result[under.index] = {
            .probability = under.probability,
            .alias = over.index,
        };

        over.probability = std::max(0.f, (over.probability + under.probability) - 1.f);

        if (over.probability < 1.f)
            underValues.push_back(over);
        else
            overValues.push_back(over);
    }

    for (auto value : underValues)
    {
        result[value.index] = {
            .probability = 1.f,
            .alias = value.index,
        };
    }

    for (auto value : overValues)
    {
        result[value.index] = {
            .probability = 1.f,
            .alias = value.index,
        };
    }

    return result;
}
