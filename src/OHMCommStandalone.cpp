
#include <memory>
#include "OHMComm.h"
#include "AudioProcessorFactory.h"
#include "AudioHandlerFactory.h"

#include "config/FileConfiguration.h"
#include "config/InteractiveConfiguration.h"
#include "config/ParameterConfiguration.h"
#include "config/PassiveConfiguration.h"

int main(int argc, char* argv[])
{
    ////
    // Configuration
    ////

    std::unique_ptr<OHMComm> ohmComm;
    Parameters params(AudioHandlerFactory::getAudioHandlerNames(), AudioProcessorFactory::getAudioProcessorNames());
    if(params.parseParameters(argc, argv))
    {
        if(params.isParameterSet(Parameters::PASSIVE_CONFIGURATION))
        {
            NetworkConfiguration networkConfig{0};
            networkConfig.remoteIPAddress = params.getParameterValue(Parameters::REMOTE_ADDRESS);
            networkConfig.remotePort = atoi(params.getParameterValue(Parameters::REMOTE_PORT).data());
            networkConfig.localPort = atoi(params.getParameterValue(Parameters::LOCAL_PORT).data());
            ohmComm.reset(new OHMComm(new PassiveConfiguration(networkConfig)));
        }
        else if(params.isParameterSet(Parameters::CONFIGURATION_FILE))
        {
            const std::string configFile = params.getParameterValue(Parameters::CONFIGURATION_FILE);
            ohmComm.reset(new OHMComm(new FileConfiguration(configFile)));
        }
        else
        {
            ohmComm.reset(new OHMComm(new ParameterConfiguration(params)));
        }
    }
    else
    {
        ohmComm.reset(new OHMComm(new InteractiveConfiguration()));
    }

    ////
    // Startup
    ////

    if(!ohmComm->isConfigurationDone(true))
    {
        std::cerr << "Failed to configure OHMComm!" << std::endl;
        return 1;
    }
    ohmComm->startAudioThreads();

    char input;
    // wait for exit
    std::cout << "Type Enter to exit..." << std::endl;
    std::cin >> input;

    ohmComm->stopAudioThreads();

    return 0;
}
