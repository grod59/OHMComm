/*
 * File:   ReceiveThread.cpp
 * Author: daniel
 *
 * Created on May 16, 2015, 12:49 PM
 */

#include "RTPListener.h"
#include "Statistics.h"

RTPListener::RTPListener(std::shared_ptr<NetworkWrapper> wrapper, std::shared_ptr<RTPBufferHandler> buffer, unsigned int receiveBufferSize, std::function<void()> stopCallback) :
    stopCallback(stopCallback), rtpHandler(receiveBufferSize)
{
    this->wrapper = wrapper;
    this->buffer = buffer;
}

RTPListener::RTPListener(const RTPListener& orig) : rtpHandler(orig.rtpHandler)
{
    this->wrapper = orig.wrapper;
    this->buffer = orig.buffer;
}

RTPListener::~RTPListener()
{
    // Wait until thread has really stopped
    receiveThread.join();
}

void RTPListener::startUp()
{
    threadRunning = true;
    receiveThread = std::thread(&RTPListener::runThread, this);
}

void RTPListener::runThread()
{
    std::cout << "RTP-Listener started ..." << std::endl;
    while(threadRunning)
    {
        //1. wait for package and store into RTPPackage
        int receivedSize = this->wrapper->receiveData(rtpHandler.getWorkBuffer(), rtpHandler.getMaximumPackageSize());
        if(receivedSize == INVALID_SOCKET)
        {
            //socket was already closed
            shutdown();
        }
        else if(receivedSize == EAGAIN || receivedSize == EWOULDBLOCK)
        {
            //just continue to next loop iteration, checking if thread should continue running
        }
        else if(threadRunning && RTPPackageHandler::isRTPPackage(rtpHandler.getWorkBuffer(), (unsigned int)receivedSize))
        {
            //2. write package to buffer
            auto result = buffer->addPackage(rtpHandler, receivedSize - RTP_HEADER_MIN_SIZE);
            if (result == RTPBufferStatus::RTP_BUFFER_INPUT_OVERFLOW)
            {
                //TODO some handling or simply discard?
                std::cerr << "Input Buffer overflow" << std::endl;
            }
                else if (result == RTPBufferStatus::RTP_BUFFER_PACKAGE_TO_OLD)
                {
                    std::cerr << "Package was too old, discarding" << std::endl;
            }
            else
            {
                Statistics::incrementCounter(Statistics::COUNTER_PACKAGES_RECEIVED, 1);
                Statistics::incrementCounter(Statistics::COUNTER_HEADER_BYTES_RECEIVED, RTP_HEADER_MIN_SIZE);
                Statistics::incrementCounter(Statistics::COUNTER_PAYLOAD_BYTES_RECEIVED, receivedSize - RTP_HEADER_MIN_SIZE);
            }
        }
    }
    std::cout << "RTP-Listener shut down" << std::endl;
}

void RTPListener::shutdown()
{
    // notify the thread to stop
    threadRunning = false;
}

