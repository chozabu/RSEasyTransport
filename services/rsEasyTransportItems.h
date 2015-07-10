/* this describes the datatypes sent over the network, and how to (de)serialise them */
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

#pragma once

/*
 * libretroshare/src/serialiser: rsEasyTransportItems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2011 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <map>

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"

/**************************************************************************/

const uint16_t RS_SERVICE_TYPE_EasyTransport_PLUGIN = 0xb031;

const uint8_t RS_PKT_SUBTYPE_EasyTransport_PING 	   = 0x01;
const uint8_t RS_PKT_SUBTYPE_EasyTransport_PONG 	   = 0x02;
const uint8_t RS_PKT_SUBTYPE_EasyTransport_PROTOCOL   = 0x03 ;//unused!
const uint8_t RS_PKT_SUBTYPE_EasyTransport_PAINT	   = 0x04 ;
const uint8_t RS_PKT_SUBTYPE_EasyTransport_DATA      	= 0x05 ;

const uint8_t QOS_PRIORITY_RS_EasyTransport = 9 ;

const uint32_t RS_EasyTransport_FLAGS_VIDEO_DATA = 0x0001 ;
const uint32_t RS_EasyTransport_FLAGS_AUDIO_DATA = 0x0002 ;

class RsEasyTransportItem: public RsItem
{
	public:
		RsEasyTransportItem(uint8_t net_example_subtype)
			: RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_EasyTransport_PLUGIN,net_example_subtype)
		{ 
			setPriorityLevel(QOS_PRIORITY_RS_EasyTransport) ;
		}	

		virtual ~RsEasyTransportItem() {};
		virtual void clear() {};
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) = 0 ;

		virtual bool serialise(void *data,uint32_t& size) = 0 ;	// Isn't it better that items can serialise themselves ?
		virtual uint32_t serial_size() const = 0 ; 							// deserialise is handled using a constructor
};

class RsEasyTransportPingItem: public RsEasyTransportItem
{
	public:
		RsEasyTransportPingItem() :RsEasyTransportItem(RS_PKT_SUBTYPE_EasyTransport_PING) {}
		RsEasyTransportPingItem(void *data,uint32_t size) ;

		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() const ; 						

		virtual ~RsEasyTransportPingItem() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t mSeqNo;
		uint64_t mPingTS;
};

class RsEasyTransportPongItem: public RsEasyTransportItem
{
	public:
		RsEasyTransportPongItem() :RsEasyTransportItem(RS_PKT_SUBTYPE_EasyTransport_PONG) {}
		RsEasyTransportPongItem(void *data,uint32_t size) ;

		virtual bool serialise(void *data,uint32_t& size) ;
		virtual uint32_t serial_size() const ;

		virtual ~RsEasyTransportPongItem() {}
	virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t mSeqNo;
		uint64_t mPingTS;
		uint64_t mPongTS;
};

class RsEasyTransportDataItem: public RsEasyTransportItem
{
	public:
		RsEasyTransportDataItem() :RsEasyTransportItem(RS_PKT_SUBTYPE_EasyTransport_DATA) {}
		RsEasyTransportDataItem(void *data,uint32_t size) ; // de-serialization

		virtual bool serialise(void *data,uint32_t& size) ;
		virtual uint32_t serial_size() const ; 							

		virtual ~RsEasyTransportDataItem()
		{
			//free(net_example_data) ;
			//net_example_data = NULL ;
		}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t flags ;
        uint32_t data_size ;
		std::string m_msg;
};

class RsEasyTransportPaintItem: public RsEasyTransportItem
{
	public:
		RsEasyTransportPaintItem() :RsEasyTransportItem(RS_PKT_SUBTYPE_EasyTransport_PAINT) {}
		RsEasyTransportPaintItem(void *data,uint32_t size) ;

		enum { EasyTransportProtocol_Ring = 1, EasyTransportProtocol_Ackn = 2, EasyTransportProtocol_Close = 3, EasyTransportProtocol_Bandwidth = 4 } ;

		virtual bool serialise(void *data,uint32_t& size) ;
		virtual uint32_t serial_size() const ;

		virtual ~RsEasyTransportPaintItem() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t x ;
		uint32_t y ;
};

class RsEasyTransportProtocolItem: public RsEasyTransportItem
{
	public:
		RsEasyTransportProtocolItem() :RsEasyTransportItem(RS_PKT_SUBTYPE_EasyTransport_PROTOCOL) {}
		RsEasyTransportProtocolItem(void *data,uint32_t size) ;

		enum { EasyTransportProtocol_Ring = 1, EasyTransportProtocol_Ackn = 2, EasyTransportProtocol_Close = 3, EasyTransportProtocol_Bandwidth = 4 } ;

		virtual bool serialise(void *data,uint32_t& size) ;
		virtual uint32_t serial_size() const ; 							

		virtual ~RsEasyTransportProtocolItem() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint32_t protocol ;
		uint32_t flags ;
};

class RsEasyTransportSerialiser: public RsSerialType
{
	public:
		RsEasyTransportSerialiser()
			:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_EasyTransport_PLUGIN)
		{ 
		}
		virtual ~RsEasyTransportSerialiser() {}

		virtual uint32_t 	size (RsItem *item) 
		{ 
			return dynamic_cast<RsEasyTransportItem *>(item)->serial_size() ;
		}

		virtual	bool serialise  (RsItem *item, void *data, uint32_t *size)
		{ 
			return dynamic_cast<RsEasyTransportItem *>(item)->serialise(data,*size) ;
		}
		virtual	RsItem *deserialise(void *data, uint32_t *size);
};

/**************************************************************************/
