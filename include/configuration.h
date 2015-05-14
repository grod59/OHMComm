/* 
 * File:   configuration.h
 * Author: daniel, jonas
 *
 * Created on April 1, 2015, 5:37 PM
 */

#ifndef CONFIGURATION_H
#define	CONFIGURATION_H

#ifdef __linux__
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

#include <string>





struct NetworkConfiguration {
	enum ConnectionType { TCP = 1, UDP = 2 };
	std::string addressIncoming;
	unsigned short portIncoming;

	std::string addressOutgoing;
	unsigned short portOutgoing;

	unsigned int outputBufferSize;
	unsigned int inputBufferSize;

	ConnectionType connectionType;
};

struct AudioConfiguration {
	// These parameters are needed for the RtAudio::StreamParameter (Struct)

	// Output Audio Device ID
	unsigned int outputDeviceID;
	// input audio device ID
	unsigned int inputDeviceID;

	// number of maximum output channels supported by the output device
	unsigned int outputDeviceChannels;
	// number of maximum input channels supported by the input device
	unsigned int inputDeviceChannels;

	// number of maximum output channels supported by the output device
	unsigned int outputDeviceFirstChannel;
	// number of maximum input channels supported by the input device
	unsigned int inputDeviceFirstChannel;


	// the name of the output audio device (optional)
	std::string outputDeviceName;

	// the name of the output audio device (optional)
	std::string inputDeviceName;

	// RtAudioformat, which is defined as: typedef unsigned long RtAudioFormat;
	unsigned long audioFormat;

	// sample rate of the audio device
	unsigned int sampleRate;

	// buffer frames
	unsigned int bufferFrames;
};
#endif	/* CONFIGURATION_H */

