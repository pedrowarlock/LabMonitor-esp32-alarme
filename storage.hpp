#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <Arduino.h>
#include <time.h>
#include <vector>
#include "alerts.hpp"

struct Usuario {
  String username;
  String passwordHash;
  String role;
};

struct RegistroAlerta {
  time_t timestamp;
  String evento;
};

struct PontoHistorico {
  time_t timestamp;
  float temperatura;
  float umidade;
  uint16_t gas_adc;
};

struct VinculoDashboard {
  String serverUrl;
  String deviceToken;
  String externalId;
};

Config storageCarregarConfig();
void storageSalvarConfig(const Config& config);
bool storageCarregarAcessoVisitante();
bool storageSalvarAcessoVisitante(bool permitido);
std::vector<Usuario> storageCarregarUsuarios();
void storageSalvarUsuarios(const std::vector<Usuario>& usuarios);
String storageHashSenha(const String& senha);
bool storageCriarUsuario(const Usuario& usuario);
bool storageRemoverUsuario(const String& username);
bool storageAtualizarSenha(const String& username, const String& senha);
std::vector<RegistroAlerta> storageCarregarLogs();
void storageAdicionarLog(const RegistroAlerta& registro);
void storageSalvarLogs(const std::vector<RegistroAlerta>& logs);
void storageLimparLogs();
void storageRegistrarEvento(const String& evento);
bool storageInicializarHistorico();
void storageRegistrarLeituraHistorica(float temperatura, float umidade, uint16_t gasAdc, time_t timestamp);
std::vector<PontoHistorico> storageCarregarHistorico(time_t inicio, time_t fim, size_t maxPontos);
void storageLimparHistorico();
VinculoDashboard storageCarregarVinculoDashboard();
bool storageSalvarVinculoDashboard(const VinculoDashboard& vinculo);
bool storageRemoverVinculoDashboard();
VinculoDashboard storageCarregarRemocaoDashboardPendente();
bool storageSalvarRemocaoDashboardPendente(const VinculoDashboard& vinculo);
void storageRemoverRemocaoDashboardPendente();

#endif
