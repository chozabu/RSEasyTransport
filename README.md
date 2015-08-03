RS .6 EasyTransport Plugin
==================

This plugin should provide a simple interface to other plugins, allowing them to talk across a network easily

##Example

###include easy transport

    #include "interface/rsEasyTransport.h"


###connect

    connect(rsEasyTransport->mNotify, SIGNAL(NeMsgArrived(RsPeerId,QString)), this , SLOT(NeMsgArrived(RsPeerId,QString)));

###send message

    rsEasyTransport->msg_all(ui->lineEdit->text().toStdString());



This is based on the ExamplePlugin, and in turn VOIP plugin.

