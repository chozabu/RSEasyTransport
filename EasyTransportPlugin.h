/* this is the central part of the plugin */
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

/*EasyTransport*/
#include "services/p3EasyTransport.h"

/*libretroshare"*/
#include <retroshare/rsplugin.h>

#include "gui/NEMainpage.h"

class EasyTransportGUIHandler ;
class EasyTransportNotify ;

class EasyTransportPlugin: public RsPlugin
{
	public:
		EasyTransportPlugin() ;
		virtual ~EasyTransportPlugin() {}

		virtual p3Service      *p3_service() 			const	;
		virtual uint16_t        rs_service_id()         const { return RS_SERVICE_TYPE_EasyTransport_PLUGIN ; }
		//virtual ConfigPage     *qt_config_page()        const ;
		virtual QDialog        *qt_about_page()         const ;
		//virtual ChatWidgetHolder *qt_get_chat_widget_holder(ChatWidget *chatWidget) const ;
		
		virtual QIcon *qt_icon() const;
		virtual QTranslator    *qt_translator(QApplication *app, const QString& languageCode, const QString& externalDir) const;
		virtual void           qt_sound_events(SoundEvents &events) const;

		virtual void getPluginVersion(int& major, int& minor, int &build, int& svn_rev) const ;
		virtual void setPlugInHandler(RsPluginHandler *pgHandler);

		virtual std::string configurationFileName() const { return "net_example.cfg" ; }

		virtual std::string getShortPluginDescription() const ;
		virtual std::string getPluginName() const;
		virtual void setInterfaces(RsPlugInInterfaces& interfaces);

		//================================== RsPlugin Notify ==================================//
		//virtual ToasterNotify *qt_toasterNotify();

		virtual MainPage       *qt_page()       			const	;

	private:
		mutable p3EasyTransport *mEasyTransport ;
		mutable RsPluginHandler *mPlugInHandler;
		mutable RsPeers* mPeers;
		mutable ConfigPage *config_page ;
		mutable QIcon *mIcon;
		mutable MainPage* mainpage ;

		EasyTransportNotify *mEasyTransportNotify ;
		EasyTransportGUIHandler *mEasyTransportGUIHandler ;
};

