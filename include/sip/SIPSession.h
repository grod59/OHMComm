/* 
 * File:   SIPSession.h
 * Author: doe300
 *
 * Created on June 12, 2016, 12:44 PM
 */

#ifndef SIPSESSION_H
#define SIPSESSION_H

#include <memory>
#include <functional>

#include "configuration.h"
#include "rtp/ParticipantDatabase.h"
#include "sip/SIPPackageHandler.h"
#include "sip/SDPMessageHandler.h"
#include "sip/SIPUserAgent.h"
#include "network/NetworkWrapper.h"
#include "Parameters.h"

namespace ohmcomm
{
    namespace sip
    {

        class SIPSession : public rtp::ParticipantListener
        {
        public:
            
            enum class SessionState
            {
                /*!
                 * Unknown status, we haven't established contact yet
                 */
                DISCONNECTED,
                /*!
                 * We sent an INVITE and are waiting for a response
                 */
                INVITING,
                /*!
                 * Session is established, communication is up and running.
                 * Don't accept any further INVITEs, only BYE
                 */
                ESTABLISHED,
                /*!
                 * We had a session and shut it down. Or we failed to initialize a session at all
                 */
                SHUTDOWN,
            };
            
            SIPSession(const ohmcomm::NetworkConfiguration& sipConfig, const std::string& remoteUser);
            virtual ~SIPSession();
            
            /*!
             * Sets additional user-info for the local user
             * 
             * \since 1.0
             */
            void setUserInfo(const Parameters& params);
            
            void onRemoteConnected(const unsigned int ssrc, const std::string& address, const unsigned short port) override;
            void onRemoteRemoved(const unsigned int ssrc) override;
            
        protected:
            UserAgentDatabase userAgents;
            std::unique_ptr<ohmcomm::network::NetworkWrapper> network;
            SessionState state;
            
            virtual void shutdownInternal() = 0;
            
        };
    }
}
#endif /* SIPSESSION_H */

