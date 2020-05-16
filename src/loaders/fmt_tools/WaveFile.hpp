//
// Created by sebastian on 13/5/20.
//

#ifndef SDL_CRT_FILTER_WAVEFILE_HPP
#define SDL_CRT_FILTER_WAVEFILE_HPP

#include <string>

struct  WAV_HEADER_T {
    /* RIFF Chunk Descriptor */
    uint8_t         RIFF[4];        // RIFF Header Magic header
    uint32_t        ChunkSize;      // RIFF Chunk Size
    uint8_t         WAVE[4];        // WAVE Header
    /* "fmt" sub-chunk */
    uint8_t         fmt[4];         // FMT header
    uint32_t        SubChunk1Size;  // Size of the fmt chunk
    uint16_t        AudioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    uint16_t        NumOfChan;      // Number of channels 1=Mono 2=Sterio
    uint32_t        SamplesPerSec;  // Sampling Frequency in Hz
    uint32_t        bytesPerSec;    // bytes per second
    uint16_t        blockAlign;     // 2=16-bit mono, 4=16-bit stereo
    uint16_t        bitsPerSample;  // Number of bits per sample
    /* "data" sub-chunk */
    uint8_t         Subchunk2ID[4]; // "data"  string
    uint32_t        SubChunk2Size;  // Sampled data length
};

class WaveIO {
protected:
    WAV_HEADER_T header;
    void initialize_commons();
    void compute_fields();
    std::string bytearray_to_string(void* data) { return bytearray_to_string(data, 4); };
    std::string bytearray_to_string(void* data, size_t len);
public:
    void setFM() { setFormat(1, 48e3, 8); }
    void setFormat(uint32_t channels, uint32_t sample_rate, uint32_t depth);
    void write(std::string filename, void* data, size_t len);
    void read(std::string filename, void* data);
    std::string getInfo();
};

void WaveIO::initialize_commons() {
    std::string RIFF = { 0x52, 0x49, 0x46, 0x46 }; //RIFF
    mempcpy(header.RIFF, RIFF.c_str(), RIFF.length() );
    std::string FMT = { 0x57, 0x41, 0x56, 0x45 }; // "WAVE
    mempcpy(header.WAVE, FMT.c_str(), FMT.length() );
    std::string SUBFMT = { 0x66, 0x6d, 0x74, 0x20 }; // "fmt "
    mempcpy(header.fmt, SUBFMT.c_str(), SUBFMT.length() );
    header.SubChunk1Size = 16; //PCM
    header.AudioFormat = 1; //PCM
    std::string DATA = { 0x64, 0x61, 0x74, 0x61 }; //Data
    mempcpy(header.Subchunk2ID, DATA.c_str(), DATA.length() );
}

void WaveIO::setFormat(uint32_t channels, uint32_t sample_rate, uint32_t depth) {
    initialize_commons();
    header.NumOfChan = channels;
    header.SamplesPerSec = sample_rate;
    header.bitsPerSample = depth;
    compute_fields();
}

void WaveIO::compute_fields() {
    header.blockAlign  = header.NumOfChan * header.bitsPerSample / 8;
    header.bytesPerSec = header.SamplesPerSec * header.blockAlign;
}

void WaveIO::write(std::string filename, void *data, size_t len) {
    header.SubChunk2Size = len;
    header.ChunkSize = 4 + (8 + header.SubChunk1Size) + (8 + header.SubChunk2Size);
    std::ofstream file;
    file.open(filename, std::ios::out | std::ios::binary );
    if(file.is_open()) {
        file.write((char *) &header, sizeof(WAV_HEADER_T));
        file.write((char *) data, len);
    } else
        SDL_Log("Cannot open file %s for write", filename.c_str() );
    file.close();
}

void WaveIO::read(std::string filename, void *data) {
    std::ifstream file;
    file.open(filename, std::ios::in | std::ios::binary );
    if(file.is_open()) {
        file.read((char *) &header, sizeof(WAV_HEADER_T));
        file.read( (char *) data, header.SubChunk2Size );
    } else
        SDL_Log("Cannot open file %s for read", filename.c_str() );
    file.close();
}


std::string WaveIO::getInfo() {
    std::ostringstream info;
    info << "Riff header: " << bytearray_to_string(header.RIFF) << std::endl;
    info << "Chunk Size: " << header.ChunkSize << std::endl;
    info << "Wave header: " << bytearray_to_string(header.WAVE) << std::endl;
    info << "Fmt header: " << bytearray_to_string(header.fmt) << std::endl;
    info << "Subchunk 1 size: " << header.SubChunk1Size << std::endl;
    info << "Audio format: " << header.AudioFormat << std::endl;
    info << "Channels: " << header.NumOfChan << std::endl;
    info << "Sample rate: " << header.SamplesPerSec << std::endl;
    info << "ByteRate: " << header.bytesPerSec << std::endl;
    info << "Block Align: " << header.blockAlign << std::endl;
    info << "Depth: " << header.bitsPerSample << std::endl;
    info << "Subchunk 2 id: " << bytearray_to_string(header.Subchunk2ID) << std::endl;
    info << "Subchunk 2 size: " << header.SubChunk2Size << std::endl;
    return info.str();
}

std::string WaveIO::bytearray_to_string(void *data, size_t size) {
    char* copy = new char[size + 1];
    memcpy(copy, data, size );
    copy[size] = 0x00;
    std::string str_copied(copy);
    delete [] copy;
    return str_copied;
}


#endif //SDL_CRT_FILTER_WAVEFILE_HPP
