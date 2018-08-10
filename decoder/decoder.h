// wujian@2018

// From Kaldi's faster-decoder.{h,cc}


#include "decoder/common.h"
#include "decoder/transition-table.h"
#include "decoder/hash-list.h"
#include "decoder/simple-fst.h"


class FasterDecoder {
public:
    FasterDecoder(const SimpleFst &fst, const TransitionTable &table, 
                    Int32 min_active = 200, Int32 max_active = 7000, 
                    Float32 beam = 15.0, Float32 ac_scale = 0.1, Float32 penalty = 0.0):
                    fst_(fst), table_(table), min_active_(min_active), 
                    max_active_(max_active), beam_(beam), 
                    acoustic_scale_(ac_scale),  word_penalty_(penalty) {
        ASSERT(min_active_ < max_active_);
        ASSERT(min_active_ > 0 && max_active_ > 1);
        ASSERT(word_penalty_ >= 0 && word_penalty_ <= 1);
        toks_.SetSize(1000);
    }

    ~FasterDecoder() { ClearToks(toks_.Clear()); }

    void Reset();

    void DecodeFrame(Float32 *loglikes, Int32 num_pdfs);

    Int32 NumDecodedFrames() { return num_frames_decoded_; }

    Bool ReachedFinal();

    Bool GetBestPath(std::vector<Int32> *word_sequence);

private:
    class Token {
    public:
        Arc arc_;
        Token *prev_;
        Int32 ref_count_;
        Float64 cost_;  // negative-log

        inline Token(const Arc &arc, Token *prev, Float32 ac_cost = 0.0):
            arc_(arc), prev_(prev), ref_count_(1) {
            if (prev) {
                prev->ref_count_++;
                cost_ = prev->cost_ + arc.weight + ac_cost;
            } else {
                cost_ = arc.weight + ac_cost;
            }
        }

        inline static void TokenDelete(Token *tok) {
            while (--tok->ref_count_ == 0) {
                Token *prev = tok->prev_;
                delete tok;
                if (prev == NULL) return;
                else tok = prev;
            }
        }
    };

    typedef HashList<StateId, Token*>::Elem Elem;

    Float64 GetCutoff(Elem *list_head, UInt64 *tok_count, Float32 *adaptive_beam, Elem **best_elem);

    Float64 ProcessEmitting(Float32 *loglikes, Int32 num_pdfs);

    inline Float32 NegativeLoglikelihood(Float32 *loglikes, Label tid);

    void ProcessNonemitting(Float64 cutoff);

    HashList<StateId, Token*> toks_;

    void ClearToks(Elem *list);

    std::vector<StateId> queue_;  // temp variable used in ProcessNonemitting,
    std::vector<Float32> tmp_array_;  // used in GetCutoff.

    Float32 beam_;
    Float32 acoustic_scale_, word_penalty_; // acwt and word penalty
    Int32 max_active_, min_active_;

    SimpleFst fst_;
    TransitionTable table_;
    Int32 num_frames_decoded_;
};