#ifndef AUDIOHANDLERFACTORY
#define	AUDIOHANDLERFACTORY

#include "AudioHandlerFactory.h"
#include "RTAudioWrapper.h"
#include <locale>

/*!
 * Factory-class to provide an audio-handler object without needing to know the details of the implementation.
 */
class AudioHandlerFactory
{
public:
    static auto getAudioHandler(std::string name, AudioConfiguration &audioConfig)->std::unique_ptr<AudioHandler>;
    static auto getAudioHandler(std::string name) -> std::unique_ptr<AudioHandler>;
private:
    static auto stringToUpperCase(const std::string& s)->std::string;
};

#endif