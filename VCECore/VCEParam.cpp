﻿// -----------------------------------------------------------------------------------------
//     VCEEnc by rigaya
// -----------------------------------------------------------------------------------------
// The MIT License
//
// Copyright (c) 2014-2016 rigaya
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// IABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// ------------------------------------------------------------------------------------------

#pragma once

#include "VideoEncoderVCE.h"
#include "VCEUtil.h"
#include "VCEParam.h"

bool is_interlaced(VCEParam *prm) {
    return prm->nInterlaced == AMF_VIDEO_ENCODER_PICTURE_STRUCTURE_TOP_FIELD
        || prm->nInterlaced == AMF_VIDEO_ENCODER_PICTURE_STRUCTURE_BOTTOM_FIELD;
}

void init_vce_param(VCEParam *prm) {
    memset(prm, 0, sizeof(prm[0]));
    prm->memoryTypeIn = amf::AMF_MEMORY_DX9;
    prm->nCodecId     = 0;
    prm->nUsage       = AMF_VIDEO_ENCODER_USAGE_TRANSCONDING;
    prm->codecParam[prm->nCodecId].nLevel   = list_avc_level[0].value;
    prm->codecParam[prm->nCodecId].nProfile = list_avc_profile[2].value;
    prm->nInterlaced = AMF_VIDEO_ENCODER_PICTURE_STRUCTURE_FRAME;

    prm->bTimerPeriodTuning = TRUE;

    prm->nBitrate    = 5000;
    prm->nMaxBitrate = VCE_DEFAULT_MAX_BITRATE;
    prm->nVBVBufferSize = VCE_DEFAULT_VBV_BUFSIZE;
    prm->nInitialVBVPercent = 64;

    prm->nQPI = VCE_DEFAULT_QPI;
    prm->nQPP = VCE_DEFAULT_QPP;
    prm->nQPB = VCE_DEFAULT_QPB;
    prm->nQPMin = 0;
    prm->nQPMax = 51;
    prm->nDeltaQPBFrame = 6;
    prm->nDeltaQPBFrameRef = 4;

    prm->nBframes = VCE_DEFAULT_BFRAMES;
    prm->bBPyramid = FALSE;
    prm->nRateControl = AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_CONSTRAINED_QP;
    prm->bDeblockFilter = TRUE;
    prm->nGOPLen = 0;
    prm->nIDRPeriod = 1;
    prm->nSlices = 1;
    prm->nMotionEst = VCE_MOTION_EST_QUATER | VCE_MOTION_EST_HALF;
}
