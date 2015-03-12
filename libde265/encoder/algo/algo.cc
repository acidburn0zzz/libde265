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

#include "libde265/encoder/algo/algo.h"
#include "libde265/encoder/encoder-context.h"


CodingOptions::CodingOptions(encoder_context* ectx, int nOptions)
{
  mCBInput = NULL;
  mContextModelInput = NULL;
  //mOriginalCBStructsAssigned=false;
  mCurrentlyReconstructedOption=-1;
  mBestRDO=-1;

  mOptions.reserve(nOptions);

  mECtx = ectx;
}

CodingOptions::~CodingOptions()
{
}

void CodingOptions::set_input(enc_cb* cb, context_model_table2& tab)
{
  mCBInput = cb;
  mContextModelInput = &tab;
}

CodingOption CodingOptions::new_option(bool active)
{
  if (!active) {
    return CodingOption();
  }


  CodingOptionData opt;

  bool firstOption = mOptions.empty();
  if (firstOption) {
    opt.cb = mCBInput;
  }
  else {
    opt.cb = new enc_cb(*mCBInput);
  }

  opt.context = *mContextModelInput;

  CodingOption option(this, mOptions.size());

  mOptions.push_back( std::move(opt) );

  return option;
}


void CodingOptions::start(bool will_modify_context_model)
{
  // we don't need the input context model anymore
  mContextModelInput->release();

  if (will_modify_context_model) {
    /* If we modify the context models in this algorithm,
       we need separate models for each option.
    */
    for (auto option : mOptions) {
      option.context.decouple();
    }
  }
}


#if 000
void CodingOptions::activate_option(int idx, bool active)
{
  if (mOptions.size() < idx+1) {
    mOptions.resize(idx+1);
  }

  mOptions[idx].optionActive = active;

  if (active) {
    mOptions[idx].rdoCost = -1;

    if (!mOriginalCBStructsAssigned) {
      mOptions[idx].isOriginalCBStruct = true;
      mOptions[idx].cb = mCBInput;
      //mOptions[idx].context = mContextModelInput;

      mOriginalCBStructsAssigned=true;
    }
    else {
      mOptions[idx].isOriginalCBStruct = false;
      mOptions[idx].cb = new enc_cb;
      *mOptions[idx].cb = *mCBInput;

      //copy_context_model_table(mOptions[idx].context_table_memory, mContextModelInput);
      //mOptions[idx].context = mOptions[idx].context_table_memory;
      mOptions[idx].context = mContextModelInput;
    }
  }
}
#endif

void CodingOption::begin_reconstruction()
{
  if (mParent->mCurrentlyReconstructedOption >= 0) {
    mParent->mOptions[mParent->mCurrentlyReconstructedOption].cb->save(mParent->mECtx->img);
  }

  mParent->mCurrentlyReconstructedOption = mOptionIdx;
}

void CodingOption::end_reconstruction()
{
  assert(mParent->mCurrentlyReconstructedOption == mOptionIdx);
}


void CodingOptions::compute_rdo_costs()
{
  for (int i=0;i<mOptions.size();i++) {
    mOptions[i].rdoCost = mOptions[i].cb->distortion + mECtx->lambda * mOptions[i].cb->rate;
  }
}


enc_cb* CodingOptions::return_best_rdo()
{
  assert(mOptions.size()>0);


  float bestRDOCost = 0;
  bool  first=true;
  int   bestRDO=-1;

  for (int i=0;i<mOptions.size();i++) {
    float cost = mOptions[i].rdoCost;
    if (first || cost < bestRDOCost) {
      bestRDOCost = cost;
      first = false;
      bestRDO = i;
    }
  }


  assert(bestRDO>=0);

  if (bestRDO != mCurrentlyReconstructedOption) {
    mOptions[bestRDO].cb->restore(mECtx->img);
  }

  /*
  if ( ! mOptions[bestRDO].isOriginalCBStruct ) {
    copy_context_model_table(mContextModelInput, mOptions[bestRDO].context_table_memory);
  }
  */

  *mContextModelInput = mOptions[bestRDO].context;


  // delete all CBs except the best one

  for (int i=0;i<mOptions.size();i++) {
    if (i != bestRDO)
      {
        delete mOptions[i].cb;
        mOptions[i].cb = NULL;
      }
  }

  return mOptions[bestRDO].cb;
}