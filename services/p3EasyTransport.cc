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

#include "util/rsdir.h"
#include "retroshare/rsiface.h"
#include "pqi/pqibin.h"
#include "pqi/pqistore.h"
#include "pqi/p3linkmgr.h"
#include <serialiser/rsserial.h>
#include <serialiser/rsconfigitems.h>

#include <sstream> // for std::istringstream

#include "services/p3EasyTransport.h"
#include "services/rsEasyTransportItems.h"

#include <sys/time.h>

#include "gui/EasyTransportNotify.h"


//#define DEBUG_EasyTransport		1


/* DEFINE INTERFACE POINTER! */
RsEasyTransport *rsEasyTransport = NULL;


#define MAX_PONG_RESULTS		150
#define EasyTransport_PING_PERIOD  		10
#define EasyTransport_BANDWIDTH_PERIOD 5


#ifdef WINDOWS_SYS
#include <time.h>
#include <sys/timeb.h>
#endif

static double getCurrentTS()
{

#ifndef WINDOWS_SYS
        struct timeval cts_tmp;
        gettimeofday(&cts_tmp, NULL);
        double cts =  (cts_tmp.tv_sec) + ((double) cts_tmp.tv_usec) / 1000000.0;
#else
        struct _timeb timebuf;
        _ftime( &timebuf);
        double cts =  (timebuf.time) + ((double) timebuf.millitm) / 1000.0;
#endif
        return cts;
}

static uint64_t convertTsTo64bits(double ts)
{
	uint32_t secs = (uint32_t) ts;
	uint32_t usecs = (uint32_t) ((ts - (double) secs) * 1000000);
	uint64_t bits = (((uint64_t) secs) << 32) + usecs;
	return bits;
}

static double convert64bitsToTs(uint64_t bits)
{
	uint32_t usecs = (uint32_t) (bits & 0xffffffff);
	uint32_t secs = (uint32_t) ((bits >> 32) & 0xffffffff);
	double ts =  (secs) + ((double) usecs) / 1000000.0;

	return ts;
}

p3EasyTransport::p3EasyTransport(RsPluginHandler *handler,EasyTransportNotify *notifier)
	 : RsPQIService(RS_SERVICE_TYPE_EasyTransport_PLUGIN,0,handler), mEasyTransportMtx("p3EasyTransport"), mServiceControl(handler->getServiceControl()) , mNotify(notifier)
{
	addSerialType(new RsEasyTransportSerialiser());

	mSentPingTime = 0;
	mSentBandwidthInfoTime = 0;
	mCounter = 0;

        //plugin default configuration
        _atransmit = 0;
        _voice_hold = 75;
        _vadmin = 16018;
        _vadmax = 23661;
        _min_loudness = 4702;
        _noise_suppress = -45;
        _echo_cancel = true;

}
RsServiceInfo p3EasyTransport::getServiceInfo()
{
	const std::string TURTLE_APP_NAME = "EasyTransport";
    const uint16_t TURTLE_APP_MAJOR_VERSION  =       1;
    const uint16_t TURTLE_APP_MINOR_VERSION  =       0;
    const uint16_t TURTLE_MIN_MAJOR_VERSION  =       1;
    const uint16_t TURTLE_MIN_MINOR_VERSION  =       0;

	return RsServiceInfo(RS_SERVICE_TYPE_EasyTransport_PLUGIN,
                         TURTLE_APP_NAME,
                         TURTLE_APP_MAJOR_VERSION,
                         TURTLE_APP_MINOR_VERSION,
                         TURTLE_MIN_MAJOR_VERSION,
                         TURTLE_MIN_MINOR_VERSION);
}

int	p3EasyTransport::tick()
{
#ifdef DEBUG_EasyTransport
	std::cerr << "ticking p3EasyTransport" << std::endl;
#endif

	//processIncoming();
	//sendPackets();

	return 0;
}

int	p3EasyTransport::status()
{
	return 1;
}

int	p3EasyTransport::sendPackets()
{
	time_t now = time(NULL);
	time_t pt;
	time_t pt2;
	{
		RsStackMutex stack(mEasyTransportMtx); /****** LOCKED MUTEX *******/
		pt = mSentPingTime;
		pt2 = mSentBandwidthInfoTime;
	}

	if (now > pt + EasyTransport_PING_PERIOD)
	{
		sendPingMeasurements();

		RsStackMutex stack(mEasyTransportMtx); /****** LOCKED MUTEX *******/
		mSentPingTime = now;
	}
	if (now > pt2 + EasyTransport_BANDWIDTH_PERIOD)
	{
		sendBandwidthInfo();

		RsStackMutex stack(mEasyTransportMtx); /****** LOCKED MUTEX *******/
		mSentBandwidthInfoTime = now;
	}
	return true ;
}
void p3EasyTransport::sendBandwidthInfo()
{
    std::set<RsPeerId> onlineIds;
	 mServiceControl->getPeersConnected(getServiceInfo().mServiceType, onlineIds);

	 RsStackMutex stack(mEasyTransportMtx); /****** LOCKED MUTEX *******/

	for(std::map<RsPeerId,EasyTransportPeerInfo>::iterator it(mPeerInfo.begin());it!=mPeerInfo.end();++it)
	{
		it->second.average_incoming_bandwidth = 0.75 * it->second.average_incoming_bandwidth + 0.25 * it->second.total_bytes_received / EasyTransport_BANDWIDTH_PERIOD ;
		it->second.total_bytes_received = 0 ;

		if(onlineIds.find(it->first) == onlineIds.end() || it->second.average_incoming_bandwidth == 0)
			continue ;

		std::cerr << "average bandwidth for peer " << it->first << ": " << it->second.average_incoming_bandwidth << " Bps" << std::endl;
		sendEasyTransportBandwidth(it->first,it->second.average_incoming_bandwidth) ;
	}
}

int p3EasyTransport::sendEasyTransportHangUpCall(const RsPeerId &peer_id)
{
	RsEasyTransportProtocolItem *item = new RsEasyTransportProtocolItem ;

	item->protocol = RsEasyTransportProtocolItem::EasyTransportProtocol_Close;
	item->flags = 0 ;
	item->PeerId(peer_id) ;

	sendItem(item) ;

	return true ;
}
int p3EasyTransport::sendEasyTransportAcceptCall(const RsPeerId& peer_id)
{
	RsEasyTransportProtocolItem *item = new RsEasyTransportProtocolItem ;

	item->protocol = RsEasyTransportProtocolItem::EasyTransportProtocol_Ackn ;
	item->flags = 0 ;
	item->PeerId(peer_id) ;

	sendItem(item) ;

	return true ;
}
void p3EasyTransport::msg_all(std::string msg){
	/* we ping our peers */
	//if(!mServiceControl)
	//    return ;

	//std::set<RsPeerId> onlineIds;
	std::list< RsPeerId > onlineIds;
	//    mServiceControl->getPeersConnected(getServiceInfo().mServiceType, onlineIds);
	rsPeers->getOnlineList(onlineIds);

	double ts = getCurrentTS();

#ifdef DEBUG_EasyTransport
	std::cerr << "p3EasyTransport::msg_all() @ts: " << ts;
	std::cerr << std::endl;
#endif

	std::cout << "READY TO BCast: " << onlineIds.size() << "\n";
	/* prepare packets */
	std::list<RsPeerId>::iterator it;
	for(it = onlineIds.begin(); it != onlineIds.end(); it++)
	{
#ifdef DEBUG_EasyTransport
		std::cerr << "p3EasyTransport::msg_all() MSging: " << *it;
		std::cerr << std::endl;
#endif

		std::cout << "MSging: " << (*it).toStdString() << "\n";
		/* create the packet */
		RsEasyTransportDataItem *pingPkt = new RsEasyTransportDataItem();
		pingPkt->PeerId(*it);
		pingPkt->m_msg = msg;
		pingPkt->data_size = msg.size();
		//pingPkt->mSeqNo = mCounter;
		//pingPkt->mPingTS = convertTsTo64bits(ts);

		//storePingAttempt(*it, ts, mCounter);

#ifdef DEBUG_EasyTransport
		std::cerr << "p3EasyTransport::msg_all() With Packet:";
		std::cerr << std::endl;
		pingPkt->print(std::cerr, 10);
#endif

		sendItem(pingPkt);
	}

	//RsStackMutex stack(mEasyTransportMtx); /****** LOCKED MUTEX *******/
	//mCounter++;
}

void p3EasyTransport::ping_all(){
	this->sendPingMeasurements();
}

void p3EasyTransport::broadcast_paint(int x, int y)
{
	std::list< RsPeerId > onlineIds;
	//    mServiceControl->getPeersConnected(getServiceInfo().mServiceType, onlineIds);
	rsPeers->getOnlineList(onlineIds);

	double ts = getCurrentTS();


	std::cout << "READY TO PAINT: " << onlineIds.size() << "\n";
	/* prepare packets */
	std::list<RsPeerId>::iterator it;
	for(it = onlineIds.begin(); it != onlineIds.end(); it++)
	{

		std::cout << "painting to: " << (*it).toStdString() << "\n";
		/* create the packet */
		RsEasyTransportPaintItem *ppkt = new RsEasyTransportPaintItem();
		ppkt->x = x;
		ppkt->y = y;

		ppkt->PeerId(*it);


		sendItem(ppkt);
	}
}

/*int p3EasyTransport::sendEasyTransportPing(const RsPeerId &peer_id)
{
	RsEasyTransportPingItem *item = new RsEasyTransportPingItem ;

	item->protocol = RsEasyTransportProtocolItem::EasyTransportProtocol_Ring ;
	item->flags = 0 ;
	item->PeerId(peer_id) ;

	sendItem(item) ;

	return true ;
}*/
int p3EasyTransport::sendEasyTransportRinging(const RsPeerId &peer_id)
{
	RsEasyTransportProtocolItem *item = new RsEasyTransportProtocolItem ;

	item->protocol = RsEasyTransportProtocolItem::EasyTransportProtocol_Ring ;
	item->flags = 0 ;
	item->PeerId(peer_id) ;

	sendItem(item) ;

	return true ;
}
int p3EasyTransport::sendEasyTransportBandwidth(const RsPeerId &peer_id,uint32_t bytes_per_sec)
{
	RsEasyTransportProtocolItem *item = new RsEasyTransportProtocolItem ;

	item->protocol = RsEasyTransportProtocolItem::EasyTransportProtocol_Bandwidth ;
	item->flags = bytes_per_sec ;
	item->PeerId(peer_id) ;

	sendItem(item) ;

	return true ;
}
int p3EasyTransport::sendEasyTransportData(const RsPeerId& peer_id,const RsEasyTransportDataChunk& chunk)
{
#ifdef DEBUG_EasyTransport
	std::cerr << "Sending " << chunk.size << " bytes of net_example data." << std::endl;
#endif

	RsEasyTransportDataItem *item = new RsEasyTransportDataItem ;

	if(!item)
	{
		std::cerr << "Cannot allocate RsEasyTransportDataItem !" << std::endl;
		return false ;
	}
	std::string tChunk = "Hello Test!!";
	/*item->net_example_data = malloc(chunk.size) ;

	if(item->net_example_data == NULL)
	{
		std::cerr << "Cannot allocate RsEasyTransportDataItem.net_example_data of size " << chunk.size << " !" << std::endl;
		delete item ;
		return false ;
	}
	memcpy(item->net_example_data,chunk.data,chunk.size) ;*/
	item->PeerId(peer_id) ;
	item->data_size = tChunk.size();

	if(chunk.type == RsEasyTransportDataChunk::RS_EasyTransport_DATA_TYPE_AUDIO)
		item->flags = RS_EasyTransport_FLAGS_AUDIO_DATA ;
	else if(chunk.type == RsEasyTransportDataChunk::RS_EasyTransport_DATA_TYPE_VIDEO)
		item->flags = RS_EasyTransport_FLAGS_VIDEO_DATA ;
	else
	{
		std::cerr << "(EE) p3EasyTransport: cannot send chunk data. Unknown data type = " << chunk.type << std::endl;
		delete item ;
		return false ;
	}

	sendItem(item) ;

	return true ;
}

void p3EasyTransport::sendPingMeasurements()
{
	/* we ping our peers */
	/* who is online? */
	//if(!mServiceControl)
	//    return ;

	//std::set<RsPeerId> onlineIds;
	std::list< RsPeerId > onlineIds;
	//    mServiceControl->getPeersConnected(getServiceInfo().mServiceType, onlineIds);
	rsPeers->getOnlineList(onlineIds);

	double ts = getCurrentTS();

#ifdef DEBUG_EasyTransport
	std::cerr << "p3EasyTransport::sendPingMeasurements() @ts: " << ts;
	std::cerr << std::endl;
#endif

	std::cout << "READY TO PING: " << onlineIds.size() << "\n";
	/* prepare packets */
	std::list<RsPeerId>::iterator it;
	for(it = onlineIds.begin(); it != onlineIds.end(); it++)
	{
#ifdef DEBUG_EasyTransport
		std::cerr << "p3EasyTransport::sendPingMeasurements() Pinging: " << *it;
		std::cerr << std::endl;
#endif

		std::cout << "pinging: " << (*it).toStdString() << "\n";
		/* create the packet */
		RsEasyTransportPingItem *pingPkt = new RsEasyTransportPingItem();
		pingPkt->PeerId(*it);
		pingPkt->mSeqNo = mCounter;
		pingPkt->mPingTS = convertTsTo64bits(ts);

		storePingAttempt(*it, ts, mCounter);

#ifdef DEBUG_EasyTransport
		std::cerr << "p3EasyTransport::sendPingMeasurements() With Packet:";
		std::cerr << std::endl;
		pingPkt->print(std::cerr, 10);
#endif

		sendItem(pingPkt);
	}

	RsStackMutex stack(mEasyTransportMtx); /****** LOCKED MUTEX *******/
	mCounter++;
}


void p3EasyTransport::handlePaint(RsEasyTransportPaintItem *item)
{

	RsStackMutex stack(mEasyTransportMtx); /****** LOCKED MUTEX *******/ //unneeded?

	// store the data in a queue.


	mNotify->notifyReceivedPaint(item->PeerId(), item->x,item->y);
}

void p3EasyTransport::handleProtocol(RsEasyTransportProtocolItem *item)
{
	// should we keep a list of received requests?

	/*switch(item->protocol)
	{
		case RsEasyTransportProtocolItem::EasyTransportProtocol_Ring: mNotify->notifyReceivedEasyTransportInvite(item->PeerId());
#ifdef DEBUG_EasyTransport
																  std::cerr << "p3EasyTransport::handleProtocol(): Received protocol ring item." << std::endl;
#endif
																  break ;

		case RsEasyTransportProtocolItem::EasyTransportProtocol_Ackn: mNotify->notifyReceivedEasyTransportAccept(item->PeerId());
#ifdef DEBUG_EasyTransport
																  std::cerr << "p3EasyTransport::handleProtocol(): Received protocol accept call" << std::endl;
#endif
																  break ;

		case RsEasyTransportProtocolItem::EasyTransportProtocol_Close: mNotify->notifyReceivedEasyTransportHangUp(item->PeerId());
#ifdef DEBUG_EasyTransport
																  std::cerr << "p3EasyTransport::handleProtocol(): Received protocol Close call." << std::endl;
#endif
																  break ;
		case RsEasyTransportProtocolItem::EasyTransportProtocol_Bandwidth: mNotify->notifyReceivedEasyTransportBandwidth(item->PeerId(),(uint32_t)item->flags);
#ifdef DEBUG_EasyTransport
																  std::cerr << "p3EasyTransport::handleProtocol(): Received protocol bandwidth. Value=" << item->flags << std::endl;
#endif
																  break ;
		default:
#ifdef DEBUG_EasyTransport
																  std::cerr << "p3EasyTransport::handleProtocol(): Received protocol item # " << item->protocol << ": not handled yet ! Sorry" << std::endl;
#endif
																  break ;
	}*/

}

void p3EasyTransport::handleData(RsEasyTransportDataItem *item)
{
	RsStackMutex stack(mEasyTransportMtx); /****** LOCKED MUTEX *******/

	// store the data in a queue.


	mNotify->notifyReceivedMsg(item->PeerId(), QString::fromStdString(item->m_msg));
	/*
	std::map<RsPeerId,EasyTransportPeerInfo>::iterator it = mPeerInfo.find(item->PeerId()) ;

	if(it == mPeerInfo.end())
	{
		std::cerr << "Peer unknown to EasyTransport process. Dropping data" << std::endl;
		delete item ;
		return ;
	}
	it->second.incoming_queue.push_back(item) ;	// be careful with the delete action!

	// For Video data, measure the bandwidth
	
	if(item->flags & RS_EasyTransport_FLAGS_VIDEO_DATA)
		it->second.total_bytes_received += item->data_size ;

	//mNotify->notifyReceivedEasyTransportData(item->PeerId());*/
}

bool p3EasyTransport::getIncomingData(const RsPeerId& peer_id,std::vector<RsEasyTransportDataChunk>& incoming_data_chunks)
{
	RsStackMutex stack(mEasyTransportMtx); /****** LOCKED MUTEX *******/

	incoming_data_chunks.clear() ;

	std::map<RsPeerId,EasyTransportPeerInfo>::iterator it = mPeerInfo.find(peer_id) ;

	if(it == mPeerInfo.end())
	{
		std::cerr << "Peer unknown to EasyTransport process. No data returned. Probably a bug !" << std::endl;
		return false ;
	}
	for(std::list<RsEasyTransportDataItem*>::const_iterator it2(it->second.incoming_queue.begin());it2!=it->second.incoming_queue.end();++it2)
	{
		/*RsEasyTransportDataChunk chunk ;
		chunk.size = (*it2)->data_size ;
		chunk.data = malloc((*it2)->data_size) ;

		uint32_t type_flags = (*it2)->flags & (RS_EasyTransport_FLAGS_AUDIO_DATA | RS_EasyTransport_FLAGS_VIDEO_DATA) ;
		if(type_flags == RS_EasyTransport_FLAGS_AUDIO_DATA)
			chunk.type = RsEasyTransportDataChunk::RS_EasyTransport_DATA_TYPE_AUDIO ;
		else if(type_flags == RS_EasyTransport_FLAGS_VIDEO_DATA)
			chunk.type = RsEasyTransportDataChunk::RS_EasyTransport_DATA_TYPE_VIDEO ;
		else
		{
			std::cerr << "(EE) p3EasyTransport::getIncomingData(): error. Cannot handle item with unknown type " << type_flags << std::endl;
			delete *it2 ;
			free(chunk.data) ;
			continue ;
		}

		memcpy(chunk.data,(*it2)->net_example_data,(*it2)->data_size) ;

		incoming_data_chunks.push_back(chunk) ;*/

		delete *it2 ;
	}

	it->second.incoming_queue.clear() ;

	return true ;
}

bool	p3EasyTransport::recvItem(RsItem *item)
{
	/* pass to specific handler */
	bool keep = false ;

	switch(item->PacketSubType())
	{
		case RS_PKT_SUBTYPE_EasyTransport_PING:
			handlePing(dynamic_cast<RsEasyTransportPingItem*>(item));
			break;

		case RS_PKT_SUBTYPE_EasyTransport_PONG:
			handlePong(dynamic_cast<RsEasyTransportPongItem*>(item));
			break;

		case RS_PKT_SUBTYPE_EasyTransport_PROTOCOL:
			handleProtocol(dynamic_cast<RsEasyTransportProtocolItem*>(item)) ;
			break ;
		case RS_PKT_SUBTYPE_EasyTransport_PAINT:
			handlePaint(dynamic_cast<RsEasyTransportPaintItem*>(item)) ;
			break ;

		case RS_PKT_SUBTYPE_EasyTransport_DATA:
			handleData(dynamic_cast<RsEasyTransportDataItem*>(item));
			keep = true ;
			break;
#if 0
													 /* THESE ARE ALL FUTURISTIC DATA TYPES */
		case RS_BANDWIDTH_PING_ITEM:	 
			handleBandwidthPing(item);
			break;

		case RS_BANDWIDTH_PONG_ITEM:
			handleBandwidthPong(item);
			 break;
#endif
		default:
			break;
	}

	/* clean up */
	if(!keep)
		delete item;
	return true ;
} 

int p3EasyTransport::handlePing(RsEasyTransportPingItem *ping)
{
	/* cast to right type */

//#ifdef DEBUG_EasyTransport
	std::cout << "p3EasyTransport::handlePing() Recvd Packet from: " << ping->PeerId();
	std::cout << std::endl;
//#endif
	mNotify->notifyReceivedMsg(ping->PeerId(), "ping");

	/* with a ping, we just respond as quickly as possible - they do all the analysis */
	RsEasyTransportPongItem *pong = new RsEasyTransportPongItem();


	pong->PeerId(ping->PeerId());
	pong->mPingTS = ping->mPingTS;
	pong->mSeqNo = ping->mSeqNo;

	// add our timestamp.
	double ts = getCurrentTS();
	pong->mPongTS = convertTsTo64bits(ts);


#ifdef DEBUG_EasyTransport
	std::cerr << "p3EasyTransport::handlePing() With Packet:";
	std::cerr << std::endl;
	pong->print(std::cerr, 10);
#endif

	sendItem(pong);
	return true ;
}


int p3EasyTransport::handlePong(RsEasyTransportPongItem *pong)
{
	/* cast to right type */

//#ifdef DEBUG_EasyTransport
	std::cout << "p3EasyTransport::handlePong() Recvd Packet from: " << pong->PeerId();
	std::cout << std::endl;
	pong->print(std::cout, 10);
//#endif
	mNotify->notifyReceivedMsg(pong->PeerId(), "pong");

	/* with a pong, we do the maths! */
	double recvTS = getCurrentTS();
	double pingTS = convert64bitsToTs(pong->mPingTS);
	double pongTS = convert64bitsToTs(pong->mPongTS);

	double rtt = recvTS - pingTS;
	double offset = pongTS - (recvTS - rtt / 2.0);  // so to get to their time, we go ourTS + offset.

#ifdef DEBUG_EasyTransport
	std::cerr << "p3EasyTransport::handlePong() Timing:";
	std::cerr << std::endl;
	std::cerr << "\tpingTS: " << pingTS;
	std::cerr << std::endl;
	std::cerr << "\tpongTS: " << pongTS;
	std::cerr << std::endl;
	std::cerr << "\trecvTS: " << recvTS;
	std::cerr << std::endl;
	std::cerr << "\t ==> rtt: " << rtt;
	std::cerr << std::endl;
	std::cerr << "\t ==> offset: " << offset;
	std::cerr << std::endl;
#endif

	storePongResult(pong->PeerId(), pong->mSeqNo, pingTS, rtt, offset);
	return true ;
}

int	p3EasyTransport::storePingAttempt(const RsPeerId& id, double ts, uint32_t seqno)
{
	RsStackMutex stack(mEasyTransportMtx); /****** LOCKED MUTEX *******/

	/* find corresponding local data */
	EasyTransportPeerInfo *peerInfo = locked_GetPeerInfo(id);

	peerInfo->mCurrentPingTS = ts;
	peerInfo->mCurrentPingCounter = seqno;

	peerInfo->mSentPings++;
	if (!peerInfo->mCurrentPongRecvd)
	{
		peerInfo->mLostPongs++;
	}

	peerInfo->mCurrentPongRecvd = true;

	return 1;
}



int	p3EasyTransport::storePongResult(const RsPeerId &id, uint32_t counter, double ts, double rtt, double offset)
{
	RsStackMutex stack(mEasyTransportMtx); /****** LOCKED MUTEX *******/

	/* find corresponding local data */
	EasyTransportPeerInfo *peerInfo = locked_GetPeerInfo(id);

	if (peerInfo->mCurrentPingCounter != counter)
	{
#ifdef DEBUG_EasyTransport
		std::cerr << "p3EasyTransport::storePongResult() ERROR Severly Delayed Measurements!" << std::endl;
#endif
	}
	else
	{
		peerInfo->mCurrentPongRecvd = true;
	}

	peerInfo->mPongResults.push_back(RsEasyTransportPongResult(ts, rtt, offset));


	while(peerInfo->mPongResults.size() > MAX_PONG_RESULTS)
	{
		peerInfo->mPongResults.pop_front();
	}

	/* should do calculations */
	return 1;
}


uint32_t p3EasyTransport::getPongResults(const RsPeerId& id, int n, std::list<RsEasyTransportPongResult> &results)
{
	RsStackMutex stack(mEasyTransportMtx); /****** LOCKED MUTEX *******/

	EasyTransportPeerInfo *peer = locked_GetPeerInfo(id);

	std::list<RsEasyTransportPongResult>::reverse_iterator it;
	int i = 0;
	for(it = peer->mPongResults.rbegin(); (it != peer->mPongResults.rend()) && (i < n); it++, i++)
	{
		/* reversing order - so its easy to trim later */
		results.push_back(*it);
	}
	return i ;
}



EasyTransportPeerInfo *p3EasyTransport::locked_GetPeerInfo(const RsPeerId &id)
{
	std::map<RsPeerId, EasyTransportPeerInfo>::iterator it;
	it = mPeerInfo.find(id);
	if (it == mPeerInfo.end())
	{
		/* add it in */
		EasyTransportPeerInfo pinfo;

		/* initialise entry */
		pinfo.initialisePeerInfo(id);
		
		mPeerInfo[id] = pinfo;

		it = mPeerInfo.find(id);
	}

	return &(it->second);
}

bool EasyTransportPeerInfo::initialisePeerInfo(const RsPeerId& id)
{
	mId = id;

	/* reset variables */
	mCurrentPingTS = 0;
	mCurrentPingCounter = 0;
	mCurrentPongRecvd = true;

	mSentPings = 0;
	mLostPongs = 0;
	average_incoming_bandwidth = 0 ;
	total_bytes_received = 0 ;

	mPongResults.clear();

	return true;
}


RsTlvKeyValue p3EasyTransport::push_int_value(const std::string& key,int value)
{
	RsTlvKeyValue kv ;
	kv.key = key ;
	rs_sprintf(kv.value, "%d", value);

	return kv ;
}
int p3EasyTransport::pop_int_value(const std::string& s)
{
	std::istringstream is(s) ;

	int val ;
	is >> val ;

	return val ;
}

bool p3EasyTransport::saveList(bool& cleanup, std::list<RsItem*>& lst)
{
	cleanup = true ;

	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;

	vitem->tlvkvs.pairs.push_back(push_int_value("P3EasyTransport_CONFIG_ATRANSMIT",_atransmit)) ;
	vitem->tlvkvs.pairs.push_back(push_int_value("P3EasyTransport_CONFIG_VOICEHOLD",_voice_hold)) ;
	vitem->tlvkvs.pairs.push_back(push_int_value("P3EasyTransport_CONFIG_VADMIN"   ,_vadmin)) ;
	vitem->tlvkvs.pairs.push_back(push_int_value("P3EasyTransport_CONFIG_VADMAX"   ,_vadmax)) ;
	vitem->tlvkvs.pairs.push_back(push_int_value("P3EasyTransport_CONFIG_NOISE_SUP",_noise_suppress)) ;
	vitem->tlvkvs.pairs.push_back(push_int_value("P3EasyTransport_CONFIG_MIN_LOUDN",_min_loudness)) ;
	vitem->tlvkvs.pairs.push_back(push_int_value("P3EasyTransport_CONFIG_ECHO_CNCL",_echo_cancel)) ;

	lst.push_back(vitem) ;

	return true ;
}
bool p3EasyTransport::loadList(std::list<RsItem*>& load)
{
	for(std::list<RsItem*>::const_iterator it(load.begin());it!=load.end();++it)
	{
#ifdef P3TURTLE_DEBUG
		assert(item!=NULL) ;
#endif
		RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet*>(*it) ;

		if(vitem != NULL)
			for(std::list<RsTlvKeyValue>::const_iterator kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit) 
				if(kit->key == "P3EasyTransport_CONFIG_ATRANSMIT")
					_atransmit = pop_int_value(kit->value) ;
				else if(kit->key == "P3EasyTransport_CONFIG_VOICEHOLD")
					_voice_hold = pop_int_value(kit->value) ;
				else if(kit->key == "P3EasyTransport_CONFIG_VADMIN")
					_vadmin = pop_int_value(kit->value) ;
				else if(kit->key == "P3EasyTransport_CONFIG_VADMAX")
					_vadmax = pop_int_value(kit->value) ;
				else if(kit->key == "P3EasyTransport_CONFIG_NOISE_SUP")
					_noise_suppress = pop_int_value(kit->value) ;
				else if(kit->key == "P3EasyTransport_CONFIG_MIN_LOUDN")
					_min_loudness = pop_int_value(kit->value) ;
				else if(kit->key == "P3EasyTransport_CONFIG_ECHO_CNCL")
					_echo_cancel = pop_int_value(kit->value) ;

		delete vitem ;
	}

	return true ;
}

RsSerialiser *p3EasyTransport::setupSerialiser()
{
	RsSerialiser *rsSerialiser = new RsSerialiser();
	rsSerialiser->addSerialType(new RsEasyTransportSerialiser());
	rsSerialiser->addSerialType(new RsGeneralConfigSerialiser());

	return rsSerialiser ;
}










