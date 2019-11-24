#define DR_FLAC_IMPLEMENTATION

#include <iostream>
#include <unistd.h>
#include <vector>

#include "/usr/local/include/FLAC++/decoder.h"
#include "/usr/local/include/FLAC++/encoder.h"
#include "/usr/local/include/FLAC++/export.h"
#include "/usr/local/include/FLAC++/metadata.h"

#include "lib/dr_flac.h"

#define SAMPLE_RATE 44100

#define NUM_CHUNKS 800
#define NUM_LEVELS 600

int main()
{
    // libflac++
    FLAC__StreamDecoder *fileDecoder;

    // dr_flac
    drflac *pFlac = drflac_open_file("music/zeldas_theme.flac", NULL);
    if (pFlac == NULL)
    {
        std::cout << "read err: filename not found" << std::endl;
        return -1;
    }

    int32_t spectrogram[NUM_CHUNKS][NUM_LEVELS];

    for (int i = 0; i < NUM_CHUNKS; i++)
    {
        for (int j = 0; j < NUM_LEVELS; j++)
        {
            spectrogram[i][j] = 0;
        }
    }

    int32_t *pSampleData = (int32_t *)malloc((size_t)pFlac->totalPCMFrameCount * pFlac->channels * sizeof(int32_t));
    drflac_read_pcm_frames_s32(pFlac, pFlac->totalPCMFrameCount, pSampleData);

    int32_t CHUNK_SIZE = pFlac->totalPCMFrameCount / NUM_CHUNKS;

    std::cout << "num blocks: " << pFlac->totalPCMFrameCount / CHUNK_SIZE << std::endl;
    std::cout << "num levels: " << NUM_LEVELS << std::endl;
    std::cout << "num samples per chunk: " << CHUNK_SIZE << std::endl;

    std::vector<int32_t> samples;

    for (int i = 0; i < NUM_CHUNKS; i++)
    {
        for (int j = 0; j < CHUNK_SIZE; j++)
        {
            // std::cout << pSampleData[(i * CHUNK_SIZE) + j] << std::endl;
            // std::cout << ((i * CHUNK_SIZE) + j) << std::endl;
            samples.push_back(pSampleData[(i * CHUNK_SIZE) + j]);
        }
    }

    std::cout << "num samples: " << samples.size() << std::endl;

    drflac_close(pFlac);

    std::cout << "file closed" << std::endl;

    return 0;
}