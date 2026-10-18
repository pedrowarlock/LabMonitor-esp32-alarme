#ifndef DASHBOARD_LINK_HPP
#define DASHBOARD_LINK_HPP

#include <Arduino.h>
#include "main.h"

struct DashboardLinkInfo {
  bool linked;
  String serverUrl;
  String externalId;
  bool hasSent;
  bool lastSendOk;
  String lastMessage;
};

DashboardLinkInfo dashboardLinkObterInfo();
bool dashboardLinkVincular(const String& serverUrl, const String& email, String& password,
                           const String& name, const String& location, bool allowInsecureHttp,
                           String& message);
bool dashboardLinkRemover(String& message);
bool dashboardLinkMonitorarConexao();
void dashboardLinkProcessarPendencias();
void dashboardLinkEnviar(const DadosSistema& dados, const String& configuredName,
                         bool alertaMudou = false, bool forcarEnvio = false);

#endif
