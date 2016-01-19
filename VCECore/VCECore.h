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

#include <d3d9.h>
#include <d3d11.h>
#include "VideoEncoderVCE.h"
#include "DeviceDX9.h"
#include "DeviceDX11.h"
#include "AMFPlatform.h"
#include "PlatformWindows.h"
#include "Thread.h"

#include "PipelineElement.h"
#include "Pipeline.h"
#include "RawStreamReader.h"

#include "CapabilityManager.h"
#include "VideoEncoderVCECaps.h"

#include "VCEUtil.h"
#include "VCEParam.h"
#include "VCELog.h"
#include "VCEInput.h"
#include "VCEOutput.h"

class VCECore : public Pipeline {
    class PipelineElementAMFComponent;
    class PipelineElementEncoder;
public:
    VCECore();
    virtual ~VCECore();

    AMF_RESULT init(VCEParam *prm, VCEInputInfo *inputInfo);
    virtual AMF_RESULT initInput(VCEParam *prm);
    AMF_RESULT initOutput(VCEParam *prm);
    AMF_RESULT run();
    void Terminate();

    void PrintMes(int log_level, const TCHAR *format, ...);

    tstring GetEncoderParam();
    void PrintEncoderParam();
    AMF_RESULT PrintResult();

    static const wchar_t* PARAM_NAME_INPUT;
    static const wchar_t* PARAM_NAME_INPUT_WIDTH;
    static const wchar_t* PARAM_NAME_INPUT_HEIGHT;

    static const wchar_t* PARAM_NAME_OUTPUT;
    static const wchar_t* PARAM_NAME_OUTPUT_WIDTH;
    static const wchar_t* PARAM_NAME_OUTPUT_HEIGHT;

    static const wchar_t* PARAM_NAME_ENGINE;

    static const wchar_t* PARAM_NAME_ADAPTERID;
    static const wchar_t* PARAM_NAME_CAPABILITY;
protected:
    std::wstring AccelTypeToString(amf::AMF_ACCELERATION_TYPE accelType);
    bool QueryIOCaps(amf::AMFIOCapsPtr& ioCaps);
    bool QueryEncoderForCodec(const wchar_t *componentID, amf::AMFCapabilityManagerPtr& capsManager);
    bool QueryEncoderCaps(amf::AMFCapabilityManagerPtr& capsManager);

    virtual AMF_RESULT checkParam(VCEParam *prm);
    virtual AMF_RESULT initEncoder(VCEParam *prm);

    shared_ptr<VCELog> m_pVCELog;
    bool m_bTimerPeriodTuning;

    shared_ptr<VCEInput> m_pInput;
    shared_ptr<VCEOutput> m_pOutput;
    shared_ptr<VCEStatus> m_pStatus;

    VCEInputInfo m_inputInfo;

    amf::AMFContextPtr m_pContext;

    AMFDataStreamPtr m_pStreamOut;

    amf::AMFComponentPtr m_pEncoder;

    DeviceDX9 m_deviceDX9;
    DeviceDX11 m_deviceDX11;

    ParametersStorage m_Params;
};

bool check_if_vce_available();
bool check_if_vce_available(tstring& mes);
