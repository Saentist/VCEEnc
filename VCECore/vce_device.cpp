﻿// -----------------------------------------------------------------------------------------
//     VCEEnc by rigaya
// -----------------------------------------------------------------------------------------
// The MIT License
//
// Copyright (c) 2014-2017 rigaya
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

#include "vce_device.h"
#include "vce_util.h"

VCEDevice::VCEDevice(shared_ptr<RGYLog> &log, amf::AMFFactory *factory, amf::AMFTrace *trace) :
    m_log(log),
    m_id(-1),
    m_devName(),
    m_d3d9interlop(false),
    m_dx9(),
    m_d3d11interlop(false),
    m_dx11(),
    m_cl(),
    m_context(),
    m_factory(factory),
    m_trace(trace),
    m_gotCaps(false),
    m_encCaps(),
    m_decCaps() {

}

VCEDevice::~VCEDevice() {
    m_context.Release();
    m_factory = nullptr;
    m_trace = nullptr;

    m_cl.reset();
    m_dx11.Terminate();
    m_dx9.Terminate();
    m_devName.clear();
    m_log.reset();
}

RGY_ERR VCEDevice::CreateContext() {
    auto res = m_factory->CreateContext(&m_context);
    if (res != AMF_OK) {
        PrintMes(RGY_LOG_ERROR, _T("Failed to CreateContext(): %s.\n"), get_err_mes(err_to_rgy(res)));
        return err_to_rgy(res);
    }
    PrintMes(RGY_LOG_DEBUG, _T("CreateContext() Success.\n"));
    return RGY_ERR_NONE;
}

RGY_ERR VCEDevice::init(const int deviceId, const bool interopD3d9, const bool interopD3d11) {
    m_devName = strsprintf(_T("device #%d"), deviceId);
    m_id = deviceId;
    {
        auto err = CreateContext();
        if (err != RGY_ERR_NONE) {
            return err;
        }
    }
    m_d3d9interlop = interopD3d9;
    m_d3d11interlop = interopD3d11;
    if (interopD3d9) {
        auto err = m_dx9.Init(true, deviceId, false, 1280, 720, m_log);
        if (err != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("Failed to get directX9 device.\n"));
            return RGY_ERR_DEVICE_LOST;
        }
        auto amferr = m_context->InitDX9(m_dx9.GetDevice());
        if (amferr != AMF_OK) {
            PrintMes(RGY_LOG_ERROR, _T("Failed to init AMF context by directX9.\n"));
            return err_to_rgy(amferr);
        }
    }
    if (interopD3d11 || !interopD3d9) {
        auto err = m_dx11.Init(deviceId, false, m_log);
        if (err != RGY_ERR_NONE) {
            PrintMes(RGY_LOG_ERROR, _T("Failed to get directX11 device.\n"));
            return RGY_ERR_DEVICE_LOST;
        }
        auto amferr = m_context->InitDX11(m_dx11.GetDevice());
        if (amferr != AMF_OK) {
            PrintMes(RGY_LOG_ERROR, _T("Failed to init AMF context by directX11.\n"));
            return err_to_rgy(amferr);
        }
    }

    RGYOpenCL cl(m_log);
    auto platforms = cl.getPlatforms("AMD");
    if (platforms.size() == 0) {
        PrintMes(RGY_LOG_ERROR, _T("Failed to find OpenCL platforms.\n"));
        return RGY_ERR_DEVICE_LOST;
    }
    auto& platform = platforms[0];
    if (interopD3d9) {
        if (platform->createDeviceListD3D9(CL_DEVICE_TYPE_GPU, (void *)m_dx9.GetDevice()) != CL_SUCCESS || platform->devs().size() == 0) {
            PrintMes(RGY_LOG_ERROR, _T("Failed to find device.\n"));
            return RGY_ERR_DEVICE_LOST;
        }
    } else if (interopD3d11 || !interopD3d9) {
        if (platform->createDeviceListD3D11(CL_DEVICE_TYPE_GPU, (void *)m_dx11.GetDevice()) != CL_SUCCESS || platform->devs().size() == 0) {
            PrintMes(RGY_LOG_ERROR, _T("Failed to find device.\n"));
            return RGY_ERR_DEVICE_LOST;
        }
    } else {
        if (platform->createDeviceList(CL_DEVICE_TYPE_GPU) != CL_SUCCESS || platform->devs().size() == 0) {
            PrintMes(RGY_LOG_ERROR, _T("Failed to find device.\n"));
            return RGY_ERR_DEVICE_LOST;
        }
    }
    const int selectCLDevice = (interopD3d9 || interopD3d11) ? 0 : deviceId;
    auto devices = platform->devs();
    if ((int)devices.size() <= selectCLDevice) {
        PrintMes(RGY_LOG_ERROR, _T("Failed to device #%d.\n"), deviceId);
        return RGY_ERR_DEVICE_LOST;
    }
    platform->setDev(devices[selectCLDevice],
        (interopD3d9) ? m_dx9.GetDevice() : nullptr,
        (interopD3d11) ? m_dx11.GetDevice() : nullptr);

    m_cl = std::make_shared<RGYOpenCLContext>(platform, m_log);
    if (m_cl->createContext() != CL_SUCCESS) {
        PrintMes(RGY_LOG_ERROR, _T("Failed to create OpenCL context.\n"));
        return RGY_ERR_UNKNOWN;
    }
    auto amferr = m_context->InitOpenCL(m_cl->queue().get());
    if (amferr != AMF_OK) {
        PrintMes(RGY_LOG_ERROR, _T("Failed to init AMF context by OpenCL.\n"));
        return err_to_rgy(amferr);
    }

    m_devName = getGPUInfo();
    return RGY_ERR_NONE;
}

void VCEDevice::getAllCaps() {
    //エンコーダの情報
    for (int i = 0; list_codec[i].desc; i++) {
        const auto codec = (RGY_CODEC)list_codec[i].value;
        getEncCaps(codec);
    }
    //デコーダの情報
    for (size_t i = 0; i < _countof(HW_DECODE_LIST); i++) {
        const auto codec = HW_DECODE_LIST[i].rgy_codec;
        getDecCaps(codec);
    }
}

amf::AMFCapsPtr VCEDevice::getEncCaps(RGY_CODEC codec) {
    if (m_encCaps.count(codec) == 0) {
        m_encCaps[codec] = amf::AMFCapsPtr();
        amf::AMFCapsPtr encoderCaps;
        amf::AMFComponentPtr p_encoder;
        auto ret = m_factory->CreateComponent(m_context, codec_rgy_to_enc(codec), &p_encoder);
        if (ret == AMF_OK) {
            //HEVCでのAMFComponent::GetCaps()は、AMFComponent::Init()を呼んでおかないと成功しない
            p_encoder->Init(amf::AMF_SURFACE_NV12, 1280, 720);
            ret = p_encoder->GetCaps(&encoderCaps);
            m_encCaps[codec] = encoderCaps;
        }
    }
    return m_encCaps[codec];
}

amf::AMFCapsPtr VCEDevice::getDecCaps(RGY_CODEC codec) {
    if (m_decCaps.count(codec) == 0) {
        const auto codec_uvd_name = codec_rgy_to_dec(codec);
        m_encCaps[codec] = amf::AMFCapsPtr();
        if (codec_uvd_name != nullptr) {
            amf::AMFCapsPtr decodeCaps;
            amf::AMFComponentPtr p_decode;
            auto ret = m_factory->CreateComponent(m_context, codec_uvd_name, &p_decode);
            if (ret == AMF_OK) {
                ret = p_decode->GetCaps(&decodeCaps);
                m_decCaps[codec] = decodeCaps;
            }
        }
    }
    return m_decCaps[codec];
}

tstring VCEDevice::QueryIOCaps(amf::AMFIOCapsPtr& ioCaps) {
    tstring str;
    bool result = true;
    if (ioCaps != NULL) {
        amf_int32 minWidth, maxWidth;
        ioCaps->GetWidthRange(&minWidth, &maxWidth);
        str += strsprintf(_T("Width:       %d - %d\n"), minWidth, maxWidth);

        amf_int32 minHeight, maxHeight;
        ioCaps->GetHeightRange(&minHeight, &maxHeight);
        str += strsprintf(_T("Height:      %d - %d\n"), minHeight, maxHeight);

        amf_int32 vertAlign = ioCaps->GetVertAlign();
        str += strsprintf(_T("alignment:   %d\n"), vertAlign);

        amf_bool interlacedSupport = ioCaps->IsInterlacedSupported();
        str += strsprintf(_T("Interlace:   %s\n"), interlacedSupport ? _T("yes") : _T("no"));

        amf_int32 numOfFormats = ioCaps->GetNumOfFormats();
        str += _T("pix format:  ");
        for (amf_int32 i = 0; i < numOfFormats; i++) {
            amf::AMF_SURFACE_FORMAT format;
            amf_bool native = false;
            if (ioCaps->GetFormatAt(i, &format, &native) == AMF_OK) {
                if (i) str += _T(", ");
                str += wstring_to_tstring(m_trace->SurfaceGetFormatName(format)) + ((native) ? _T("(native)") : _T(""));
            }
        }
        str += _T("\n");

        if (result == true) {
            amf_int32 numOfMemTypes = ioCaps->GetNumOfMemoryTypes();
            str += _T("memory type: ");
            for (amf_int32 i = 0; i < numOfMemTypes; i++) {
                amf::AMF_MEMORY_TYPE memType;
                amf_bool native = false;
                if (ioCaps->GetMemoryTypeAt(i, &memType, &native) == AMF_OK) {
                    if (i) str += _T(", ");
                    str += wstring_to_tstring(m_trace->GetMemoryTypeName(memType)) + ((native) ? _T("(native)") : _T(""));
                }
            }
        }
        str += _T("\n");
    } else {
        str += _T("failed to get io capability\n");
    }
    return str;
}

tstring VCEDevice::QueryIOCaps(RGY_CODEC codec, amf::AMFCapsPtr& encoderCaps) {
    tstring str;
    if (encoderCaps == NULL) {
        str += _T("failed to get encoder capability\n");
    }
    amf::AMF_ACCELERATION_TYPE accelType = encoderCaps->GetAccelerationType();
    str += _T("acceleration:   ") + AccelTypeToString(accelType) + _T("\n");

    amf_uint32 maxProfile = 0;
    encoderCaps->GetProperty(AMF_PARAM_CAP_MAX_PROFILE(codec), &maxProfile);
    str += _T("max profile:    ") + tstring(get_cx_desc(get_profile_list(codec), maxProfile)) + _T("\n");

    amf_uint32 maxLevel = 0;
    encoderCaps->GetProperty(AMF_PARAM_CAP_MAX_LEVEL(codec), &maxLevel);
    str += _T("max level:      ") + tstring(get_cx_desc(get_level_list(codec), maxLevel)) + _T("\n");

    amf_uint32 maxBitrate = 0;
    encoderCaps->GetProperty(AMF_PARAM_CAP_MAX_BITRATE(codec), &maxBitrate);
    str += strsprintf(_T("max bitrate:    %d kbps\n"), maxBitrate / 1000);

    amf_uint32 maxRef = 0, minRef = 0;
    encoderCaps->GetProperty(AMF_PARAM_CAP_MIN_REFERENCE_FRAMES(codec), &minRef);
    encoderCaps->GetProperty(AMF_PARAM_CAP_MAX_REFERENCE_FRAMES(codec), &maxRef);
    str += strsprintf(_T("ref frames:     %d-%d\n"), minRef, maxRef);

    if (codec == RGY_CODEC_H264) {
        //amf_uint32 maxTemporalLayers = 0;
        //encoderCaps->GetProperty(AMF_VIDEO_ENCODER_CAP_MAX_TEMPORAL_LAYERS, &maxTemporalLayers);
        //str += strsprintf(_T("max temp layers:  %d\n"), maxTemporalLayers);

        bool bBPictureSupported = false;
        encoderCaps->GetProperty(AMF_VIDEO_ENCODER_CAP_BFRAMES, &bBPictureSupported);
        str += strsprintf(_T("Bframe support: %s\n"), (bBPictureSupported) ? _T("yes") : _T("no"));

        amf_uint32 NumOfHWInstances = 0;
        encoderCaps->GetProperty(AMF_VIDEO_ENCODER_CAP_NUM_OF_HW_INSTANCES, &NumOfHWInstances);
        str += strsprintf(_T("HW instances:   %d\n"), NumOfHWInstances);
    } else if (codec == RGY_CODEC_HEVC) {
        //いまは特になし
    }

    amf_uint32 maxNumOfStreams = 0;
    encoderCaps->GetProperty(AMF_PARAM_CAP_NUM_OF_STREAMS(codec), &maxNumOfStreams);
    str += strsprintf(_T("max streams:    %d\n"), maxNumOfStreams);


    str += strsprintf(_T("\n%s encoder input:\n"), CodecToStr(codec).c_str());
    amf::AMFIOCapsPtr inputCaps;
    if (encoderCaps->GetInputCaps(&inputCaps) == AMF_OK) {
        str += QueryIOCaps(inputCaps);
    }

    str += strsprintf(_T("\n%s encoder output:\n"), CodecToStr(codec).c_str());
    amf::AMFIOCapsPtr outputCaps;
    if (encoderCaps->GetOutputCaps(&outputCaps) == AMF_OK) {
        str += QueryIOCaps(outputCaps);
    }
    return str;
}

tstring VCEDevice::getGPUInfo() const {
    if (m_dx11.isValid()) {
        auto str = m_dx11.GetDisplayDeviceName();
        str = str_replace(str, L"(TM)", L"");
        str = str_replace(str, L"(R)", L"");
        str = str_replace(str, L" Series", L"");
        str = str_replace(str, L" Graphics", L"");
        return wstring_to_tstring(str);
    }
    return RGYOpenCLDevice(m_cl->platform()->dev(0)).infostr();
}
