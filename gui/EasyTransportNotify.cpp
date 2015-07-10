#include "EasyTransportNotify.h"

EasyTransportNotify::EasyTransportNotify(QObject *parent) : QObject(parent)
{

}

void EasyTransportNotify::notifyReceivedPaint(const RsPeerId &peer_id, int x, int y)
{
	std::cout << "pNotify Recvd paint from: " << peer_id;
	std::cout << " at " << x << " , " << y;
	std::cout << std::endl;
	emit NePaintArrived(peer_id, x, y);
}


void EasyTransportNotify::notifyReceivedMsg(const RsPeerId& peer_id, QString str)
{
	std::cout << "pNotify Recvd Packet from: " << peer_id;
	std::cout << " saying " << str.toStdString();
	std::cout << std::endl;
	emit NeMsgArrived(peer_id, str) ;
}
