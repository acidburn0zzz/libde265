/*
 * H.265 video codec.
 * Copyright (c) 2013-2014 struktur AG, Dirk Farin <farin@struktur.de>
 *
 * Authors: Dirk Farin <farin@struktur.de>
 *
 * This file is part of libde265.
 *
 * libde265 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libde265 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libde265.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TB_SPLIT_H
#define TB_SPLIT_H

#include "libde265/nal-parser.h"
#include "libde265/decctx.h"
#include "libde265/encoder/encode.h"
#include "libde265/slice.h"
#include "libde265/scan.h"
#include "libde265/intrapred.h"
#include "libde265/transform.h"
#include "libde265/fallback-dct.h"
#include "libde265/quality.h"
#include "libde265/fallback.h"
#include "libde265/configparam.h"

#include "libde265/encoder/algo/tb-intrapredmode.h"


/*  Encoder search tree, bottom up:

    - Algo_TB_Split - whether TB is split or not

    - Algo_TB_IntraPredMode - choose the intra prediction mode (or NOP, if at the wrong tree level)

    - Algo_CB_IntraPartMode - choose between NxN and 2Nx2N intra parts

    - Algo_CB_Split - whether CB is split or not

    - Algo_CTB_QScale - select QScale on CTB granularity
 */


// ========== TB split decision ==========

class Algo_TB_Split
{
 public:
  Algo_TB_Split() : mAlgo_TB_IntraPredMode(NULL) { }
  virtual ~Algo_TB_Split() { }

  virtual const enc_tb* analyze(encoder_context*,
                                context_model_table2&,
                                const de265_image* input,
                                const enc_tb* parent,
                                enc_cb* cb,
                                int x0,int y0, int xBase,int yBase, int log2TbSize,
                                int blkIdx,
                                int TrafoDepth, int MaxTrafoDepth, int IntraSplitFlag) = 0;

  void setAlgo_TB_IntraPredMode(Algo_TB_IntraPredMode* algo) { mAlgo_TB_IntraPredMode=algo; }

 protected:
  const enc_tb* encode_transform_tree_split(encoder_context* ectx,
                                            context_model_table2& ctxModel,
                                            const de265_image* input,
                                            const enc_tb* parent,
                                            enc_cb* cb,
                                            int x0,int y0, int log2TbSize,
                                            int TrafoDepth, int MaxTrafoDepth, int IntraSplitFlag);

  Algo_TB_IntraPredMode* mAlgo_TB_IntraPredMode;
};



enum ALGO_TB_Split_BruteForce_ZeroBlockPrune {
  // numeric value specifies the maximum size for log2Tb for which the pruning is applied
  ALGO_TB_BruteForce_ZeroBlockPrune_off = 0,
  ALGO_TB_BruteForce_ZeroBlockPrune_8x8 = 3,
  ALGO_TB_BruteForce_ZeroBlockPrune_8x8_16x16 = 4,
  ALGO_TB_BruteForce_ZeroBlockPrune_all = 5
};

class option_ALGO_TB_Split_BruteForce_ZeroBlockPrune
: public choice_option<enum ALGO_TB_Split_BruteForce_ZeroBlockPrune>
{
 public:
  option_ALGO_TB_Split_BruteForce_ZeroBlockPrune() {
    add_choice("off"     ,ALGO_TB_BruteForce_ZeroBlockPrune_off);
    add_choice("8x8"     ,ALGO_TB_BruteForce_ZeroBlockPrune_8x8);
    add_choice("8-16"    ,ALGO_TB_BruteForce_ZeroBlockPrune_8x8_16x16);
    add_choice("all"     ,ALGO_TB_BruteForce_ZeroBlockPrune_all, true);
  }
};

class Algo_TB_Split_BruteForce : public Algo_TB_Split
{
 public:
  struct params
  {
    params() {
      zeroBlockPrune.set_ID("TB-Split-BruteForce-ZeroBlockPrune");
    }

    option_ALGO_TB_Split_BruteForce_ZeroBlockPrune zeroBlockPrune;
  };

  void setParams(const params& p) { mParams=p; }

  void registerParams(config_parameters& config) {
    config.add_option(&mParams.zeroBlockPrune);
  }

  virtual const enc_tb* analyze(encoder_context*,
                                context_model_table2&,
                                const de265_image* input,
                                const enc_tb* parent,
                                enc_cb* cb,
                                int x0,int y0, int xBase,int yBase, int log2TbSize,
                                int blkIdx,
                                int TrafoDepth, int MaxTrafoDepth, int IntraSplitFlag);

 private:
  params mParams;
};

#endif