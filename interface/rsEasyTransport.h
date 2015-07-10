/* this is a simple class to make it easy for any part of the plugin to call its services */
/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2015
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

// interface class for p3EasyTransport service
//

#pragma once

#include <stdint.h>
#include <string>
#include <list>
#include <vector>
#include <retroshare/rstypes.h>

class RsEasyTransport ;
extern RsEasyTransport *rsEasyTransport;
 
static const uint32_t CONFIG_TYPE_EasyTransport_PLUGIN 		= 0xe001 ;

class RsEasyTransportPongResult
{
	public:
	RsEasyTransportPongResult()
	:mTS(0), mRTT(0), mOffset(0) { return; }

	RsEasyTransportPongResult(double ts, double rtt, double offset)
	:mTS(ts), mRTT(rtt), mOffset(offset) { return; }

	double mTS;
	double mRTT;
	double mOffset;
};

struct RsEasyTransportDataChunk
{
	typedef enum { RS_EasyTransport_DATA_TYPE_AUDIO, RS_EasyTransport_DATA_TYPE_VIDEO } RsEasyTransportDataType ;

	void *data ; // create/delete using malloc/free.
	uint32_t size ;
	RsEasyTransportDataType type ;	// video or audio
};

class RsEasyTransport
{
	public:
		virtual int sendEasyTransportHangUpCall(const RsPeerId& peer_id) = 0;
		virtual int sendEasyTransportRinging(const RsPeerId& peer_id) = 0;
		virtual int sendEasyTransportAcceptCall(const RsPeerId& peer_id) = 0;

	virtual void ping_all() = 0;
	virtual void broadcast_paint(int x, int y) = 0;
	virtual void msg_all(std::string msg) = 0;
		// Sending data. The client keeps the memory ownership and must delete it after calling this.
		virtual int sendEasyTransportData(const RsPeerId& peer_id,const RsEasyTransportDataChunk& chunk) = 0;

		// The server fill in the data and gives up memory ownership. The client must delete the memory
		// in each chunk once it has been used.
		//
		virtual bool getIncomingData(const RsPeerId& peer_id,std::vector<RsEasyTransportDataChunk>& chunks) = 0;

		typedef enum { AudioTransmitContinous = 0, AudioTransmitVAD = 1, AudioTransmitPushToTalk = 2 } enumAudioTransmit ;

		// Config stuff

		virtual uint32_t getPongResults(const RsPeerId& id, int n, std::list<RsEasyTransportPongResult> &results) = 0;
};


