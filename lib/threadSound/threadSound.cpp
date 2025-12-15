#include "threadSound.h"

#include <Drivers/STM32469I-Discovery/stm32469i_discovery_audio.h>

// Création des membres statiques
EventFlags ThreadSound::m_flags;
EventFlags ThreadSound::m_flagsError;
Thread *ThreadSound::m_mp3Decoder = nullptr;
Thread *ThreadSound::m_playSound = nullptr;
Thread *ThreadSound::m_garbage = nullptr;
FILE *ThreadSound::m_infile = nullptr;
HMP3Decoder ThreadSound::m_hMP3Decoder;
short ThreadSound::m_outBuf[2][MAX_NCHAN * MAX_NGRAN * MAX_NSAMP];
uint16_t ThreadSound::m_nbDatas[2];
int ThreadSound::m_sampleRate = 44100;
uint8_t ThreadSound::m_volume = 75;
bool ThreadSound::m_mono = false;

// Création de l'instance unique de la classe
ThreadSound *const ThreadSound::threadSound = new ThreadSound();

ThreadSound::ThreadSound() {
    BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_BOTH, m_volume, m_sampleRate);
}

void ThreadSound::destroy() {
    BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);
    if (m_garbage) {
        delete m_garbage;
    }
    m_garbage = new Thread;
    m_garbage->start(callback(garbage));
}

void ThreadSound::garbage() {
    if (m_mp3Decoder) {
        if (m_mp3Decoder->get_state() != Thread::Deleted) {
            m_mp3Decoder->terminate();
            m_mp3Decoder->join();
        }
        delete m_mp3Decoder;
        m_mp3Decoder = nullptr;
    }
    if (m_playSound) {
        if (m_playSound->get_state() != Thread::Deleted) {
            m_playSound->terminate();
            m_playSound->join();
        }
        delete m_playSound;
        m_playSound = nullptr;
    }
    if (m_infile) {
        fclose(m_infile);
        m_infile = nullptr;
    }
    ThisThread::sleep_for(100ms);
    m_flags.clear();
}

ThreadSound::ErrorSound ThreadSound::playMp3(const char *file) {
    ErrorSound error;
    // Teste si la ressource est disponible
    CriticalSectionLock::enable();
    if (m_flags.get() & FLAG_IS_PLAYING) {
        // Ressource occupée
        error = ERROR_RESSOURCE_USED;
    } else {
        // Ressource disponible
        error = NO_ERROR;
        // Prend la ressource
        m_flags.clear();
        m_flagsError.clear();
        m_flags.set(FLAG_IS_PLAYING);
    }
    CriticalSectionLock::disable();

    if (error != NO_ERROR) return error;

    m_infile = fopen(file, "rb");
    if (!m_infile) {
        m_flags.clear(FLAG_IS_PLAYING);
        return ERROR_FILE_NOT_FOUND;
    }
    m_mp3Decoder = new Thread;
    m_mp3Decoder->start(callback(runMp3Decoder));
    m_playSound = new Thread;
    m_playSound->start(callback(runPlaySound));
    return NO_ERROR;
}

ThreadSound::ErrorSound ThreadSound::stop() {
    if (m_flags.get() & FLAG_IS_PLAYING) {
        destroy();
        return NO_ERROR;
    }
    return ERROR_NOT_PLAYING;
}

ThreadSound::ErrorSound ThreadSound::pause() {
    uint32_t f = m_flags.get();
    if ((f & FLAG_IS_PLAYING) && !(f & FLAG_PAUSE)) {
        if (BSP_AUDIO_OUT_Pause() == AUDIO_ERROR) {
            return ERROR_PAUSE;
        }
        m_flags.set(FLAG_PAUSE);
        return NO_ERROR;
    }
    return ERROR_NOT_PLAYING;
}

ThreadSound::ErrorSound ThreadSound::resume() {
    uint32_t f = m_flags.get();
    if ((f & FLAG_IS_PLAYING) && (f & FLAG_PAUSE)) {
        if (BSP_AUDIO_OUT_Resume() == AUDIO_ERROR) {
            return ERROR_RESUME;
        }
        m_flags.clear(FLAG_PAUSE);
        return NO_ERROR;
    }
    return ERROR_NOT_PLAYING;
}

ThreadSound::ErrorSound ThreadSound::setVolume(uint8_t v) {
    if (v>100) {
        return ERROR_VOLUME;
    }
    if (BSP_AUDIO_OUT_SetVolume(v) == AUDIO_ERROR) {
        return ERROR_VOLUME;
    }
    m_volume = v;
    return NO_ERROR;
}

ThreadSound::ErrorSound ThreadSound::mute() {
    if (BSP_AUDIO_OUT_SetMute(AUDIO_MUTE_ON) == AUDIO_ERROR) {
        return ERROR_MUTE;
    }
    m_flags.set(FLAG_MUTE);
    return NO_ERROR;
}

ThreadSound::ErrorSound ThreadSound::unMute() {
    if (BSP_AUDIO_OUT_SetMute(AUDIO_MUTE_OFF) == AUDIO_ERROR) {
        return ERROR_MUTE;
    }
    m_flags.clear(FLAG_MUTE);
    return NO_ERROR;
}

bool ThreadSound::isPlaying()
{
    return bool(m_flags.get() & FLAG_IS_PLAYING);
}

#define READBUF_SIZE (1024 * 16) /* feel free to change this, but keep big enough for >= one frame at high bitrates */

int ThreadSound::fillReadBuffer(unsigned char *readBuf, unsigned char *readPtr, int bufSize, int bytesLeft) {
    int nRead;

    /* move last, small chunk from end of buffer to start, then fill with new data */
    memmove(readBuf, readPtr, bytesLeft);
    nRead = fread(readBuf + bytesLeft, 1, bufSize - bytesLeft, m_infile);
    /* zero-pad to avoid finding false sync word after last frame (from old data in readBuf) */
    if (nRead < bufSize - bytesLeft)
        memset(readBuf + bytesLeft + nRead, 0, bufSize - bytesLeft - nRead);

    return nRead;
}

void ThreadSound::runMp3Decoder() {
    static unsigned char readBuf[READBUF_SIZE];
    int bytesLeft = 0;
    int outOfData = 0;
    int eofReached = 0;
    unsigned char *readPtr = readBuf;
    int nRead = 0;
    int offset;
    int err;
    MP3FrameInfo mp3FrameInfo;
    int indexOutBuf = 0;
    short *outBuf = m_outBuf[0];

    if ((m_hMP3Decoder = MP3InitDecoder()) == 0) {
        m_flagsError.set(ERROR_MP3_DECODER);
        destroy();
        return;
    }

    m_flags.set(FLAG_PLAY_BUFFER1_RELEASE);

    do {
        /* somewhat arbitrary trigger to refill buffer - should always be enough for a full frame */
        if (bytesLeft < 2 * MAINBUF_SIZE && !eofReached) {
            nRead = fillReadBuffer(readBuf, readPtr, READBUF_SIZE, bytesLeft);
            bytesLeft += nRead;
            readPtr = readBuf;
            if (nRead == 0)
                eofReached = 1;
        }

        /* find start of next MP3 frame - assume EOF if no sync found */
        offset = MP3FindSyncWord(readPtr, bytesLeft);
        if (offset < 0) {
            outOfData = 1;
            break;
        }
        readPtr += offset;
        bytesLeft -= offset;

        /* decode one MP3 frame - if offset < 0 then bytesLeft was less than a full frame */
        err = MP3Decode(m_hMP3Decoder, &readPtr, &bytesLeft, outBuf, 0);

        if (err) {
            /* error occurred */
            switch (err) {
                case ERR_MP3_INDATA_UNDERFLOW:
                    outOfData = 1;
                    break;
                case ERR_MP3_MAINDATA_UNDERFLOW:
                    /* do nothing - next call to decode will provide more mainData */
                    break;
                case ERR_MP3_FREE_BITRATE_SYNC:
                default:
                    outOfData = 1;
                    break;
            }
        } else {
            /* no error */
            MP3GetLastFrameInfo(m_hMP3Decoder, &mp3FrameInfo);
            m_sampleRate = mp3FrameInfo.samprate;
            m_mono = mp3FrameInfo.nChans == 1;
            m_nbDatas[indexOutBuf] = mp3FrameInfo.bitsPerSample * mp3FrameInfo.outputSamps / 8;
            if (indexOutBuf == 0) {
                indexOutBuf = 1;
                m_flags.set(FLAG_MP3_DECODER_BUFFER0_READY);
                m_flags.wait_all(FLAG_PLAY_BUFFER1_RELEASE);
            } else {
                indexOutBuf = 0;
                m_flags.set(FLAG_MP3_DECODER_BUFFER1_READY);
                m_flags.wait_all(FLAG_PLAY_BUFFER0_RELEASE);
            }
            outBuf = m_outBuf[indexOutBuf];
        }

    } while (!outOfData);

    MP3FreeDecoder(m_hMP3Decoder);
    m_hMP3Decoder = nullptr;

    fclose(m_infile);
    m_infile = nullptr;
}

void ThreadSound::runPlaySound() {
    int nbTotalDatas = 0;

    if (m_flags.wait_all(FLAG_MP3_DECODER_BUFFER0_READY, 100) == osFlagsErrorTimeout) {
        // erreur ou fin
        destroy();
        return;
    }

    if (BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_BOTH, m_volume, m_sampleRate) != 0) {
        m_flagsError.set(ERROR_AUDIO_CODEC);
        destroy();
        return;
    }

    if (m_mono) BSP_AUDIO_OUT_ChangeAudioConfig(BSP_AUDIO_OUT_CIRCULARMODE | BSP_AUDIO_OUT_MONOMODE);

    /*
    Start playing the file from a circular buffer, once the DMA is enabled, it is
    always in running state. Application has to fill the buffer with the audio data
    using Transfer complete and/or half transfer complete interrupts callbacks
    (BSP_AUDIO_OUT_TransferComplete_CallBack() or BSP_AUDIO_OUT_HalfTransfer_CallBack()...
    */

    BSP_AUDIO_OUT_Play((uint16_t *)m_outBuf[0], m_nbDatas[0] * 2);

    while (1) {
        nbTotalDatas++;
        m_flags.wait_all(FLAG_HALF_BUFFER);
        m_flags.set(FLAG_PLAY_BUFFER0_RELEASE);
        if (!(m_flags.get() & FLAG_MP3_DECODER_BUFFER1_READY)) {
            break;
        }
        m_flags.clear(FLAG_MP3_DECODER_BUFFER1_READY);
        nbTotalDatas++;
        m_flags.wait_all(FLAG_FULL_BUFFER);
        m_flags.set(FLAG_PLAY_BUFFER1_RELEASE);
        if (!(m_flags.get() & FLAG_MP3_DECODER_BUFFER0_READY)) {
            break;
        }
        m_flags.clear(FLAG_MP3_DECODER_BUFFER0_READY);
    }

    // printf("Nb Datas %d x %d = %d\n", nbTotalDatas, datas, nbTotalDatas * datas);
    
    // Libère la ressource
    destroy();
}

/*------------------------------------------------------------------------------
       Callbacks implementation:
           the callbacks API are defined __weak in the stm32769i_discovery_audio.c file
           and their implementation should be done the user code if they are needed.
           Below some examples of callback implementations.
  ----------------------------------------------------------------------------*/
/**
 * @brief  Manages the full Transfer complete event.
 * @param  None
 * @retval None
 */
void BSP_AUDIO_OUT_TransferComplete_CallBack(void) {
    if (ThreadSound::m_flags.get() & ThreadSound::FLAG_IS_PLAYING) {
        ThreadSound::m_flags.set(ThreadSound::FLAG_FULL_BUFFER);
    }
}

/**
 * @brief  Manages the DMA Half Transfer complete event.
 * @param  None
 * @retval None
 */
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void) {
    if (ThreadSound::m_flags.get() & ThreadSound::FLAG_IS_PLAYING) {
        ThreadSound::m_flags.set(ThreadSound::FLAG_HALF_BUFFER);
    }
}

// /**
//  * @brief  Manages the DMA FIFO error event.
//  * @param  None
//  * @retval None
//  */
void BSP_AUDIO_OUT_Error_CallBack(void) {
    ThreadSound::m_flagsError.set(ThreadSound::ERROR_DMA);
}
