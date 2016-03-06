/* 
 * File:   ProcessorManager.cpp
 * Author: daniel
 * 
 * Created on February 22, 2016, 11:17 AM
 */

#include "ProcessorManager.h"
#include "ProfilingAudioProcessor.h"

ProcessorManager::ProcessorManager() : audioProcessors()
{

}

void ProcessorManager::printAudioProcessorOrder(std::ostream& OutputStream) const
{
    for (const auto& processor : audioProcessors) {
        OutputStream << processor->getName() << std::endl;
    }
}

bool ProcessorManager::addProcessor(AudioProcessor *audioProcessor)
{
    if (hasAudioProcessor(audioProcessor) == false) {
        audioProcessors.push_back(std::unique_ptr<AudioProcessor>(audioProcessor));
        if (ProfilingAudioProcessor * profiler = dynamic_cast<ProfilingAudioProcessor*> (audioProcessor)) {
            //if this is a profiling processor, we need to add it to statistics to be printed on exit
            Statistics::addProfiler(profiler);
        }
        return true; // Successful added
    }
    return false;
}

bool ProcessorManager::removeAudioProcessor(AudioProcessor *audioProcessor)
{
    for (size_t i = 0; i < audioProcessors.size(); i++) {
        if ((audioProcessors.at(i))->getName() == audioProcessor->getName()) {
            audioProcessors.erase(audioProcessors.begin() + i);
            if (ProfilingAudioProcessor * profiler = dynamic_cast<ProfilingAudioProcessor*> (audioProcessor)) {
                Statistics::removeProfiler(profiler);
            }
            return true; // Successful removed
        }
    }
    return false;
}

bool ProcessorManager::removeAudioProcessor(std::string nameOfAudioProcessor)
{
    for (size_t i = 0; i < audioProcessors.size(); i++) {
        if (audioProcessors.at(i)->getName() == nameOfAudioProcessor) {
            if (ProfilingAudioProcessor * profiler = dynamic_cast<ProfilingAudioProcessor*> (audioProcessors.at(i).get())) {
                Statistics::removeProfiler(profiler);
            }
            audioProcessors.erase(audioProcessors.begin() + i);
            return true; // Successful removed
        }
    }
    return false;
}

bool ProcessorManager::clearAudioProcessors()
{
    audioProcessors.clear();
    Statistics::removeAllProfilers();
    return true;
}

unsigned int ProcessorManager::processAudioOutput(void *outputBuffer, const unsigned int &outputBufferByteSize, StreamData *streamData)
{
    unsigned int bufferSize = outputBufferByteSize;
    for (unsigned int i = audioProcessors.size(); i > 0; i--) {
        bufferSize = audioProcessors.at(i - 1)->processOutputData(outputBuffer, bufferSize, streamData);
    }
    return bufferSize;
}

unsigned int ProcessorManager::processAudioInput(void *inputBuffer, const unsigned int &inputBufferByteSize, StreamData *streamData)
{
    unsigned int bufferSize = inputBufferByteSize;
    for (unsigned int i = 0; i < audioProcessors.size(); i++) {
        bufferSize = audioProcessors.at(i)->processInputData(inputBuffer, bufferSize, streamData);
    }
    return bufferSize;
}

bool ProcessorManager::configureAudioProcessors(const AudioConfiguration& audioConfiguration, const std::shared_ptr<ConfigurationMode> configMode, const uint16_t bufferSize)
{
    for (const auto& processor : audioProcessors) {
        std::cout << "Configuring audio-processor '" << processor->getName() << "'..." << std::endl;
        bool result = processor->configure(audioConfiguration, configMode, bufferSize);
        if (result == false) // Configuration failed
            return false;
    }
    return true;
}

void ProcessorManager::startupAudioProcessors()
{
    for (const auto& processor : audioProcessors) {
        processor->startup();
    }
}

bool ProcessorManager::cleanUpAudioProcessors()
{
    for (const auto& processor : audioProcessors) {
        bool result = processor->cleanUp();
        if (result == false) // Cleanup failed
            return false;
    }
    return true;
}

bool ProcessorManager::hasAudioProcessor(AudioProcessor *audioProcessor) const
{
    for (const auto& processor : audioProcessors) {
        if (processor->getName() == audioProcessor->getName())
            return true;
    }
    return false;
}

bool ProcessorManager::hasAudioProcessor(std::string nameOfAudioProcessor) const
{
    for (const auto& processor : audioProcessors) {
        if (processor->getName() == nameOfAudioProcessor)
            return true;
    }
    return false;
}

unsigned int ProcessorManager::findOptimalBufferSize(unsigned int defaultBufferSize, unsigned int sampleRate)
{
    //get all supported buffer-sizes
    std::vector<std::vector<int>> processorBufferSizes(audioProcessors.size());
    for (unsigned int i = 0; i < audioProcessors.size(); i++) {
        processorBufferSizes[i] = audioProcessors[i]->getSupportedBufferSizes(sampleRate);
    }
    //number of processors supporting all buffer-sizes
    std::vector<bool> processorSupportAll(processorBufferSizes.size(), false);
    //iterate through all priorities and check if the value is supported by other processors
    for (unsigned int sizeIndex = 0; sizeIndex < 32; sizeIndex++) {
        for (unsigned int procIndex = 0; procIndex < processorBufferSizes.size(); procIndex++) {
            //processor has no size-entries left
            if (processorBufferSizes[procIndex].size() <= sizeIndex) {
                continue;
            }
            if (processorBufferSizes[procIndex][sizeIndex] == AudioProcessor::BUFFER_SIZE_ANY) {
                processorSupportAll[procIndex] = true;
                continue;
            }
            //"suggest" buffer-size
            int suggestedBufferSize = processorBufferSizes[procIndex][sizeIndex];
            //check "suggested" buffer-size against all other processors
            bool suggestionAccepted = true;
            for (unsigned int checkProcIndex = 0; checkProcIndex < processorBufferSizes.size(); checkProcIndex++) {
                bool singleProcessorAccepted = false;
                //we already checked all sizes below sizeIndex
                for (unsigned int checkSizeIndex = sizeIndex; checkSizeIndex < processorBufferSizes[checkProcIndex].size(); checkSizeIndex++) {
                    if (processorBufferSizes[checkProcIndex][checkSizeIndex] == suggestedBufferSize || processorBufferSizes[checkProcIndex][checkSizeIndex] == AudioProcessor::BUFFER_SIZE_ANY) {
                        singleProcessorAccepted = true;
                        //break;
                    }
                }
                if (!singleProcessorAccepted) {
                    suggestionAccepted = false;
                    //break;
                }
            }
            if (suggestionAccepted) {
                return suggestedBufferSize;
            }
        }
    }
    //if all processors only have BUFFER_SIZE_ALL, return default-value
    bool supportAll = true;
    for (unsigned int procIndex = 0; procIndex < processorSupportAll.size(); procIndex++) {
        if (!processorSupportAll[procIndex]) {
            supportAll = false;
            break;
        }
    }
    if (supportAll) {
        return defaultBufferSize;
    }

    return 0;
}

bool ProcessorManager::queryProcessorSupport(AudioConfiguration& audioConfiguration, const AudioDevice& inputDevice)
{
    //preset supported audio-formats with device-supported formats
    //although there is a value for native audio-formats in the device-info, RtAudio documentation says:
    // "However, RtAudio will automatically provide format conversion if a particular format is not natively supported."
    unsigned int supportedFormats = AudioConfiguration::AUDIO_FORMAT_ALL;
    if (audioConfiguration.forceAudioFormatFlag != 0) {
        //force the specific audio-format
        supportedFormats = audioConfiguration.forceAudioFormatFlag;
    }
    //preset supported sample-rates with device-supported rates
    unsigned int supportedSampleRates = mapDeviceSampleRates(inputDevice.sampleRates);
    if (audioConfiguration.forceSampleRate != 0) {
        //force the specific sample-rate
        supportedSampleRates = mapDeviceSampleRates({audioConfiguration.forceSampleRate});
    }
    for (unsigned int i = 0; i < audioProcessors.size(); i++) {
        // a & b return all bits set in a AND b -> all flags supported by both
        supportedFormats = supportedFormats & audioProcessors.at(i)->getSupportedAudioFormats();
        supportedSampleRates = supportedSampleRates & audioProcessors.at(i)->getSupportedSampleRates();
    }
    if (supportedFormats == 0) {
        //there is no format supported by all processors
        std::cerr << "Could not find a single audio-format supported by all processors!" << std::endl;
        return false;
    }
    if (supportedSampleRates == 0) {
        //there is no sample-rate supported by all processors
        std::cerr << "Could not find a single sample-rate supported by all processors!" << std::endl;
        return false;
    }
    audioConfiguration.audioFormatFlag = autoSelectAudioFormat(supportedFormats);
    audioConfiguration.sampleRate = AudioConfiguration::flagToSampleRate(supportedSampleRates);

    //find common supported buffer-size, defaults to 512
    int supportedBufferSize = findOptimalBufferSize(512, audioConfiguration.sampleRate);
    if (supportedBufferSize == 0) {
        std::cerr << "Could not find a single buffer-size supported by all processors!" << std::endl;
        return false;
    }
    audioConfiguration.framesPerPackage = supportedBufferSize;

    std::cout << "Using audio-format: " << AudioConfiguration::getAudioFormatDescription(audioConfiguration.audioFormatFlag, false) << std::endl;
    std::cout << "Using a sample-rate of " << audioConfiguration.sampleRate << " Hz" << std::endl;
    std::cout << "Using a buffer-size of " << supportedBufferSize << " samples (" << (supportedBufferSize * 1000 / audioConfiguration.sampleRate) << " ms)" << std::endl;

    return true;
}

const ProcessorCapabilities ProcessorManager::getCombinedCapabilities()
{
    ProcessorCapabilities caps = {false, false, false, false, false, 0, 0};
    for(const std::unique_ptr<AudioProcessor>& proc : audioProcessors)
    {
        const ProcessorCapabilities& procCaps = proc->getCapabilities();
        if(procCaps.isCodec)
            //there is at least one audio-codec
            caps.isCodec = true;
        if(procCaps.canDetectSilence)
            //at least one processor can detect silence
            caps.canDetectSilence = true;
        if(procCaps.canHandleSilence)
            //at least one processor can handle silence-packages in some special way
            caps.canHandleSilence = true;
        if(procCaps.generatesComfortNoise)
            //at least one processor generates comfort noise
            caps.generatesComfortNoise = true;
        if(procCaps.usesForwardErrorCorrection)
            //at least one processor uses FEC
            caps.usesForwardErrorCorrection = true;
        if(procCaps.maximumBandwidth > caps.maximumBandwidth)
            //return maximum of all bandwidths
            caps.maximumBandwidth = procCaps.maximumBandwidth;
        //make a list of all supported resample-rates
        caps.supportedResampleRates |= procCaps.supportedResampleRates;
    }
    return caps;
}

unsigned int ProcessorManager::autoSelectAudioFormat(unsigned int supportedFormats)
{
    if ((supportedFormats & AudioConfiguration::AUDIO_FORMAT_FLOAT64) == AudioConfiguration::AUDIO_FORMAT_FLOAT64) {
        return AudioConfiguration::AUDIO_FORMAT_FLOAT64;
    }
    if ((supportedFormats & AudioConfiguration::AUDIO_FORMAT_FLOAT32) == AudioConfiguration::AUDIO_FORMAT_FLOAT32) {
        return AudioConfiguration::AUDIO_FORMAT_FLOAT32;
    }
    if ((supportedFormats & AudioConfiguration::AUDIO_FORMAT_SINT32) == AudioConfiguration::AUDIO_FORMAT_SINT32) {
        return AudioConfiguration::AUDIO_FORMAT_SINT32;
    }
    if ((supportedFormats & AudioConfiguration::AUDIO_FORMAT_SINT24) == AudioConfiguration::AUDIO_FORMAT_SINT24) {
        return AudioConfiguration::AUDIO_FORMAT_SINT24;
    }
    if ((supportedFormats & AudioConfiguration::AUDIO_FORMAT_SINT16) == AudioConfiguration::AUDIO_FORMAT_SINT16) {
        return AudioConfiguration::AUDIO_FORMAT_SINT16;
    }
    //fall back to worst quality
    return AudioConfiguration::AUDIO_FORMAT_SINT8;
}

unsigned int ProcessorManager::mapDeviceSampleRates(std::vector<unsigned int> sampleRates)
{
    unsigned int sampleRate;
    unsigned int sampleRatesFlags = 0;
    for (unsigned int i = 0; i < sampleRates.size(); i++) {
        sampleRate = sampleRates.at(i);
        if(AudioConfiguration::sampleRateToFlag(sampleRate) == 0)
            std::cout << "Unrecognized sample-rate: " << sampleRate << std::endl;
        else
            sampleRatesFlags |= AudioConfiguration::sampleRateToFlag(sampleRate);
    }

    return sampleRatesFlags;
}