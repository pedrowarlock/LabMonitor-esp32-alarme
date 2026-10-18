#include "auth.hpp"

#include "storage.hpp"

namespace {
  String gerarToken(const String& username, const String& ip) {
    uint32_t seed = millis() ^ micros() ^ username.length() ^ ip.length();
    String token = "lab_";
    token += username;
    token += "_";
    token += String(seed, HEX);
    return token;
  }
}

bool login(const String& username, const String& password, const String& ip, String& token, String& message) {
  if (username.isEmpty() || password.isEmpty()) {
    token = "";
    message = "Usuário e senha são obrigatórios.";
    return false;
  }

  std::vector<Usuario> usuarios = storageCarregarUsuarios();
  for (const auto& usuario : usuarios) {
    if (!usuario.username.equalsIgnoreCase(username)) {
      continue;
    }

    String senhaHash = storageHashSenha(password);
    if (!usuario.passwordHash.equalsIgnoreCase(senhaHash)) {
      token = "";
      message = "Usuário ou senha inválidos.";
      return false;
    }

    token = gerarToken(usuario.username, ip);
    message = "Login realizado com sucesso.";
    return true;
  }

  token = "";
  message = "Usuário ou senha inválidos.";
  return false;
}
