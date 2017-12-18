#pragma once
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include "hypothesis.h"
#include "sentences.h"

namespace amunmt {

typedef std::vector<HypothesisPtr> Hypotheses;

////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef std::vector<Hypotheses> Beams;

}

