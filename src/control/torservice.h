/****************************************************************
 *  Vidalia is distributed under the following license:
 *
 *  Copyright (C) 2006,  Matt Edman, Justin Hipple
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

/** 
 * \file torservice.h
 * \version $Id$
 * \brief Starts, stops, installs, and uninstalls a Tor service (Win32).
 */

#ifndef _TORSERVICE_H
#define _TORSERVICE_H

#include <QObject>
#include <QProcess>

#include <windows.h>
#define TOR_SERVICE_NAME "tor"
#define TOR_SERVICE_DISP "Tor Win32 Service"
#define TOR_SERVICE_DESC \
  TEXT("Provides an anonymous Internet communication system.")
#define TOR_SERVICE_ACCESS SERVICE_ALL_ACCESS
#define SERVICE_ERROR 8


class TorService : public QObject
{
  Q_OBJECT

public:
  /* Returns if services are supported. */
  static bool isSupported();

  /** Default ctor. */
  TorService(QObject* parent = 0);
  /** Default dtor. */
  ~TorService();

  /** Returns true if the Tor service is installed. */
  bool isInstalled();
  /** Returns true if the Tor service is running. */
  bool isRunning();
  /** Starts the Tor service. Emits started on success. */
  void start();
  /** Stops the Tor service. Emits finished on success. */
  void stop();
  /** Returns the exit code of the last Tor service that finished. */
  int exitCode();
  /** Returns the exit status of the last Tor service that finished. */
  QProcess::ExitStatus exitStatus();
  /** Installs the Tor service. */
  bool install(const QString &torPath, const QString &torrc,
               quint16 controlPort);
  /** Removes the Tor service. */
  bool remove();

signals:
  /** Called when the service gets started. */
  void started();
  /** Called when the service gets stopped. */
  void finished(int exitCode, QProcess::ExitStatus);
  /** Called when there is an error in starting the service. */
  void startFailed(QString error);

private:
  /** Initializes the service and the service manager. */
  void initialize();
  /** Closes the service and the service manager. */
  void close();
  /** Gets the status of the Tor service. */
  DWORD status(); 
  
  SC_HANDLE _manager; /** Handle to a service manager object. */
  SC_HANDLE _service; /** Handle to the Tor service object. */
};

#endif

