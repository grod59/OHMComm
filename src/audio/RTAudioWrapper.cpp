
#ifdef RTAUDIO_HEADER //Only compile, if RtAudio is linked

#include "Logger.h"
#include "audio/RTAudioWrapper.h"
#include "Statistics.h"

using namespace ohmcomm;

// constructor
RtAudioWrapper::RtAudioWrapper() : AudioHandler(), bufferAudioOutput(nullptr), streamData(new StreamData())
{
}

// constructor
RtAudioWrapper::RtAudioWrapper(const AudioConfiguration &audioConfig) : RtAudioWrapper()
{
    this->setConfiguration(audioConfig);
}

RtAudioWrapper::~RtAudioWrapper()
{
    delete streamData;
    delete[] (char*)bufferAudioOutput;
}

// static callbackHelper (calls the callback of the object)
auto RtAudioWrapper::callbackHelper(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *rtAudioWrapperObject) -> int
{
    RtAudioWrapper *rtAudioWrapper = static_cast <RtAudioWrapper*> (rtAudioWrapperObject);
    return rtAudioWrapper->callback(outputBuffer, inputBuffer, nBufferFrames, streamTime, status, nullptr);
}

// callback of the object
auto RtAudioWrapper::callback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *rtAudioWrapperObject) -> int
{
    if (status == RTAUDIO_INPUT_OVERFLOW)
    {
        // TODO: Create a log. Input data was discarded because of an overflow (data loss)
        ohmcomm::warn("RtAudio") << "Overflow" << ohmcomm::endl;
    }

    if (status == RTAUDIO_OUTPUT_UNDERFLOW)
    {
        // TODO: Create a log. Output buffer ran low, produces a break in the output sound.
        ohmcomm::warn("RtAudio") << "Underflow" << ohmcomm::endl;
    }

    this->streamData->nBufferFrames = nBufferFrames;
    //streamTime is the number of seconds since start of stream, so we convert to number of microseconds
    this->streamData->streamTime = lround(streamTime * 1000000);
    Statistics::setCounter(Statistics::TOTAL_ELAPSED_MILLISECONDS, streamTime * 1000);
    Statistics::incrementCounter(Statistics::COUNTER_PAYLOAD_BYTES_RECORDED, inputBufferByteSize);
    Statistics::incrementCounter(Statistics::COUNTER_FRAMES_RECORDED, nBufferFrames);
    Statistics::incrementCounter(Statistics::COUNTER_PAYLOAD_BYTES_OUTPUT, outputBufferByteSize);
    Statistics::incrementCounter(Statistics::COUNTER_FRAMES_OUTPUT, nBufferFrames);

    //in case the two buffer sizes max vary
    this->streamData->maxBufferSize = inputBufferByteSize;
    this->streamData->isSilentPackage = false;
    if (inputBuffer != nullptr)
        processors.processAudioInput(inputBuffer, inputBufferByteSize, streamData);

    //in case the two buffer sizes max vary
    this->streamData->maxBufferSize = outputBufferByteSize;
    if (outputBuffer != nullptr)
        processors.processAudioOutput(outputBuffer, outputBufferByteSize, streamData);

    return 0;
}

void RtAudioWrapper::startHandler(const PlaybackMode mode)
{
    if (this->flagPrepared)
    {
        unsigned int bufferFrames = audioConfiguration.framesPerPackage;
        RtAudio::StreamParameters* inputParams = (mode & PlaybackMode::INPUT) == PlaybackMode::INPUT ? &input : nullptr;
        RtAudio::StreamParameters* outputParams = (mode & PlaybackMode::OUTPUT) == PlaybackMode::OUTPUT ? &output : nullptr;
        this->rtaudio.openStream(outputParams, inputParams, audioConfiguration.audioFormatFlag, audioConfiguration.sampleRate, &bufferFrames, &RtAudioWrapper::callbackHelper, this);
        //RtAudio can update buffer-size, but some processors (i.e. Opus) require certain fixed buffer-sizes
        //so disallow RtAudio to change the buffer-size
        if(bufferFrames != audioConfiguration.framesPerPackage)
        {
            throw ohmcomm::configuration_error("RtAudio", "Invalid buffer-size update!");
        }
        this->rtaudio.startStream();
    }
    else
        ohmcomm::warn("RtAudio") << "Did you forget to call AudioHandler::prepare()?" << ohmcomm::endl;
}

void RtAudioWrapper::setConfiguration(const AudioConfiguration &audioConfig)
{
    this->audioConfiguration = audioConfig;
    this->flagAudioConfigSet = true;
}

void RtAudioWrapper::suspend()
{
    if (this->rtaudio.isStreamRunning())
    {
        this->rtaudio.stopStream();
    }	
}

void RtAudioWrapper::stop()
{
    this->suspend();
    //FIXME ALSA-API throws "duplicate free or corruption" in closeStream()
    this->rtaudio.closeStream();
    processors.cleanUpAudioProcessors();
}

void RtAudioWrapper::resume()
{
    if (this->rtaudio.isStreamOpen() && this->rtaudio.isStreamRunning() == false)
        this->rtaudio.startStream();
}

void RtAudioWrapper::reset()
{
    this->stop();
    this->audioConfiguration = { 0 };
    this->flagAudioConfigSet = false;
    this->flagPrepared = false;
}


// Region: private functions
auto RtAudioWrapper::initRtAudioStreamParameters() -> bool
{
    // calculate the input- and output buffer sizes
    this->outputBufferByteSize = audioConfiguration.framesPerPackage * audioConfiguration.outputDeviceChannels * getAudioFormatByteSize(audioConfiguration.audioFormatFlag);
    this->inputBufferByteSize = audioConfiguration.framesPerPackage * audioConfiguration.inputDeviceChannels * getAudioFormatByteSize(audioConfiguration.audioFormatFlag);

    /* internal buffer for playback data */
    this->bufferAudioOutput = new char[this->outputBufferByteSize];

    /* Prepare the StreamParameters */
    this->input.deviceId = audioConfiguration.inputDeviceID;
    this->output.deviceId = audioConfiguration.outputDeviceID;
    this->input.nChannels = audioConfiguration.inputDeviceChannels;
    this->output.nChannels = audioConfiguration.outputDeviceChannels;
    
    //set playback-mode
    audioConfiguration.playbackMode = (PlaybackMode)((input.deviceId != AudioConfiguration::INVALID_DEVICE ? PlaybackMode::INPUT : 0) | 
            (output.deviceId != AudioConfiguration::INVALID_DEVICE ? PlaybackMode::OUTPUT : 0));
    return true;
}

void RtAudioWrapper::setDefaultAudioConfig()
{
    AudioConfiguration audioConfig{};
    audioConfig.inputDeviceID = this->rtaudio.getDefaultInputDevice();
    audioConfig.outputDeviceID = this->rtaudio.getDefaultOutputDevice();

    //input device
//    RtAudio::DeviceInfo inputDeviceInfo = this->rtaudio.getDeviceInfo(audioConfig.inputDeviceID);
//    RtAudio::DeviceInfo outputDeviceInfo = this->rtaudio.getDeviceInfo(audioConfig.outputDeviceID);

    audioConfig.inputDeviceChannels = 2;
    audioConfig.outputDeviceChannels = 2;
    //RtAudioFormat rtaudioFormat = autoSelectAudioFormat(outputDeviceInfo.nativeFormats);
    //audioConfig.audioFormat = getAudioFormatByteSize(rtaudioFormat);
    //audioFormat sampleRate and bufferFrames are overridden by queryProcessorSupport()
    audioConfig.audioFormatFlag = 0;
    audioConfig.sampleRate = 0;
    audioConfig.framesPerPackage = 0;

    this->setConfiguration(audioConfig);
}

auto RtAudioWrapper::getAudioFormatByteSize(RtAudioFormat rtaudioFormat) -> int
{
    switch (rtaudioFormat)
    {
        case(RTAUDIO_SINT8): return 1;
        case(RTAUDIO_SINT16) : return 2;
        case(RTAUDIO_SINT24) : return 3;
        case(RTAUDIO_SINT32) : return 4;
        case(RTAUDIO_FLOAT32) : return 4;
        case(RTAUDIO_FLOAT64) : return 8;
    }
    return 0;
}

auto RtAudioWrapper::getOutputFrameSize() -> int
{
    if (this->flagAudioConfigSet)
        return getAudioFormatByteSize(this->audioConfiguration.audioFormatFlag) * this->audioConfiguration.outputDeviceChannels;
    return 0;
}

auto RtAudioWrapper::getInputFrameSize() -> int
{
	if (this->flagAudioConfigSet)
		return getAudioFormatByteSize(this->audioConfiguration.audioFormatFlag) * this->audioConfiguration.inputDeviceChannels;
	return 0;
}

bool RtAudioWrapper::prepare(const std::shared_ptr<ConfigurationMode> configMode)
{
    /* If there is no config set, then load the default */
    if (this->flagAudioConfigSet == false)
        this->setDefaultAudioConfig();
    
    //checks if there is a configuration all processors support
    if(!processors.queryProcessorSupport(audioConfiguration, getAudioDevices()[audioConfiguration.inputDeviceID]))
    {
        ohmcomm::error("RtAudio") << "AudioProcessors could not agree on configuration!" << ohmcomm::endl;
        return false;
    }
    
    bool resultA = this->initRtAudioStreamParameters();
    bool resultB = processors.configureAudioProcessors(audioConfiguration, configMode, outputBufferByteSize);

    if (resultA && resultB) {
        this->flagPrepared = true;
        return true;
    }

    return false;
}

const std::vector<AudioDevice>& RtAudioWrapper::getAudioDevices()
{
    static std::vector<AudioDevice> devices{};
    
    if(devices.empty())
    {
        //initial add all audio devices
        unsigned int availableAudioDevices = rtaudio.getDeviceCount();
        for(unsigned int i = 0; i < availableAudioDevices; i++)
        {
            RtAudio::DeviceInfo deviceInfo = rtaudio.getDeviceInfo(i);
            if(deviceInfo.probed)
            {
                //although there is a value for native audio-formats in the device-info, RtAudio documentation says:
                // "However, RtAudio will automatically provide format conversion if a particular format is not natively supported."
                devices.push_back({deviceInfo.name, deviceInfo.outputChannels, deviceInfo.inputChannels, 
                        deviceInfo.isDefaultOutput, deviceInfo.isDefaultInput, (unsigned int)deviceInfo.nativeFormats, true, deviceInfo.sampleRates});
            }
        }
    }
    
    return devices;
}

#endif /* RTAUDIO_HEADER */