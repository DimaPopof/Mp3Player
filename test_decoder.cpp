#define MINIAUDIO_IMPLEMENTATION
#include "src/miniaudio.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    ma_decoder decoder;
    ma_decoder_config config = ma_decoder_config_init_default();
    config.format = ma_format_f32;
    config.channels = 2;
    config.sampleRate = 44100;
    if (ma_decoder_init_file(argv[1], &config, &decoder) != MA_SUCCESS) {
        std::cout << "Failed to open" << std::endl;
        return 1;
    }
    std::cout << "Opened " << argv[1] << std::endl;
    ma_uint64 length;
    ma_decoder_get_length_in_pcm_frames(&decoder, &length);
    std::cout << "Length: " << length << " frames" << std::endl;
    ma_decoder_uninit(&decoder);
    return 0;
}
