#pragma once

#include <vector>
#include <yaml-cpp/yaml.h>

#include "common/scorer.h"
#include "common/base_best_hyps.h"
#include "common/threadpool.h"
#include "gpu/types-gpu.h"
#include "gpu/mblas/matrix.h"
#include "gpu/mblas/handles.h"
#include "gpu/mblas/vector.h"
#include "enc_out_buffer.h"


namespace amunmt {

class Histories;

namespace GPU {

class EncoderDecoderState;
class Encoder;
class Decoder;
class Weights;

////////////////////////////////////////////
class EncoderDecoder : public Scorer {
  private:
    typedef EncoderDecoderState EDState;

  public:
    EncoderDecoder(const God &god,
                   const std::string& name,
                   const YAML::Node& config,
                   size_t tab,
                   const Weights& model,
                   const Search &search);

    virtual ~EncoderDecoder();

    virtual void Encode(SentencesPtr source);

    virtual void AssembleBeamState(const State& state,
                                   const Hypotheses& beam,
                                   State& nextState) const;

    virtual State* NewState() const;

    void GetAttention(mblas::Matrix& Attention);

    mblas::Matrix& GetAttention();
    virtual BaseMatrix& GetProbs();

    virtual void *GetNBest();
    virtual const BaseMatrix *GetBias() const;

    size_t GetVocabSize() const;

    void Filter(const std::vector<uint>& filterIds);


  private:
    const Weights& model_;
    std::unique_ptr<Encoder> encoder_;
    std::unique_ptr<Decoder> decoder_;

    EncOutBuffer encDecBuffer_;
    std::unique_ptr<std::thread> decThread_;

    EncoderDecoder(const EncoderDecoder&) = delete;


    /////////////////////////////////////////////////////////////////////////////////////
    // const-batch2
    void DecodeAsync();
    void DecodeAsyncInternal();
    void DecodeAsyncInternal(EncOutPtr encOut);
    void DecodeAsyncInternalOLD();

    void BeginSentenceState(size_t batchSize,
                            const mblas::Matrix &SourceContext,
                            const mblas::Vector<uint> &sentenceLengths,
                            State& state,
                            mblas::Matrix& SCU);

    size_t CalcBeam(BestHypsBase &bestHyps,
        Histories& histories,
                          Hypotheses& prevHyps,
                          State& state,
                          State& nextState,
                          const Words &filterIndices);

};

}
}

