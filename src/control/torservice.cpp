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
 * \file torservice.cpp
 * \version $Id$
 * \brief Starts, stops, installs, and uninstalls a Tor service (Win32).
 */

#include "torservice.h"

/** Returned by TorService::exitCode() when we are unable to determine the
 * actual exit code of the service (unless, of course, Tor returns -999999). */
#define UNKNOWN_EXIT_CODE     -999999


/** Returns true if services are supported. */
bool
TorService::isSupported()
{
  return (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based);
}

/** Default ctor. */
TorService::TorService(QObject* parent)
  : QObject(parent)
{
  _manager = NULL;
  _service = NULL;

  initialize();
}

/** Default dtor. */
TorService::~TorService()
{
  close();
}

void
TorService::close()
{
  if (_service) {
    CloseServiceHandle(_service);
    _service = NULL;
  }
  if (_manager) {
    CloseServiceHandle(_manager);
    _manager = NULL;
  }
}

/** Initializes the service and service manager. */
void
TorService::initialize()
{
  /* If services are supported, initialize the manager and service */
  if (isSupported()) {
    /* Open service manager */
    if (_manager == NULL) {
    _manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    }
    /* Open tor service */
    if (_service == NULL) {
    _service = OpenServiceA(_manager, TOR_SERVICE_NAME, TOR_SERVICE_ACCESS);
    }
  }
}

/** Returns true if the Tor service is installed. */
bool
TorService::isInstalled()
{
  initialize();
  return (_service != NULL);
}

/** Returns true if the Tor service is running. */
bool
TorService::isRunning()
{
  initialize();
  return (status() == SERVICE_RUNNING);
}

/** Starts Tor service. */
void
TorService::start()
{
  if (!isSupported()) {
    emit startFailed(tr("Services not supported on this platform."));
    return;
  }
  if (!isInstalled()) {
    emit startFailed(tr("The Tor service is not installed."));
    return;
  }

  /* Make sure we're initialized */
  initialize();

  /* Starting a service can take up to 30 seconds! */
  if (!isRunning()) StartService(_service, 0, NULL); 
  
  int tries = 0;
  while (true) {
    if (isRunning()) break;
    tries++;
    Sleep(1000);
    if (tries > 5) break;
  }

  if (isRunning()) {
    emit started();
  } else {
    emit startFailed(tr("Unable to start Tor service."));
  }
}

/** Stops Tor service. */
void
TorService::stop()
{
  if (isRunning()) {
    SERVICE_STATUS stat;
    stat.dwCurrentState = SERVICE_RUNNING;
    ControlService(_service, SERVICE_CONTROL_STOP, &stat);

    int tries = 0;
    while (true) {
      if (status() ==  SERVICE_STOPPED) break;
      tries++;
      Sleep(1000);
      if (tries > 5) break;
    }
  }

  if (!isRunning()) {
    /* Emit the signal that we stopped and the service's exit code and status. */
    emit finished(exitCode(), exitStatus());
  }
}

/** Returns the exit code of the last Tor service that finished. */
int
TorService::exitCode()
{
  int exitCode = UNKNOWN_EXIT_CODE;
  
  if (isSupported() && _manager && _service) {
    SERVICE_STATUS s;

    if (QueryServiceStatus(_service, &s)) {
      /* Services return one exit code, but it could be in one of two
       * variables. Fun. */
      exitCode = (int)(s.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR
                                              ? s.dwServiceSpecificExitCode
                                              : s.dwWin32ExitCode);
    }
  }
  return exitCode;
}

/** Returns the exit status of the last Tor service that finished. */
QProcess::ExitStatus
TorService::exitStatus()
{
  /* NT services don't really have an equivalent to QProcess::CrashExit, so 
   * this just returns QProcess::NormalExit. Tor _could_ set
   * dwServiceSpecificExitCode to some magic value when it starts and then
   * set it to the real exit code when Tor exits. Then we would know if the
   * service crashed when dwServiceSpecificExitCode is still the magic value.
   * However, I don't care and it doesn't really matter anyway. */
  return QProcess::NormalExit;
}

/** Installs the Tor service. */
bool
TorService::install(const QString &torPath, const QString &torrc,
                    quint16 controlPort)
{
  if (!isSupported()) return false;

  if (!isInstalled()) {
    QString command = QString("\"%1\" --nt-service -f \"%2\" ControlPort %3")
                                                 .arg(torPath)
                                                 .arg(torrc)
                                                 .arg(controlPort);

    _service = CreateServiceA(_manager, TOR_SERVICE_NAME, TOR_SERVICE_DISP,
                              TOR_SERVICE_ACCESS, SERVICE_WIN32_OWN_PROCESS,
                              SERVICE_AUTO_START, SERVICE_ERROR_IGNORE,
                              command.toAscii().data(), NULL, NULL, NULL, NULL, "");

    if (_service != NULL) {
      SERVICE_DESCRIPTION desc;
      desc.lpDescription = TOR_SERVICE_DESC;
      ChangeServiceConfig2(_service, SERVICE_CONFIG_DESCRIPTION, &desc);
    }
  }
  return isInstalled();
}

/** Removes the Tor service. */
bool
TorService::remove()
{
  if (!isSupported()) return false;
  if (isInstalled()) {
    /* Stop the service */
    stop();

    /* Delete the service */
    if (!DeleteService(_service)) return false;
    
    /* Release references to manager and service */
    close();
  }
  return !isInstalled();
}


/** Gets the status of the Tor service. */
DWORD
TorService::status()
{
  if (!(isSupported() && _manager && _service)) return DWORD(-1);

  SERVICE_STATUS s;
  DWORD stat = SERVICE_ERROR;

  if (QueryServiceStatus(_service, &s)) {
    stat = s.dwCurrentState;
  } 
  return stat;
}

