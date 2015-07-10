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

#include <stdexcept>
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"

#include "services/rsEasyTransportItems.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

#define HOLLERITH_LEN_SPEC 4
/*************************************************************************/

std::ostream& RsEasyTransportPingItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsEasyTransportPingItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "SeqNo: " << mSeqNo << std::endl;

	printIndent(out, int_Indent);
	out << "PingTS: " << std::hex << mPingTS << std::dec << std::endl;

	printRsItemEnd(out, "RsEasyTransportPingItem", indent);
	return out;
}

std::ostream& RsEasyTransportPongItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsEasyTransportPongItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "SeqNo: " << mSeqNo << std::endl;

	printIndent(out, int_Indent);
	out << "PingTS: " << std::hex << mPingTS << std::dec << std::endl;

	printIndent(out, int_Indent);
	out << "PongTS: " << std::hex << mPongTS << std::dec << std::endl;

	printRsItemEnd(out, "RsEasyTransportPongItem", indent);
	return out;
}
std::ostream& RsEasyTransportProtocolItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsEasyTransportProtocolItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "flags: " << flags << std::endl;

	printIndent(out, int_Indent);
	out << "protocol: " << std::hex << protocol << std::dec << std::endl;

	printRsItemEnd(out, "RsEasyTransportProtocolItem", indent);
	return out;
}
std::ostream& RsEasyTransportPaintItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsEasyTransportPaintItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "x: " << x << std::endl;

	printIndent(out, int_Indent);
	out << "y: " << y << std::endl;

	printRsItemEnd(out, "RsEasyTransportPaintItem", indent);
	return out;
}
std::ostream& RsEasyTransportDataItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsEasyTransportDataItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "flags: " << flags << std::endl;

	printIndent(out, int_Indent);
	out << "data size: " << std::hex << data_size << std::dec << std::endl;

	printRsItemEnd(out, "RsEasyTransportDataItem", indent);
	return out;
}

/*************************************************************************/
uint32_t RsEasyTransportDataItem::serial_size() const
{
	uint32_t s = 8; /* header */
	s += 4; /* flags */
	s += 4; /* data_size  */
	//s += m_msg.length()+HOLLERITH_LEN_SPEC; /* data */
	s += getRawStringSize(m_msg);

	return s;
}
uint32_t RsEasyTransportProtocolItem::serial_size() const
{
	uint32_t s = 8; /* header */
	s += 4; /* flags */
	s += 4; /* protocol */

	return s;
}
uint32_t RsEasyTransportPaintItem::serial_size() const
{
	uint32_t s = 8; /* header */
	s += 4; /* x */
	s += 4; /* y */

	return s;
}
uint32_t RsEasyTransportPingItem::serial_size() const
{
	uint32_t s = 8; /* header */
	s += 4; /* seqno */
	s += 8; /* pingTS  */

	return s;
}
bool RsEasyTransportProtocolItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportDataItem() Header: " << ok << std::endl;
	std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportDataItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, protocol);
	ok &= setRawUInt32(data, tlvsize, &offset, flags);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportPingItem() Size Error! " << std::endl;
	}

	return ok;
}
bool RsEasyTransportPaintItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportDataItem() Header: " << ok << std::endl;
	std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportDataItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, x);
	ok &= setRawUInt32(data, tlvsize, &offset, y);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportPingItem() Size Error! " << std::endl;
	}

	return ok;
}
/* serialise the data to the buffer */
bool RsEasyTransportDataItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportDataItem() Header: " << ok << std::endl;
	std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportDataItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, flags);
	ok &= setRawUInt32(data, tlvsize, &offset, data_size);


	ok &= setRawString(data, tlvsize, &offset, m_msg );
	std::cout << "string sizes: " << getRawStringSize(m_msg) << " OR " << m_msg.size() << "\n";
	//offset += m_msg.size() ;
	//offset += getRawStringSize(m_msg);
	//memcpy( &((uint8_t*)data)[offset],net_example_data,data_size) ;
	//offset += data_size ;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportPingItem() Size Error! " << std::endl;
		std::cerr << "expected " << tlvsize << " got " << offset << std::endl;
		std::cerr << "m_msg looks like " << m_msg << std::endl;
	}

	return ok;
}
/* serialise the data to the buffer */
bool RsEasyTransportPingItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportPingItem() Header: " << ok << std::endl;
	std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportPingItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, mSeqNo);
	ok &= setRawUInt64(data, tlvsize, &offset, mPingTS);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportPingItem() Size Error! " << std::endl;
	}

	return ok;
}

RsEasyTransportProtocolItem::RsEasyTransportProtocolItem(void *data, uint32_t pktsize)
	: RsEasyTransportItem(RS_PKT_SUBTYPE_EasyTransport_PROTOCOL)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_EasyTransport_PLUGIN != getRsItemService(rstype)) || (RS_PKT_SUBTYPE_EasyTransport_PROTOCOL != getRsItemSubType(rstype)))
		throw std::runtime_error("Wrong packet type!") ;

	if (pktsize < rssize)    /* check size */
		throw std::runtime_error("Not enough size!") ;

	bool ok = true;

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &protocol);
	ok &= getRawUInt32(data, rssize, &offset, &flags);

	if (offset != rssize)
		throw std::runtime_error("Deserialisation error!") ;

	if (!ok)
		throw std::runtime_error("Deserialisation error!") ;
}
RsEasyTransportPaintItem::RsEasyTransportPaintItem(void *data, uint32_t pktsize)
	: RsEasyTransportItem(RS_PKT_SUBTYPE_EasyTransport_PAINT)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_EasyTransport_PLUGIN != getRsItemService(rstype)) || (RS_PKT_SUBTYPE_EasyTransport_PAINT != getRsItemSubType(rstype)))
		throw std::runtime_error("Wrong packet type!") ;

	if (pktsize < rssize)    /* check size */
		throw std::runtime_error("Not enough size!") ;

	bool ok = true;

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &x);
	ok &= getRawUInt32(data, rssize, &offset, &y);

	if (offset != rssize)
		throw std::runtime_error("Deserialisation error!") ;

	if (!ok)
		throw std::runtime_error("Deserialisation error!") ;
}
RsEasyTransportPingItem::RsEasyTransportPingItem(void *data, uint32_t pktsize)
	: RsEasyTransportItem(RS_PKT_SUBTYPE_EasyTransport_PING)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_EasyTransport_PLUGIN != getRsItemService(rstype)) || (RS_PKT_SUBTYPE_EasyTransport_PING != getRsItemSubType(rstype)))
		throw std::runtime_error("Wrong packet type!") ;

	if (pktsize < rssize)    /* check size */
		throw std::runtime_error("Not enough size!") ;

	bool ok = true;

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &mSeqNo);
	ok &= getRawUInt64(data, rssize, &offset, &mPingTS);

	if (offset != rssize)
		throw std::runtime_error("Deserialisation error!") ;

	if (!ok)
		throw std::runtime_error("Deserialisation error!") ;
}

/*************************************************************************/
/*************************************************************************/


uint32_t RsEasyTransportPongItem::serial_size() const
{
	uint32_t s = 8; /* header */
	s += 4; /* seqno */
	s += 8; /* pingTS  */
	s += 8; /* pongTS */

	return s;
}

/* serialise the data to the buffer */
bool RsEasyTransportPongItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportPongItem() Header: " << ok << std::endl;
	std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportPongItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, mSeqNo);
	ok &= setRawUInt64(data, tlvsize, &offset, mPingTS);
	ok &= setRawUInt64(data, tlvsize, &offset, mPongTS);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsEasyTransportSerialiser::serialiseEasyTransportPongItem() Size Error! " << std::endl;
	}

	return ok;
}
RsEasyTransportDataItem::RsEasyTransportDataItem(void *data, uint32_t pktsize)
	: RsEasyTransportItem(RS_PKT_SUBTYPE_EasyTransport_DATA)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_EasyTransport_PLUGIN != getRsItemService(rstype)) || (RS_PKT_SUBTYPE_EasyTransport_DATA != getRsItemSubType(rstype)))
		throw std::runtime_error("Wrong packet subtype") ;

	if (pktsize < rssize)    /* check size */
		throw std::runtime_error("Not enough space") ;

	bool ok = true;

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &flags);
	ok &= getRawUInt32(data, rssize, &offset, &data_size);


	ok &= getRawString(data, rssize, &offset, m_msg );

	/*net_example_data = malloc(data_size) ;
	memcpy(net_example_data,&((uint8_t*)data)[offset],data_size) ;
	offset += data_size ;*/

	if (offset != rssize)
		throw std::runtime_error("Serialization error.") ;

	if (!ok)
		throw std::runtime_error("Serialization error.") ;
}
RsEasyTransportPongItem::RsEasyTransportPongItem(void *data, uint32_t pktsize)
	: RsEasyTransportItem(RS_PKT_SUBTYPE_EasyTransport_PONG)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_EasyTransport_PLUGIN != getRsItemService(rstype)) || (RS_PKT_SUBTYPE_EasyTransport_PONG != getRsItemSubType(rstype)))
		throw std::runtime_error("Wrong packet subtype") ;

	if (pktsize < rssize)    /* check size */
		throw std::runtime_error("Not enough space") ;

	bool ok = true;

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &mSeqNo);
	ok &= getRawUInt64(data, rssize, &offset, &mPingTS);
	ok &= getRawUInt64(data, rssize, &offset, &mPongTS);

	if (offset != rssize)
		throw std::runtime_error("Serialization error.") ;

	if (!ok)
		throw std::runtime_error("Serialization error.") ;
}

/*************************************************************************/

RsItem* RsEasyTransportSerialiser::deserialise(void *data, uint32_t *pktsize)
{
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsEasyTransportSerialiser::deserialise()" << std::endl;
#endif

	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_EasyTransport_PLUGIN != getRsItemService(rstype)))
		return NULL ;
	
	try
	{
		switch(getRsItemSubType(rstype))
		{
			case RS_PKT_SUBTYPE_EasyTransport_PING: 		return new RsEasyTransportPingItem(data, *pktsize);
			case RS_PKT_SUBTYPE_EasyTransport_PONG: 		return new RsEasyTransportPongItem(data, *pktsize);
			case RS_PKT_SUBTYPE_EasyTransport_PROTOCOL: 	return new RsEasyTransportProtocolItem(data, *pktsize);
			case RS_PKT_SUBTYPE_EasyTransport_PAINT:		return new RsEasyTransportPaintItem(data, *pktsize);
			case RS_PKT_SUBTYPE_EasyTransport_DATA: 		return new RsEasyTransportDataItem(data, *pktsize);

			default:
				return NULL;
		}
	}
	catch(std::exception& e)
	{
		std::cerr << "RsEasyTransportSerialiser: deserialization error: " << e.what() << std::endl;
		return NULL;
	}
}


/*************************************************************************/

