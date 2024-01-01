#pragma once

#include "cuda.h"
#include "cuda_decoder/interface/cuviddec.hpp"
#include "cuda_decoder/utils/nv_codec_utils.hpp"
#include "cuda_runtime.h"

#include <iostream>
#include <sstream>


/**
 *   @brief  Function to generate space-separated list of supported video surface formats
 *   @param  nOutputFormatMask - Bit mask to represent supported cudaVideoSurfaceFormat in decoder
 *   @param  OutputFormats     - Variable into which output string is written
 */
static void getOutputFormatNames(unsigned short nOutputFormatMask, char* OutputFormats) {
  if (nOutputFormatMask == 0) {
    strcpy(OutputFormats, "N/A");
    return;
  }
  if (nOutputFormatMask & (1U << cudaVideoSurfaceFormat_NV12)) { strcat(OutputFormats, "NV12 "); }

  if (nOutputFormatMask & (1U << cudaVideoSurfaceFormat_P016)) { strcat(OutputFormats, "P016 "); }

  if (nOutputFormatMask & (1U << cudaVideoSurfaceFormat_YUV444)) { strcat(OutputFormats, "YUV444 "); }

  if (nOutputFormatMask & (1U << cudaVideoSurfaceFormat_YUV444_16Bit)) { strcat(OutputFormats, "YUV444P16 "); }
  return;
}


/**
 *   @brief  Utility function to create CUDA context
 *   @param  cuContext - Pointer to CUcontext. Updated by this function.
 *   @param  iGpu      - Device number to get handle for
 */
static void createCudaContext(CUcontext* cuContext, int iGpu, unsigned int flags) {
  CUdevice cuDevice = 0;
  ck(cuDeviceGet(&cuDevice, iGpu));
  char szDeviceName[80];
  ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
  std::cout << "GPU in use: " << szDeviceName << std::endl;
  ck(cuCtxCreate(cuContext, flags, cuDevice));
}



/**
 *   @brief  Print decoder capabilities on std::cout
 */
static void ShowDecoderCapability() {
  ck(cuInit(0));
  int nGpu = 0;
  ck(cuDeviceGetCount(&nGpu));
  std::cout << "Decoder Capability" << std::endl << std::endl;
  const char*    aszCodecName[]    = {"JPEG", "MPEG1", "MPEG2", "MPEG4", "H264", "HEVC", "HEVC", "HEVC", "HEVC", "HEVC", "HEVC", "VC1", "VP8", "VP9", "VP9", "VP9", "AV1", "AV1", "AV1", "AV1"};
  const char*    aszChromaFormat[] = {"4:0:0", "4:2:0", "4:2:2", "4:4:4"};
  char           strOutputFormats[64];
  cudaVideoCodec aeCodec[]          = {cudaVideoCodec_JPEG, cudaVideoCodec_MPEG1, cudaVideoCodec_MPEG2, cudaVideoCodec_MPEG4, cudaVideoCodec_H264, cudaVideoCodec_HEVC, cudaVideoCodec_HEVC,
                                       cudaVideoCodec_HEVC, cudaVideoCodec_HEVC,  cudaVideoCodec_HEVC,  cudaVideoCodec_HEVC,  cudaVideoCodec_VC1,  cudaVideoCodec_VP8,  cudaVideoCodec_VP9,
                                       cudaVideoCodec_VP9,  cudaVideoCodec_VP9,   cudaVideoCodec_AV1,   cudaVideoCodec_AV1,   cudaVideoCodec_AV1,  cudaVideoCodec_AV1};
  int            anBitDepthMinus8[] = {0, 0, 0, 0, 0, 0, 2, 4, 0, 2, 4, 0, 0, 0, 2, 4, 0, 2, 0, 2};

  cudaVideoChromaFormat aeChromaFormat[] = {cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420,        cudaVideoChromaFormat_420,
                                            cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_444,        cudaVideoChromaFormat_444,
                                            cudaVideoChromaFormat_444, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420,        cudaVideoChromaFormat_420,
                                            cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_420, cudaVideoChromaFormat_Monochrome, cudaVideoChromaFormat_Monochrome};

  for (int iGpu = 0; iGpu < nGpu; iGpu++) {

    CUcontext cuContext = NULL;
    createCudaContext(&cuContext, iGpu, 0);

    for (int i = 0; i < sizeof(aeCodec) / sizeof(aeCodec[0]); i++) {

      CUVIDDECODECAPS decodeCaps = {};
      decodeCaps.eCodecType      = aeCodec[i];
      decodeCaps.eChromaFormat   = aeChromaFormat[i];
      decodeCaps.nBitDepthMinus8 = anBitDepthMinus8[i];

      cuvidGetDecoderCaps(&decodeCaps);

      strOutputFormats[0] = '\0';
      getOutputFormatNames(decodeCaps.nOutputFormatMask, strOutputFormats);

      // setw() width = maximum_width_of_string + 2 spaces
      std::cout << "Codec  " << std::left << std::setw(7) << aszCodecName[i] << "BitDepth  " << std::setw(4) << decodeCaps.nBitDepthMinus8 + 8 << "ChromaFormat  " << std::setw(7)
                << aszChromaFormat[decodeCaps.eChromaFormat] << "Supported  " << std::setw(3) << (int)decodeCaps.bIsSupported << "MaxWidth  " << std::setw(7) << decodeCaps.nMaxWidth << "MaxHeight  "
                << std::setw(7) << decodeCaps.nMaxHeight << "MaxMBCount  " << std::setw(10) << decodeCaps.nMaxMBCount << "MinWidth  " << std::setw(5) << decodeCaps.nMinWidth << "MinHeight  "
                << std::setw(5) << decodeCaps.nMinHeight << "SurfaceFormat  " << std::setw(11) << strOutputFormats << std::endl;
    }

    std::cout << std::endl;

    ck(cuCtxDestroy(cuContext));
  }
}
