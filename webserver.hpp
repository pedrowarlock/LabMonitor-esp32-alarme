#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <Arduino.h>
#include <WebServer.h>
#include "alerts.hpp"

WebServer* webserverIniciar(int porta);
void webserverServidor(WebServer* server);
void webserverAtualizarDados(const DadosSistema& dados);
bool webserverOperacaoAtiva();
void webserverConectarWifiConfiguradoAsync();
bool webserverResetDeFabricaAsync();
void webserverDashboardEnviarAsync(const DadosSistema& dados, const String& configuredName,
								  bool alertaMudou = false, bool forcarEnvio = false);
void webserverVerificarInternetAsync();
void webserverHistoricoRegistrarAsync(const DadosSistema& dados);

#endif
