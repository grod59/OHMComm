/*
 * File:   ReceiveThread.h
 * Author: daniel
 *
 * Created on May 16, 2015, 12:49 PM
 */

#ifndef RTPLISTENER_H
#define	RTPLISTENER_H

#include <thread>

#include "ParticipantDatabase.h"
#include "RTPBufferHandler.h"
#include "network/NetworkWrapper.h"
#include "JitterBuffers.h"

namespace ohmcomm
{
    namespace rtp
    {

        /*!
         * Listening-thread for incoming RTP-packages
         *
         * This class starts a new thread which writes all received RTP-packages to the RTPBuffer.
         * 
         * The RTP-listener instance is managed by the ProcessorRTP
         */
        class RTPListener
        {
        public:
            /*!
             * Constructs a new RTPListener
             *
             * \param wrapper The NetworkWrapper to use for receiving packages
             *
             * \param buffers The buffers to write into
             *
             * \param receiveBufferSize The maximum size (in bytes) a RTP-package can fill, according to the configuration
             *
             */
            RTPListener(std::shared_ptr<ohmcomm::network::NetworkWrapper> wrapper, JitterBuffers& buffers, unsigned int receiveBufferSize);
            RTPListener(const RTPListener& orig);
            ~RTPListener();

        private:
            const std::shared_ptr<ohmcomm::network::NetworkWrapper> wrapper;
            JitterBuffers& buffers;
            RTPPackageHandler rtpHandler;
            std::thread receiveThread;
            bool threadRunning = false;

            /*!
             * Method called in the parallel thread, receiving packages and writing them into RTPBuffer
             */
            void runThread();
            
            /*!
             * Shuts down the receive-thread
             */
            void shutdown();

            /*!
             * Starts the receive-thread
             */
            void startUp();

            /*!
             * Calculates the new extended highest sequence number for the received package
             */
            static uint32_t calculateExtendedHighestSequenceNumber(const Participant& participant, const uint16_t receivedSequenceNumber);
            
            //enables access to private methods for RTP-processor
            friend class ProcessorRTP;
        };
    }
}
#endif	/* RTPLISTENER_H */

