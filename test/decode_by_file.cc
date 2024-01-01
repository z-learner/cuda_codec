#include "cuda.h"
#include "cuda_decoder/decoder/nv_decoder.hpp"
#include "cuda_decoder/demuxer/ffmpeg_demuxer.hpp"
#include "cuda_decoder/utils/cuda_utils.hpp"
#include "cxxopts.hpp"
#include "libavutil/error.h"

#include <chrono>

class FileDataProvider : public FFmpegDemuxer::DataProvider {
public:
  FileDataProvider(std::string szInFilePath) {
    fpIn.open(szInFilePath.c_str(), std::ifstream::in | std::ifstream::binary);
    if (!fpIn) {
      std::cout << "Unable to open input file: " << szInFilePath << std::endl;
      return;
    }
  }
  ~FileDataProvider() { fpIn.close(); }
  // Fill in the buffer owned by the demuxer/decoder
  int GetData(uint8_t* pBuf, int nBuf) {
    if (fpIn.eof()) { return AVERROR_EOF; }

    // We read a file for this example. You may get your data from network or somewhere else
    return (int)fpIn.read(reinterpret_cast<char*>(pBuf), nBuf).gcount();
  }

private:
  std::ifstream fpIn;
};



int main(int argc, char** argv) {

  std::string input_file_name{""};
  std::string out_file_name{""};
  int         gpu_id = 0;

  cxxopts::Options options("decode test", "parameter for decode test");

  options.add_options()("i,input_file", "input_file", cxxopts::value<std::string>())("o,output_file", "output_file", cxxopts::value<std::string>())("g,gpu_id", "gpu_id",
                                                                                                                                                    cxxopts::value<int>()->default_value("0"));

  auto result     = options.parse(argc, argv);
  input_file_name = result["input_file"].as<std::string>();
  out_file_name   = result["output_file"].as<std::string>();
  gpu_id          = result["gpu_id"].as<int>();

  int gpu_count = 0;

  ck(cuInit(0));
  ck(cuDeviceGetCount(&gpu_count));
  if (gpu_id < 0 || gpu_id >= gpu_count) {
    std::ostringstream err;
    err << "GPU ordinal out of range. Should be within [" << 0 << ", " << gpu_count - 1 << "]" << std::endl;
    throw std::invalid_argument(err.str());
  }
  ShowDecoderCapability();

  CUcontext cuContext = NULL;
  createCudaContext(&cuContext, gpu_id, 0);
  FileDataProvider dp(input_file_name);
  FFmpegDemuxer    demuxer(&dp);

  NvDecoder dec(cuContext, false, FFmpeg2NvCodecId(demuxer.GetVideoCodec()));

  std::ofstream fpOut(out_file_name, std::ios::out | std::ios::binary);
  if (!fpOut) {
    std::ostringstream err;
    err << "Unable to open output file: " << out_file_name << std::endl;
    throw std::invalid_argument(err.str());
  }

  int      nFrame         = 0;
  uint8_t* pVideo         = nullptr;
  int      nVideoBytes    = 0;
  uint8_t* pFrame         = nullptr;
  int      nFrameReturned = 0;


  double decode_time = 0;   // ms
  int    frame_count = 0;

  do {
    demuxer.Demux(&pVideo, &nVideoBytes);
    auto start     = std::chrono::steady_clock::now();
    nFrameReturned = dec.Decode(pVideo, nVideoBytes);
    auto duration  = std::chrono::steady_clock::now() - start;
    decode_time += duration.count() / 1000000.0f;
    if (!nFrame && nFrameReturned) { std::cout << dec.GetVideoInfo() << std::endl; }
    nFrame += nFrameReturned;
    frame_count += nFrameReturned;

    if (frame_count >= 100) {
      std::cout << "decode avg time in " << frame_count << " frames is " << decode_time / frame_count << " ms" << std::endl;
      frame_count = 0;
      decode_time = 0;
    }

    for (int i = 0; i < nFrameReturned; i++) {
      pFrame = dec.GetFrame();
      // only save 200 frames
      if (nFrame < 2000) { fpOut.write(reinterpret_cast<char*>(pFrame), dec.GetFrameSize()); }
    }
  } while (nVideoBytes);
  fpOut.close();
  const char* aszDecodeOutFormat[] = {"NV12", "P016", "YUV444", "YUV444P16"};
  std::cout << "Total frame decoded: " << nFrame << std::endl << "Saved in file " << out_file_name << " in format " << aszDecodeOutFormat[dec.GetOutputFormat()] << std::endl;
  return 0;
}