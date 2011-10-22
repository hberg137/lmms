/*
 * VstEffectControls.cpp - controls for VST effect plugins
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include <QtXml/QDomElement>

#include "VstEffectControls.h"
#include "VstEffect.h"

#include "MainWindow.h"
#include <QtGui/QMdiArea>
#include <QApplication>



VstEffectControls::VstEffectControls( VstEffect * _eff ) :
	EffectControls( _eff ),
	m_effect( _eff ),
	m_subWindow( NULL ),
	knobFModel( NULL ),
	vstKnobs( NULL ),
	ctrHandle( NULL ),
	lastPosInMenu (0)
//	m_presetLabel ( NULL )
{
	menu = new QMenu;
	connect( menu, SIGNAL( aboutToShow() ), this, SLOT( updateMenu() ) );
}




VstEffectControls::~VstEffectControls()
{
	delete ctrHandle;
	ctrHandle = NULL;
}




void VstEffectControls::loadSettings( const QDomElement & _this )
{
	m_effect->closePlugin();
	m_effect->openPlugin( _this.attribute( "plugin" ) );
	m_effect->m_pluginMutex.lock();
	if( m_effect->m_plugin != NULL )
	{
		m_effect->m_plugin->loadSettings( _this );

		const QMap<QString, QString> & dump = m_effect->m_plugin->parameterDump();
		int paramCount = (dump).size();
		char paramStr[35];
		vstKnobs = new knob *[paramCount];
		knobFModel = new FloatModel *[paramCount];
		QStringList list1;
		QWidget * xx = new QWidget();
		for (int i = 0; i < paramCount; i++) {
			sprintf( paramStr, "param%d", i);
			list1 = dump[paramStr].split(":");

			vstKnobs[i] = new knob( knobBright_26, xx );
			vstKnobs[i]->setHintText( list1.at(1) + ":", "");
			vstKnobs[i]->setLabel( list1.at(1).left(15) );

			knobFModel[i] = new FloatModel( (list1.at(2)).toFloat(), 0.0f, 1.0f, 0.01f, this, QString::number(i) );
			knobFModel[i]->loadSettings( _this, paramStr );
			connect( knobFModel[i], SIGNAL( dataChanged() ), this, SLOT( setParameter() ) );

			vstKnobs[i]->setModel( knobFModel[i] );
		}

	}
	m_effect->m_pluginMutex.unlock();
}




void VstEffectControls::setParameter( void )
{

	Model *action = qobject_cast<Model *>(sender());
	int knobUNID = action->displayName().toInt();

	if ( m_effect->m_plugin != NULL ) {
		m_effect->m_plugin->setParam( knobUNID, knobFModel[knobUNID]->value() );
	}
}




void VstEffectControls::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "plugin", m_effect->m_key.attributes["file"] );
	m_effect->m_pluginMutex.lock();
	if( m_effect->m_plugin != NULL )
	{
		m_effect->m_plugin->saveSettings( _doc, _this );
		if (knobFModel != NULL) {
			const QMap<QString, QString> & dump = m_effect->m_plugin->parameterDump();
			int paramCount = (dump).size();
			char paramStr[35];
			for (int i = 0; i < paramCount; i++)
				if (knobFModel[i]->isAutomated() || knobFModel[i]->getControllerConnection()) {
					sprintf( paramStr, "param%d", i);
					knobFModel[i]->saveSettings( _doc, _this, paramStr );
				}
		}
	}
	m_effect->m_pluginMutex.unlock();
}




int VstEffectControls::controlCount()
{
	return m_effect->m_plugin != NULL &&
		m_effect->m_plugin->hasEditor() ?  1 : 0;
}




void VstEffectControls::managePlugin( void )
{
	if ( m_effect->m_plugin != NULL && m_subWindow == NULL ) {
		manageVSTEffectView * tt = new manageVSTEffectView( m_effect, this);
		ctrHandle = (QObject *)tt;
	} else if (m_subWindow != NULL) {
		m_scrollArea->show();
		m_subWindow->show();
	}
}





void VstEffectControls::savePreset( void )
{

	if ( m_effect->m_plugin != NULL ) {
		m_effect->m_plugin->savePreset( );
/*    		bool converted;
    		QString str = m_vi->m_plugin->presetString().section("/", 0, 0);
     		if (str != "")
   			lastPosInMenu = str.toInt(&converted, 10) - 1;
		QWidget::update();*/
	}

}




void VstEffectControls::updateMenu( void )
{

	// get all presets -
	if ( m_effect->m_plugin != NULL ) {
		m_effect->m_plugin->loadPrograms( 1 );
		///QWidget::update();

     		QString str = m_effect->m_plugin->presetsString();

    		QStringList list1 = str.split("|");

     		QMenu * to_menu = m_selPresetButton->menu();
    		to_menu->clear();

    		QAction *presetActions[list1.size()];

     		for (int i = 0; i < list1.size(); i++) {
			presetActions[i] = new QAction(this);
			connect(presetActions[i], SIGNAL(triggered()), this, SLOT(selPreset()));

        		presetActions[i]->setText(QString("%1. %2").arg(QString::number(i+1), list1.at(i)));
        		presetActions[i]->setData(i);
			if (i == lastPosInMenu) {
        			presetActions[i]->setIcon(embed::getIconPixmap( "sample_file", 16, 16 ));
			} else  presetActions[i]->setIcon(embed::getIconPixmap( "edit_copy", 16, 16 ));
			to_menu->addAction( presetActions[i] );
     		}

	}

}




void VstEffectControls::openPreset( void )
{

	if ( m_effect->m_plugin != NULL ) {
		m_effect->m_plugin->openPreset( );
    		bool converted;
    		QString str = m_effect->m_plugin->presetString().section("/", 0, 0);
     		if (str != "")
   			lastPosInMenu = str.toInt(&converted, 10) - 1;
		//QWidget::update();
	}

}




void VstEffectControls::rollPreset( void )
{

	if ( m_effect->m_plugin != NULL ) {
		m_effect->m_plugin->rollPreset( 1 );
    		bool converted;
    		QString str = m_effect->m_plugin->presetString().section("/", 0, 0);
     		if (str != "")
   			lastPosInMenu = str.toInt(&converted, 10) - 1;
		//QWidget::update();
	}
}




void VstEffectControls::rolrPreset( void )
{

	if ( m_effect->m_plugin != NULL ) {
		m_effect->m_plugin->rollPreset( -1 );
    		bool converted;
    		QString str = m_effect->m_plugin->presetString().section("/", 0, 0);
     		if (str != "")
   			lastPosInMenu = str.toInt(&converted, 10) - 1;
		//QWidget::update();
	}
}




void VstEffectControls::selPreset( void )
{

     QAction *action = qobject_cast<QAction *>(sender());
     if (action)
         if ( m_effect->m_plugin != NULL ) {
		lastPosInMenu = action->data().toInt();
		m_effect->m_plugin->rollPreset( lastPosInMenu + 2 );
		//QWidget::update();
	 }
}




void VstEffectControls::paintEvent( QPaintEvent * )
{

}




manageVSTEffectView::manageVSTEffectView( VstEffect * _eff, VstEffectControls * m_vi ) :
	m_effect( _eff )
{
	m_vi2 = m_vi;
	widget = new QWidget();
        m_vi->m_scrollArea = new QScrollArea( widget );
	l = new QGridLayout( widget );

	m_vi->m_subWindow = engine::mainWindow()->workspace()->addSubWindow(new QMdiSubWindow, Qt::SubWindow | 
			Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
	m_vi->m_subWindow->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	m_vi->m_subWindow->setFixedSize( 960, 300);
	m_vi->m_subWindow->setWidget(m_vi->m_scrollArea);
	m_vi->m_subWindow->setWindowTitle(_eff->m_plugin->name());
	m_vi->m_subWindow->setWindowIcon( PLUGIN_NAME::getIconPixmap( "logo" ) );
	//m_vi->m_subWindow->setAttribute(Qt::WA_DeleteOnClose);


	l->setContentsMargins( 20, 10, 10, 10 );
	l->setVerticalSpacing( 10 );
	l->setHorizontalSpacing( 23 );

	m_syncButton = new QPushButton( tr( "VST Sync" ), widget );
	connect( m_syncButton, SIGNAL( clicked() ), this,
							SLOT( syncPlugin() ) );
	m_syncButton->setWhatsThis(
		tr( "Click here if you want to synchronize all parameters with VST plugin." ) );

	l->addWidget( m_syncButton, 0, 0, 1, 2, Qt::AlignLeft );

	const QMap<QString, QString> & dump = m_effect->m_plugin->parameterDump();
	int paramCount = (dump).size();

	bool isVstKnobs = true, isKnobFModel = true;


	if (m_vi->vstKnobs == NULL) {
		m_vi->vstKnobs = new knob *[paramCount];
		isVstKnobs = false;
	}
	if (m_vi->knobFModel == NULL) {
		m_vi->knobFModel = new FloatModel *[paramCount];
		isKnobFModel = false;
	}

	char paramStr[35];
	QStringList list1;

	if (isVstKnobs == false) {
		for (int i = 0; i < paramCount; i++) {
			sprintf( paramStr, "param%d", i);
    			list1 = dump[paramStr].split(":");

			m_vi->vstKnobs[i] = new knob( knobBright_26, widget);
			m_vi->vstKnobs[i]->setHintText( list1.at(1) + ":", "");
			m_vi->vstKnobs[i]->setLabel( list1.at(1).left(15) );

			sprintf( paramStr, "%d", i);
			m_vi->knobFModel[i] = new FloatModel( (list1.at(2)).toFloat(), 0.0f, 1.0f, 0.01f, 
					_eff, tr( paramStr ) );
			connect( m_vi->knobFModel[i], SIGNAL( dataChanged() ), this, SLOT( setParameter() ) );
			m_vi->vstKnobs[i] ->setModel( m_vi->knobFModel[i] );
		}
	}

	int i = 0;
	for (int lrow = 0+1; lrow < (int(paramCount / 10) + 1)+1; lrow++) {
		for (int lcolumn = 0; lcolumn < 10; lcolumn++) {
			if (i < paramCount)
				l->addWidget( m_vi->vstKnobs[i], lrow, lcolumn, Qt::AlignCenter );
			i++;
		}
	}

	l->setRowStretch( (int(paramCount / 10) + 1), 1 );
	l->setColumnStretch( 10, 1 );

	widget->setLayout(l);
	widget->setAutoFillBackground(true);

	m_vi->m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_vi->m_scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_vi->m_scrollArea->setPalette( QApplication::palette( m_vi->m_scrollArea ) );
	m_vi->m_scrollArea->setMinimumHeight( 64 );

	m_vi->m_scrollArea->setWidget( widget );

	m_vi->m_subWindow->show();
}




void manageVSTEffectView::syncPlugin( void )
{
	char paramStr[35];
	QStringList list1;
	const QMap<QString, QString> & dump = m_effect->m_plugin->parameterDump();
	float f;

	for (int i = 0; i<(dump).size(); i++) {
		sprintf( paramStr, "param%d", i);
    		list1 = dump[paramStr].split(":");
		f = (list1.at(2)).toFloat();
		m_vi2->knobFModel[i]->setValue(f);
		m_vi2->knobFModel[i]->setInitValue(f);
	}
}




void manageVSTEffectView::setParameter( void )
{

	Model *action = qobject_cast<Model *>(sender());
	int knobUNID = action->displayName().toInt();

	if ( m_effect->m_plugin != NULL ) {
		m_effect->m_plugin->setParam( knobUNID, m_vi2->knobFModel[knobUNID]->value() );
	}
}




manageVSTEffectView::~manageVSTEffectView()
{
	delete m_vi2->m_subWindow;
	m_vi2->m_subWindow = NULL;
}




#include "moc_VstEffectControls.cxx"

