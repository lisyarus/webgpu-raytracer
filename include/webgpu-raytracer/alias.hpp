#pragma once

#include <vector>
#include <cstdint>

struct AliasRecord
{
    float probability;
    std::uint32_t alias;
};

// Generate the data for sampling values in proportion to input probabilities
// using the alias method
std::vector<AliasRecord> generateAlias(std::vector<float> const & probabilities);
