const state = {
            token: localStorage.getItem('labToken') || '',
            language: localStorage.getItem('labLanguage') || 'pt-BR',
            user: null,
            role: 'visitor',
            status: {},
            config: {},
            users: [],
            logs: [],
            history: null,
            historyDays: 1,
            historyLoadedAt: 0
          };
          let dashboardServerUrlEdited = false;

          const translations = {
            'Visão geral': 'Overview', 'Rede': 'Network', 'Sensores': 'Sensors', 'Usuários': 'Users',
            'Manutenção': 'Maintenance', 'Logs de alertas': 'Alert logs', 'Dashboard de Supervisão': 'Monitoring Dashboard',
            'Sobre': 'About', 'Este projeto foi desenvolvido em um projeto gerenciado pelo professor Carlos Augusto Sicsú, na UERJ - Universidade do Estado do Rio de Janeiro.': 'This project was developed under the management of Professor Carlos Augusto Sicsú at UERJ - Rio de Janeiro State University.',
            'MONITORAMENTO DE LABORATÓRIO': 'LABORATORY MONITORING', 'Sistema ativo': 'System active',
            'Sair': 'Logout', 'Usuário': 'User', 'Entrar': 'Login', 'Entrar como visitante': 'Enter as visitor',
            'ACESSO AO SISTEMA': 'SYSTEM ACCESS', 'Entre com suas credenciais para acessar o painel.': 'Enter your credentials to access the dashboard.',
            'Senha': 'Password', 'Nome da rede (SSID)': 'Network name (SSID)', 'Tipo de segurança': 'Security type',
            'Aberta': 'Open', 'WPA/WPA2 pessoal': 'WPA/WPA2 personal', 'WPA2 Enterprise': 'WPA2 Enterprise',
            'WPA Enterprise': 'WPA Enterprise', 'Senha da rede': 'Network password', 'Credenciais Enterprise': 'Enterprise credentials',
            'Identidade': 'Identity', 'Usuário': 'Username', 'Salvar e conectar': 'Save and connect',
            'Excluir conexão salva e voltar ao AP': 'Delete saved connection and return to AP',
            'Configuração de rede': 'Network configuration', 'Escanear redes disponíveis': 'Scan available networks',
            'A conexão atual será preservada durante o scan.': 'The current connection will be preserved during the scan.',
            'Nenhuma rede escaneada.': 'No networks scanned.', 'Visão geral': 'Overview',
            '': 'Laboratory device XYZ', 'Data e hora: --': 'Date and time: --',
            'Temperatura': 'Temperature', 'Umidade': 'Humidity', 'Gás': 'Gas', 'Rede e internet': 'Network and internet',
            'Internet: verificando...': 'Internet: checking...', 'Wi‑Fi: aguardando': 'Wi-Fi: waiting',
            'Sem alarmes': 'No alarms', 'Desativar alarme sonoro': 'Mute alarm sound', 'Últimos eventos': 'Latest events',
            'Configuração dos sensores': 'Sensor configuration', 'Nome do aparelho': 'Device name',
            'Temperatura mínima': 'Minimum temperature', 'Temperatura máxima': 'Maximum temperature',
            'Umidade mínima': 'Minimum humidity', 'Umidade máxima': 'Maximum humidity', 'Gás crítico (ADC)': 'Critical gas (ADC)',
            'Alertas': 'Alerts', 'Intervalo de leitura (segundos)': 'Reading interval (seconds)',
            'Salvar configurações': 'Save settings', 'Configurar Wi‑Fi': 'Configure Wi-Fi',
            'Gerenciamento de usuários': 'User management', 'Criar usuário': 'Create user', 'Confirmar senha': 'Confirm password',
            'Administrador': 'Administrator', 'Modificar senha': 'Change password', 'Nova senha': 'New password',
            'Confirmar nova senha': 'Confirm new password', 'Salvar nova senha': 'Save new password',
            'Acesso de visitante': 'Visitor access', 'Permitir botão “Entrar como visitante”': 'Allow “Enter as visitor” button',
            'Permissões para usuários comuns': 'Permissions for regular users',
            'Permissões de visualização para visitantes': 'Visitor viewing permissions',
            'Pode visualizar temperatura': 'Can view temperature', 'Pode visualizar umidade': 'Can view humidity',
            'Pode visualizar nível de gás': 'Can view gas level', 'Pode visualizar Wi‑Fi conectado': 'Can view connected Wi-Fi',
            'Pode visualizar nome da rede e status da internet': 'Can view network name and internet status',
            'Pode visualizar eventos': 'Can view events', 'Pode visualizar eventos recentes': 'Can view recent events',
            'Pode desativar alarme sonoro': 'Can mute alarm sound', 'Salvar permissões': 'Save permissions',
            'Manutenção': 'Maintenance', 'Backup das configurações': 'Back up settings', 'Selecionar backup': 'Select backup',
            'Restaurar backup selecionado': 'Restore selected backup', 'Reset de fábrica': 'Factory reset',
            'Data e hora': 'Date and time', 'Configuração manual': 'Manual configuration',
            'Buscar hora atual da web': 'Get current web time', 'Salvar data e hora': 'Save date and time',
            'Logs de alertas': 'Alert logs', 'Apagar logs': 'Delete logs', 'Nenhum evento registrado.': 'No events recorded.',
            'Excluir': 'Delete', 'Tipo': 'Type', 'Ação': 'Action', 'Idioma': 'Language',
            'Fonte ainda não consultada.': 'Source not queried yet.',
            'Variação dos sensores': 'Sensor trends', 'Período do histórico': 'History period',
            '30 dias': '30 days', '15 dias': '15 days', '7 dias': '7 days', '1 dia': '1 day',
            'Carregando histórico...': 'Loading history...',
            'Amostras coletadas a cada minuto.': 'Samples are collected once per minute.',
            'Vincular ao LAB/MONITOR': 'Link to LAB/MONITOR',
            'Use a conta do servidor LAB/MONITOR. O ESP32 envia leituras automaticamente enquanto estiver conectado ao Wi-Fi.': 'Use your LAB/MONITOR server account. The ESP32 sends readings automatically while connected to Wi-Fi.',
            'Endereço do LAB/MONITOR': 'LAB/MONITOR address',
            'HTTPS é recomendado. HTTP está disponível para redes controladas.': 'HTTPS is recommended. HTTP is available for controlled networks.',
            'Entendo que HTTP transmite e-mail, senha e token sem criptografia.': 'I understand HTTP sends email, password, and token without encryption.',
            'E-mail da conta LAB/MONITOR': 'LAB/MONITOR account email',
            'Senha da conta LAB/MONITOR': 'LAB/MONITOR account password',
            'Localização (opcional)': 'Location (optional)',
            'Vincular aparelho': 'Link device',
            'Aparelho vinculado': 'Device linked',
            'Remover vínculo': 'Remove link',
            'Wi-Fi': 'Wi-Fi', 'LAB/MONITOR': 'LAB/MONITOR', 'Aparelho': 'Device',
            'Sensores e alertas': 'Sensors and alerts', 'Contas': 'Accounts',
            'Visitante': 'Visitor', 'Permissões': 'Permissions',
            'Backup e reset': 'Backup and reset', 'Data e hora': 'Date and time',
            'Conexão atual': 'Current connection', 'Rede conectada': 'Connected network',
            'Endereço do aparelho': 'Device address', 'Tipo de conexão': 'Connection type',
            'Internet: OK': 'Internet: OK', 'Internet: indisponível': 'Internet: unavailable',
            'Nenhuma rede conectada': 'No network connected', 'Aguardando conexão': 'Waiting for connection',
            'Salvar aparelho': 'Save device', 'Excluir rede salva e voltar ao ponto de acesso': 'Forget saved network and return to access point',
            'HTTPS valida o certificado do servidor. HTTP não criptografa credenciais ou leituras e só deve ser usado em rede confiável. A página local do ESP32 também usa HTTP.': 'HTTPS validates the server certificate. HTTP does not encrypt credentials or readings and should only be used on a trusted network. The local ESP32 page also uses HTTP.'
          };

          function translateEvent(evento) {
            return String(evento || '')
              .replace('ENTRADA EM CRITICO: ', 'CRITICAL: ')
              .replace('RETORNO AO NORMAL: ', 'RETURNED TO NORMAL: ')
              .replace('temperatura', 'temperature')
              .replace('umidade', 'humidity')
              .replace('gas', 'gas');
          }

          function applyLanguage() {
            const english = state.language === 'en';
            document.documentElement.lang = english ? 'en' : 'pt-BR';
            const walker = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT);
            const nodes = [];
            while (walker.nextNode()) nodes.push(walker.currentNode);
            nodes.forEach((node) => {
              if (node.parentElement && ['SCRIPT', 'STYLE'].includes(node.parentElement.tagName)) return;
              const text = node.nodeValue.trim();
              if (!text) return;
              if (!node.ptText) node.ptText = text;
              const translated = english ? (translations[node.ptText] || node.ptText) : node.ptText;
              if (translated) {
                node.nodeValue = node.nodeValue.replace(text, translated);
              }
            });
            const language = document.getElementById('languageSelect');
            if (language) language.value = state.language;
            const loginLanguage = document.getElementById('loginLanguageSelect');
            if (loginLanguage) loginLanguage.value = state.language;
          }

          function showMessage(text, isError = true) {
            const el = document.getElementById('loginMessage');
            if (!el) return;
            el.textContent = text;
            el.style.color = isError ? 'var(--red)' : 'var(--green)';
          }

          async function request(path, options = {}) {
            const headers = { ...(options.headers || {}) };
            if (state.token) {
              headers.Authorization = `Bearer ${state.token}`;
            }
            const response = await fetch(path, {
              ...options,
              headers
            });
            const data = await response.json().catch(() => ({}));
            if (!response.ok) {
              throw new Error(data.message || `HTTP ${response.status}`);
            }
            return data;
          }

          async function acompanharOperacao(operationId, timeoutMs = 120000) {
            const deadline = Date.now() + timeoutMs;
            while (Date.now() < deadline) {
              const operation = await request(`/api/operation?id=${encodeURIComponent(operationId)}`);
              if (operation.done) return operation;
              await new Promise((resolve) => setTimeout(resolve, 500));
            }
            throw new Error('A operação continua em andamento. Consulte novamente em instantes.');
          }

          function setTab(tab) {
            document.querySelectorAll('.nav-btn').forEach((btn) => {
              btn.classList.toggle('active', btn.dataset.tab === tab);
            });
            document.querySelectorAll('[id^="tab-"]').forEach((section) => {
              section.classList.toggle('hidden', section.id !== `tab-${tab}`);
            });
            const firstSubmenu = document.querySelector(`[data-submenu-group="${tab}"]`);
            if (firstSubmenu) setSubmenu(tab, firstSubmenu.dataset.submenu);
            if (tab === 'network' && state.role === 'admin') loadDashboardLink();
          }

          function setSubmenu(group, submenu) {
            document.querySelectorAll(`[data-submenu-group="${group}"]`).forEach((button) => {
              const active = button.dataset.submenu === submenu;
              button.classList.toggle('active', active);
              button.setAttribute('aria-selected', String(active));
            });
            document.querySelectorAll(`[data-subpanel-group="${group}"]`).forEach((panel) => {
              panel.classList.toggle('hidden', panel.dataset.subpanel !== submenu);
            });
          }

          function renderDashboardLink(data) {
            const linked = !!data.linked;
            document.getElementById('dashboardLinkForm').classList.toggle('hidden', linked);
            document.getElementById('dashboardLinkedState').classList.toggle('hidden', !linked);
            if (linked) {
              document.getElementById('dashboardLinkDetails').textContent = `ID: ${data.externalId} · Servidor: ${data.serverUrl}`;
              const transportStatus = document.getElementById('dashboardTransportStatus');
              const secureTransport = String(data.serverUrl).toLowerCase().startsWith('https://');
              transportStatus.textContent = secureTransport ? 'HTTPS ativo; certificado validado.' : 'HTTP ativo; tráfego sem criptografia.';
              transportStatus.className = secureTransport ? 'muted link-send-ok' : 'muted link-send-error';
              const sendStatus = document.getElementById('dashboardLinkSendStatus');
              sendStatus.textContent = data.hasSent
                ? `${data.lastSendOk ? 'Último envio concluído: ' : 'Último envio com falha: '}${data.lastMessage}`
                : 'Vínculo ativo. Aguardando a primeira leitura.';
              sendStatus.className = data.hasSent && !data.lastSendOk ? 'muted link-send-error' : 'muted link-send-ok';
            } else if (data.serverUrl && !dashboardServerUrlEdited) {
              document.getElementById('dashboardServerUrl').value = data.serverUrl;
              updateDashboardTransportChoice();
            }
          }

          function updateDashboardTransportChoice() {
            const url = document.getElementById('dashboardServerUrl').value.trim().toLowerCase();
            const usesHttp = url.startsWith('http://');
            document.getElementById('dashboardHttpWarning').classList.toggle('hidden', !usesHttp);
            if (!usesHttp) document.getElementById('dashboardHttpAck').checked = false;
          }

          async function loadDashboardLink() {
            for (let attempt = 0; attempt < 20; attempt += 1) {
              try {
                const data = await request('/api/dashboard-link');
                if (!data.busy) {
                  renderDashboardLink(data);
                  return;
                }
              } catch (error) {
                document.getElementById('dashboardLinkMessage').textContent = error.message || 'Não foi possível consultar o vínculo.';
                return;
              }
              await new Promise((resolve) => setTimeout(resolve, 500));
            }
            document.getElementById('dashboardLinkMessage').textContent = 'A comunicação com o dashboard ainda está em andamento.';
          }

          async function linkDashboard() {
            const message = document.getElementById('dashboardLinkMessage');
            const serverUrl = document.getElementById('dashboardServerUrl').value.trim();
            const allowInsecureHttp = serverUrl.toLowerCase().startsWith('http://') && document.getElementById('dashboardHttpAck').checked;
            if (serverUrl.toLowerCase().startsWith('http://') && !allowInsecureHttp) {
              message.textContent = 'Confirme o aviso de segurança para permitir HTTP.';
              return;
            }
            const button = document.getElementById('dashboardLinkBtn');
            button.disabled = true;
            message.textContent = 'Validando a conta e vinculando o aparelho...';
            try {
              const started = await request('/api/dashboard-link/login', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                  serverUrl,
                  email: document.getElementById('dashboardLinkEmail').value.trim(),
                  password: document.getElementById('dashboardLinkPassword').value,
                  location: document.getElementById('dashboardLinkLocation').value.trim(),
                  allowInsecureHttp
                })
              });
              document.getElementById('dashboardLinkPassword').value = '';
              document.getElementById('dashboardLinkEmail').value = '';
              message.textContent = started.message || 'Vinculando aparelho...';
              const result = await acompanharOperacao(started.operationId);
              message.textContent = result.message || 'Aparelho vinculado.';
              if (result.ok) await loadDashboardLink();
            } catch (error) {
              message.textContent = error.message || 'Não foi possível vincular o aparelho.';
            } finally {
              button.disabled = false;
            }
          }

          async function unlinkDashboard() {
            if (!window.confirm('Remover o vínculo deste aparelho com o LAB/MONITOR?')) return;
            const button = document.getElementById('dashboardUnlinkBtn');
            const message = document.getElementById('dashboardLinkMessage');
            button.disabled = true;
            try {
              const started = await request('/api/dashboard-link/remove', { method: 'POST' });
              message.textContent = started.message || 'Removendo vínculo...';
              const result = await acompanharOperacao(started.operationId);
              message.textContent = result.message || 'Vínculo removido.';
              if (result.ok) await loadDashboardLink();
            } catch (error) {
              message.textContent = error.message || 'Não foi possível remover o vínculo.';
            } finally {
              button.disabled = false;
            }
          }

          function updateNetworkSecurityFields() {
            const security = document.getElementById('networkSecurity').value;
            const enterprise = ['WPA2-ENTERPRISE', 'WPA-ENTERPRISE'].includes(security);
            document.getElementById('enterpriseFields').classList.toggle('hidden', !enterprise);
            document.getElementById('personalPasswordField').classList.toggle('hidden', enterprise || security === 'OPEN');
            updateEnterpriseMethodFields();
          }

          function updateEnterpriseMethodFields() {
            const method = document.getElementById('enterpriseMethod').value;
            document.getElementById('ttlsPhase2Field').classList.toggle('hidden', method !== 'TTLS');
            document.getElementById('enterpriseIdentity').value = document.getElementById('enterpriseUsername').value;
            const hints = {
              PEAP: 'PEAP-MSCHAPv2 selecionado: usa login e senha.',
              TTLS: 'EAP-TTLS selecionado: usa login, senha e a autenticação interna escolhida.',
              FAST: 'EAP-FAST selecionado: usa login e senha; o servidor pode exigir PAC.',
              TLS: 'EAP-TLS exige certificado do cliente e ainda não está configurado neste firmware.',
              'PEAP-GTC': 'EAP-PEAP-GTC não está disponível neste firmware.',
              SIM: 'EAP-SIM exige um cartão SIM e não está disponível neste firmware.',
              AKA: 'EAP-AKA exige um cartão SIM e não está disponível neste firmware.',
              AKA_PRIME: "EAP-AKA' exige um cartão SIM e não está disponível neste firmware."
            };
            document.getElementById('enterpriseMethodHint').textContent = hints[method] || 'Selecione o método informado pela instituição.';
          }

          async function scanNetworks() {
            const status = document.getElementById('scanStatus');
            const list = document.getElementById('networkList');
            status.textContent = 'Escaneando sem desconectar a rede atual...';
            try {
              let data = await request('/api/networks');
              const deadline = Date.now() + 20000;
              while (data.scanning && Date.now() < deadline) {
                await new Promise((resolve) => setTimeout(resolve, 750));
                data = await request('/api/networks');
              }
              if (data.scanning) throw new Error('O scan demorou demais. Tente novamente.');
              list.innerHTML = (data.networks || []).map((network) => `<button class="secondary network-choice" type="button" data-ssid="${network.ssid}" data-security="${network.security}">${network.ssid} (${network.security})</button>`).join('') || 'Nenhuma rede encontrada.';
              document.querySelectorAll('.network-choice').forEach((button) => button.addEventListener('click', () => {
                const detectedSecurity = button.dataset.security;
                const security = detectedSecurity === 'WPA/WPA2-PSK' ? 'WPA-PSK' : detectedSecurity;
                document.getElementById('networkSsid').value = button.dataset.ssid;
                if (['OPEN', 'WPA-PSK', 'WPA2-ENTERPRISE', 'WPA-ENTERPRISE'].includes(security)) {
                  document.getElementById('networkSecurity').value = security;
                  document.getElementById('networkSecurity').disabled = true;
                  document.getElementById('securityDetectionHint').textContent = `Segurança detectada automaticamente: ${security}.`;
                } else {
                  document.getElementById('networkSecurity').value = 'WPA-PSK';
                  document.getElementById('networkSecurity').disabled = false;
                  document.getElementById('securityDetectionHint').textContent = 'Tipo não identificado; selecione a segurança manualmente.';
                }
                updateNetworkSecurityFields();
                document.getElementById('networkMessage').textContent = `Rede selecionada: ${button.dataset.ssid}. Informe as credenciais.`;
              }));
              status.textContent = 'Scan concluído. A conexão atual foi mantida.';
            } catch (error) {
              status.textContent = error.message || 'Não foi possível escanear as redes.';
            }
          }

          async function saveNetwork() {
            const message = document.getElementById('networkMessage');
            const button = document.getElementById('saveNetworkBtn');
            let operationAccepted = false;
            button.disabled = true;
            try {
              const started = await request('/api/network-config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                  ssid: document.getElementById('networkSsid').value.trim(),
                  password: document.getElementById('networkPassword').value,
                  security: document.getElementById('networkSecurity').value,
                  enterprise_identity: document.getElementById('enterpriseIdentity').value,
                  enterprise_username: document.getElementById('enterpriseUsername').value,
                  enterprise_password: document.getElementById('enterprisePassword').value,
                  enterprise_method: document.getElementById('enterpriseMethod').value,
                  enterprise_ttls_phase2: document.getElementById('enterpriseTtlsPhase2').value
                })
              });
              operationAccepted = true;
              message.textContent = started.message || 'Conectando à rede...';
              const result = await acompanharOperacao(started.operationId, 45000);
              message.textContent = result.message || 'Configuração concluída.';
              if (result.ok) showNetworkRestartOverlay('http://monitor-configurar.local/');
            } catch (error) {
              message.textContent = error.message || 'Não foi possível configurar a rede.';
              if (operationAccepted) {
                showNetworkRestartOverlay('http://monitor-configurar.local/', 'A conexão do aparelho está mudando. Conecte-se à rede configurada ou ao access point.');
              }
            } finally {
              button.disabled = false;
            }
          }

          async function deleteSavedNetwork() {
            const message = document.getElementById('networkMessage');
            if (!window.confirm('Excluir a conexão salva e voltar ao access point?')) return;
            let operationAccepted = false;
            try {
              const started = await request('/api/network-config/delete', { method: 'POST' });
              operationAccepted = true;
              message.textContent = started.message || 'Iniciando access point...';
              const result = await acompanharOperacao(started.operationId, 30000);
              message.textContent = result.message || 'Conexão excluída.';
              if (result.ok) showNetworkRestartOverlay('http://monitor-configurar.local/');
            } catch (error) {
              message.textContent = error.message || 'Não foi possível excluir a conexão salva.';
              if (operationAccepted) {
                showNetworkRestartOverlay('http://monitor-configurar.local/', 'O access point está sendo iniciado. Conecte-se a MONITOR-CONFIGURAR.');
              }
            }
          }

          function showNetworkRestartOverlay(url, messageText = null) {
            const overlay = document.getElementById('networkRestartOverlay');
            const seconds = document.getElementById('networkRestartSeconds');
            const target = document.getElementById('networkRestartUrl');
            const messageEl = document.getElementById('networkRestartMessage');
            const normalizedUrl = String(url || 'http://monitor-configurar.local/');
            const hostHint = normalizedUrl.replace(/^https?:\/\//i, '').replace(/\/$/, '');
            let remaining = 15;
            target.textContent = normalizedUrl;
            messageEl.textContent = messageText || `Reiniciando, você será direcionado automaticamente para ${hostHint}`;
            overlay.classList.remove('hidden');
            seconds.textContent = remaining;
            const timer = window.setInterval(() => {
              remaining -= 1;
              seconds.textContent = remaining;
              if (remaining <= 0) {
                window.clearInterval(timer);
                window.location.href = normalizedUrl;
              }
            }, 1000);
          }

          function buildDeviceRedirectUrl(deviceName) {
            const cleanName = String(deviceName || '').trim();
            const host = cleanName ? `monitor-${cleanName.replace(/[^a-zA-Z0-9-]+/g, '-').replace(/-+/g, '-').replace(/^-|-$/g, '')}` : 'monitor-configurar';
            return `http://${host}.local/`;
          }

          function formatEventTime(timestamp) {
            const value = Number(timestamp || 0);
            return value > 0
              ? new Date(value * 1000).toLocaleString('pt-BR')
              : 'Data não sincronizada';
          }

          function formatHistoryAxisTime(timestamp) {
            const date = new Date(timestamp * 1000);
            const locale = state.language === 'en' ? 'en-US' : 'pt-BR';
            return state.historyDays === 1
              ? date.toLocaleTimeString(locale, { hour: '2-digit', minute: '2-digit' })
              : date.toLocaleDateString(locale, { day: '2-digit', month: '2-digit' });
          }

          function drawHistoryChart(canvasId, seriesIndex, color, unit) {
            const canvas = document.getElementById(canvasId);
            const context = canvas.getContext('2d');
            const width = Math.max(1, Math.floor(canvas.getBoundingClientRect().width));
            const height = 210;
            const ratio = Math.max(1, window.devicePixelRatio || 1);
            canvas.width = width * ratio;
            canvas.height = height * ratio;
            context.setTransform(ratio, 0, 0, ratio, 0, 0);
            context.clearRect(0, 0, width, height);

            const values = (state.history?.points || [])
              .filter((point) => point[seriesIndex] !== null && point[seriesIndex] !== undefined)
              .map((point) => ({ timestamp: Number(point[0]), value: Number(point[seriesIndex]) }))
              .filter((point) => Number.isFinite(point.timestamp) && Number.isFinite(point.value));
            if (!values.length) {
              context.fillStyle = '#6b7280';
              context.font = '12px Arial, sans-serif';
              context.textAlign = 'center';
              const emptyText = state.language === 'en'
                ? (state.history?.available === false ? 'History storage unavailable' : state.history?.timeValid === false ? 'Sync device clock' : 'No samples for this period')
                : (state.history?.available === false ? 'Histórico indisponível' : state.history?.timeValid === false ? 'Sincronize o relógio' : 'Sem amostras neste período');
              context.fillText(emptyText, width / 2, height / 2);
              return;
            }

            const margin = { top: 10, right: 10, bottom: 28, left: 48 };
            const plotWidth = Math.max(1, width - margin.left - margin.right);
            const plotHeight = height - margin.top - margin.bottom;
            let min = Math.min(...values.map((point) => point.value));
            let max = Math.max(...values.map((point) => point.value));
            if (min === max) {
              const padding = Math.max(Math.abs(min) * 0.08, seriesIndex === 3 ? 20 : 1);
              min -= padding;
              max += padding;
            } else {
              const padding = (max - min) * 0.12;
              min -= padding;
              max += padding;
            }

            context.font = '10px Arial, sans-serif';
            context.textAlign = 'right';
            context.textBaseline = 'middle';
            for (let tick = 0; tick <= 4; tick += 1) {
              const y = margin.top + (plotHeight * tick) / 4;
              const value = max - ((max - min) * tick) / 4;
              context.strokeStyle = '#e3e8ef';
              context.lineWidth = 1;
              context.beginPath();
              context.moveTo(margin.left, y);
              context.lineTo(width - margin.right, y);
              context.stroke();
              context.fillStyle = '#6b7280';
              context.fillText(seriesIndex === 3 ? `${Math.round(value)}` : value.toFixed(1), margin.left - 7, y);
            }

            context.strokeStyle = color;
            context.lineWidth = 2;
            context.lineJoin = 'round';
            context.lineCap = 'round';
            context.beginPath();
            values.forEach((point, index) => {
              const x = margin.left + (values.length === 1 ? plotWidth / 2 : (plotWidth * index) / (values.length - 1));
              const y = margin.top + ((max - point.value) / (max - min)) * plotHeight;
              if (index === 0) context.moveTo(x, y);
              else context.lineTo(x, y);
            });
            context.stroke();

            context.fillStyle = color;
            values.forEach((point, index) => {
              if (values.length > 36 && index % Math.ceil(values.length / 24) !== 0 && index !== values.length - 1) return;
              const x = margin.left + (values.length === 1 ? plotWidth / 2 : (plotWidth * index) / (values.length - 1));
              const y = margin.top + ((max - point.value) / (max - min)) * plotHeight;
              context.beginPath();
              context.arc(x, y, 2.4, 0, Math.PI * 2);
              context.fill();
            });

            context.fillStyle = '#6b7280';
            context.textAlign = 'center';
            context.textBaseline = 'top';
            [0, 1, 2, 3].forEach((tick) => {
              const index = Math.round(((values.length - 1) * tick) / 3);
              const x = margin.left + (plotWidth * tick) / 3;
              context.fillText(formatHistoryAxisTime(values[index].timestamp), x, height - margin.bottom + 9);
            });
            canvas.setAttribute('aria-label', `${values.length} pontos, unidade ${unit}`);
          }

          function renderHistoryCharts() {
            const message = document.getElementById('historyMessage');
            const english = state.language === 'en';
            if (!state.history) {
              message.textContent = english ? 'History could not be loaded.' : 'Não foi possível carregar o histórico.';
            } else if (state.history.available === false) {
              message.textContent = english ? 'Historical storage is unavailable.' : 'Armazenamento do histórico indisponível.';
            } else if (state.history.timeValid === false) {
              message.textContent = english ? 'Waiting for clock synchronization before recording samples.' : 'Aguardando sincronização do relógio para registrar amostras.';
            } else if (!state.history.points.length) {
              message.textContent = english ? 'At most one sample is stored per minute. No data is available for this period yet.' : 'É armazenada no máximo uma amostra por minuto. Ainda não há dados neste período.';
            } else {
              message.textContent = english
                ? `${state.history.points.length} aggregated points · at most one sample per minute.`
                : `${state.history.points.length} pontos agregados · no máximo uma amostra por minuto.`;
            }
            drawHistoryChart('temperatureChart', 1, '#c44f36', '°C');
            drawHistoryChart('humidityChart', 2, '#2878ad', '%');
            drawHistoryChart('gasChart', 3, '#16784b', 'ADC');
          }

          function renderOverview() {
            const status = state.status || {};
            const cfg = state.config || {};
            const isAdmin = state.role === 'admin';
            const isVisitor = state.role === 'visitor';
            const canSeeTemp = isAdmin || (isVisitor ? !!cfg.visitante_pode_ver_temperatura : !!cfg.usuario_pode_ver_temperatura);
            const canSeeUmid = isAdmin || (isVisitor ? !!cfg.visitante_pode_ver_umidade : !!cfg.usuario_pode_ver_umidade);
            const canSeeGas = isAdmin || (isVisitor ? !!cfg.visitante_pode_ver_gas : !!cfg.usuario_pode_ver_gas);
            const canSeeWifi = isAdmin || (isVisitor ? !!cfg.visitante_pode_ver_wifi : !!cfg.usuario_pode_ver_wifi);
            const canSeeNetwork = isAdmin || (isVisitor ? !!cfg.visitante_pode_ver_rede : !!cfg.usuario_pode_ver_rede);
            const canSeeLogs = isAdmin || (isVisitor ? !!cfg.visitante_pode_ver_logs : !!cfg.usuario_pode_ver_logs);
            const canSilence = isAdmin || (isVisitor ? !!cfg.visitante_pode_silenciar_alarme : !!cfg.usuario_pode_silenciar_alarme);

            document.querySelector('[data-visible-for="temp"]').style.display = canSeeTemp ? 'block' : 'none';
            document.querySelector('[data-visible-for="umid"]').style.display = canSeeUmid ? 'block' : 'none';
            document.querySelector('[data-visible-for="gas"]').style.display = canSeeGas ? 'block' : 'none';
            document.querySelector('[data-history-series="temperature"]').style.display = canSeeTemp ? '' : 'none';
            document.querySelector('[data-history-series="humidity"]').style.display = canSeeUmid ? '' : 'none';
            document.querySelector('[data-history-series="gas"]').style.display = canSeeGas ? '' : 'none';
            document.getElementById('wifiState').style.display = canSeeWifi ? 'inline-flex' : 'none';
            document.getElementById('networkCard').style.display = canSeeNetwork ? 'block' : 'none';

            document.getElementById('tempValue').textContent = canSeeTemp ? `${Number(status.temperatura || 0).toFixed(1)} °C` : 'Sem acesso';
            document.getElementById('humidityValue').textContent = canSeeUmid ? `${Number(status.umidade || 0).toFixed(1)} %` : 'Sem acesso';
            document.getElementById('gasValue').textContent = canSeeGas ? `${Number(status.gas_adc || 0)} ADC` : 'Sem acesso';
            document.getElementById('tempValue').className = `value ${canSeeTemp && status.alerta_temperatura ? 'alert' : 'ok'}`;
            document.getElementById('humidityValue').className = `value ${canSeeUmid && status.alerta_umidade ? 'alert' : 'ok'}`;
            document.getElementById('gasValue').className = `value ${canSeeGas && status.alerta_gas ? 'alert' : 'ok'}`;

            const alerting = !!(status.alerta_temperatura || status.alerta_umidade || status.alerta_gas);
            const alertEl = document.getElementById('alertState');
            alertEl.textContent = alerting ? 'Alerta ativo' : 'Sem alarmes';
            alertEl.className = alerting ? 'pill alert blinking' : 'pill warn';
            document.getElementById('silenceAlarmBtn').style.display = alerting && canSilence && !status.buzzer_silenciado ? 'inline-block' : 'none';

            const wifiEl = document.getElementById('wifiState');
            wifiEl.textContent = canSeeWifi ? 'Wi‑Fi: conectado' : 'Wi‑Fi: oculto';
            wifiEl.className = 'pill';
            document.getElementById('networkName').textContent = status.rede_nome || 'Sem rede conectada';
            document.getElementById('connectedNetworkName').textContent = status.rede_nome || 'Nenhuma rede conectada';
            document.getElementById('networkAccessIp').textContent = `IP de acesso: ${status.ip_acesso || 'indisponível'}`;
            document.getElementById('connectedNetworkIp').textContent = status.ip_acesso || 'Indisponível';
            document.getElementById('networkAccessHost').textContent = `Endereço: ${status.endereco_acesso || 'indisponível'}`;
            document.getElementById('connectedNetworkHost').textContent = status.endereco_acesso || 'Endereço local indisponível';
            const securityLabels = {
              OPEN: 'Aberta',
              'WPA-PSK': 'WPA/WPA2 pessoal',
              'WPA2-PSK': 'WPA/WPA2 pessoal',
              'WPA2-ENTERPRISE': 'WPA/WPA2 Enterprise',
              'WPA-ENTERPRISE': 'WPA/WPA2 Enterprise',
              'PONTO DE ACESSO': 'Ponto de acesso',
              DESCONECTADO: 'Desconectado'
            };
            const configuredSecurity = state.config.wifi && state.config.wifi.security;
            const securityLabel = securityLabels[status.tipo_conexao] || securityLabels[configuredSecurity] || status.tipo_conexao || configuredSecurity || 'indisponível';
            const methodLabel = status.metodo_enterprise ? ` · Método: ${status.metodo_enterprise}` : '';
            document.getElementById('networkConnectionType').textContent = `Tipo de conexão: ${securityLabel}${methodLabel}`;
            document.getElementById('connectedNetworkType').textContent = securityLabel;
            document.getElementById('connectedNetworkInternet').textContent = status.internet_ok ? 'Internet: OK' : 'Internet: indisponível';
            document.getElementById('connectedNetworkInternet').style.color = status.internet_ok ? 'var(--green)' : 'var(--red)';
            document.getElementById('deleteNetworkBtn').disabled = !((state.config.wifi && state.config.wifi.ssid) || status.rede_nome);
            document.getElementById('headerIp').textContent = `IP: ${status.ip_acesso || 'indisponível'}`;
            document.getElementById('internetStatus').textContent = status.internet_ok ? 'Internet: OK' : 'Internet: indisponível';
            document.getElementById('internetStatus').style.color = status.internet_ok ? 'var(--green)' : 'var(--red)';

            const list = document.getElementById('overviewLogList');
            if (!canSeeLogs) {
              list.textContent = 'Acesso a eventos bloqueado.';
            } else {
              const logs = state.logs.slice(0, 5);
              if (!logs.length) {
                list.textContent = 'Nenhum evento registrado.';
              } else {
                list.innerHTML = logs.map((log) => `<div>${formatEventTime(log.timestamp)} — ${state.language === 'en' ? translateEvent(log.evento) : (log.evento || 'Evento sem nome')}</div>`).join('');
              }
            }

            const name = cfg.nome_aparelho || '';
            document.getElementById('deviceNameTitle').textContent = name;
            document.getElementById('dateTimeDisplay').textContent = `Data e hora: ${new Date().toLocaleString('pt-BR')}`;
            applyLanguage();
            const externalPower = !!status.alimentacao_externa;
            const powerStatus = document.getElementById('powerStatus');
            const powerDescription = state.language === 'en'
              ? (externalPower ? 'External power connected' : 'Running on battery')
              : (externalPower ? 'Alimentação externa conectada' : 'Alimentado pela bateria');
            powerStatus.classList.toggle('power-status--external', externalPower);
            powerStatus.classList.toggle('power-status--battery', !externalPower);
            const batteryPresent = !!status.bateria_presente;
            const batteryPercent = Number(status.bateria_percentual || 0);
            const batteryVoltage = Number(status.bateria_tensao || 0);
            const batteryPercentText = batteryPresent ? `${batteryPercent}%` : '--%';
            const fillHeight = batteryPresent ? Math.max(1, 14 * batteryPercent / 100) : 0;
            document.getElementById('batteryLevelFill').setAttribute('height', fillHeight.toFixed(1));
            document.getElementById('batteryLevelFill').setAttribute('y', (20 - fillHeight).toFixed(1));
            document.getElementById('batteryPercentLabel').textContent = batteryPercentText;
            const batteryDetails = batteryPresent
              ? (state.language === 'en' ? `${batteryPercent}% battery, ${batteryVoltage.toFixed(2)} V` : `Bateria ${batteryPercent}%, ${batteryVoltage.toFixed(2)} V`)
              : (state.language === 'en' ? 'Battery not detected' : 'Bateria não detectada');
            powerStatus.setAttribute('aria-label', `${powerDescription}. ${batteryDetails}`);
            powerStatus.title = `${powerDescription}. ${batteryDetails}`;
            document.getElementById('powerStatusLabel').textContent = state.language === 'en'
              ? (externalPower ? 'External power' : 'Battery power')
              : (externalPower ? 'Na tomada' : 'Na bateria');
          }

          function renderUsers() {
            const tbody = document.getElementById('usersTableBody');
            const passwordUser = document.getElementById('changePasswordUser');
            const managedUsers = state.users.filter((user) =>
              user.role !== 'visitor' && user.username?.trim().toLocaleLowerCase('pt-BR') !== 'visitante'
            );
            passwordUser.innerHTML = managedUsers.map((user) => `<option value="${user.username}">${user.username} (${user.role})</option>`).join('');
            if (!managedUsers.length) {
              tbody.innerHTML = '<tr><td colspan="3" class="muted">Nenhum usuário cadastrado.</td></tr>';
              return;
            }
            tbody.innerHTML = managedUsers.map((user) => `
              <tr>
                <td>${user.username}</td>
                <td>${user.role}</td>
                <td>
                  <button class="danger delete-user" data-user="${user.username}" type="button">Excluir</button>
                </td>
              </tr>
            `).join('');

            document.querySelectorAll('.delete-user').forEach((btn) => {
              btn.addEventListener('click', async () => {
                await deleteUser(btn.dataset.user);
              });
            });
          }

          function renderLogs() {
            const container = document.getElementById('logsContainer');
            if (!state.logs.length) {
              container.textContent = 'Nenhum evento registrado.';
              return;
            }
            container.innerHTML = state.logs.map((log) => `
              <div style="border-bottom:1px solid var(--line); padding:10px 0;">
                <strong>${formatEventTime(log.timestamp)}</strong><br>
                <span>${state.language === 'en' ? translateEvent(log.evento) : log.evento}</span>
              </div>
            `).join('');
            applyLanguage();
          }

          function renderPermissions(resetTab = true) {
            const tabs = {
              overview: true,
              network: state.role === 'admin',
              sensors: state.role === 'admin',
              users: state.role === 'admin',
              maintenance: state.role === 'admin',
              logs: state.role === 'admin',
              about: true
            };

            document.querySelectorAll('.nav-btn').forEach((btn) => {
              const show = tabs[btn.dataset.tab] !== false;
              btn.classList.toggle('hidden', !show);
            });

            const visitorBtn = document.getElementById('visitorBtn');
            if (visitorBtn) {
              const allowVisitor = !!state.config.permitir_visitante;
              visitorBtn.style.display = allowVisitor ? 'inline-block' : 'none';
            }

            if (resetTab) {
              setTab('overview');
            }
          }

          function renderAuth() {
            const logged = !!state.token;
            document.getElementById('loginView').classList.toggle('hidden', logged);
            document.getElementById('appView').classList.toggle('hidden', !logged);
            document.getElementById('userBadge').textContent = state.user ? `${state.user} · ${state.role}` : 'Usuário';
          }

          async function hydrateSession() {
            if (!state.token) {
              try {
                await loadConfig();
              } catch (error) {}
              renderAuth();
              renderPermissions();
              return;
            }
            try {
              const data = await request('/api/session');
              state.user = data.username;
              state.role = data.role;
              renderAuth();
              renderPermissions();
              await loadDashboard();
            } catch (error) {
              state.token = '';
              localStorage.removeItem('labToken');
              try {
                await loadConfig();
              } catch (loadError) {}
              renderAuth();
              renderPermissions();
            }
          }

          async function loadDashboard() {
            const tasks = [loadStatus(), loadUsers(), loadLogs()];
            if (['admin', 'user', 'visitor'].includes(state.role)) tasks.push(loadConfig());
            if (['admin', 'user', 'visitor'].includes(state.role)) tasks.push(loadHistory());
            await Promise.all(tasks);
            if (state.role === 'admin') await loadDashboardLink();
            renderOverview();
            renderUsers();
            renderLogs();
            renderPermissions();
          }

          async function loadStatus() {
            const data = await request('/api/status');
            state.status = data;
          }

          async function loadHistory() {
            const message = document.getElementById('historyMessage');
            const requestedDays = state.historyDays;
            try {
              const history = await request(`/api/history?days=${requestedDays}`);
              if (requestedDays !== state.historyDays) return;
              state.history = history;
              state.historyLoadedAt = Date.now();
              renderHistoryCharts();
            } catch (error) {
              if (requestedDays !== state.historyDays) return;
              state.history = null;
              message.textContent = error.message || 'Não foi possível carregar o histórico.';
              renderHistoryCharts();
            }
          }

          async function loadConfig() {
            const data = await request('/api/config');
            state.config = data;
            document.getElementById('nomeAparelho').value = data.nome_aparelho || '';
            document.getElementById('tempMin').value = data.temperatura_minima ?? 15;
            document.getElementById('tempMax').value = data.temperatura_maxima ?? 35;
            document.getElementById('umidMin').value = data.umidade_minima ?? 30;
            document.getElementById('umidMax').value = data.umidade_maxima ?? 70;
            document.getElementById('gasCritico').value = data.gas_adc_critico ?? 3800;
            document.getElementById('intervaloLeitura').value = data.intervalo_leitura ?? 1;
            document.getElementById('alarmeTemp').checked = !!data.alarme_temperatura;
            document.getElementById('alarmeUmid').checked = !!data.alarme_umidade;
            document.getElementById('alarmeGas').checked = !!data.alarme_gas;
            document.getElementById('ledAlerta').checked = !!data.led_alerta;
            document.getElementById('buzzer').checked = !!data.buzzer;
            document.getElementById('permVeTemp').checked = !!data.usuario_pode_ver_temperatura;
            document.getElementById('permVeUmid').checked = !!data.usuario_pode_ver_umidade;
            document.getElementById('permVeGas').checked = !!data.usuario_pode_ver_gas;
            document.getElementById('permVeWifi').checked = !!data.usuario_pode_ver_wifi;
            document.getElementById('permVeRede').checked = !!data.usuario_pode_ver_rede;
            document.getElementById('permVeLogs').checked = !!data.usuario_pode_ver_logs;
            document.getElementById('permSilenciar').checked = !!data.usuario_pode_silenciar_alarme;
            document.getElementById('visitanteVeTemp').checked = !!data.visitante_pode_ver_temperatura;
            document.getElementById('visitanteVeUmid').checked = !!data.visitante_pode_ver_umidade;
            document.getElementById('visitanteVeGas').checked = !!data.visitante_pode_ver_gas;
            document.getElementById('visitanteVeWifi').checked = !!data.visitante_pode_ver_wifi;
            document.getElementById('visitanteVeRede').checked = !!data.visitante_pode_ver_rede;
            document.getElementById('visitanteVeLogs').checked = !!data.visitante_pode_ver_logs;
            document.getElementById('visitanteSilenciar').checked = !!data.visitante_pode_silenciar_alarme;
            document.getElementById('permVisitante').checked = !!data.permitir_visitante;
            document.getElementById('manualDateTime').value = data.data_hora_manual || '';
            document.getElementById('networkSsid').value = data.wifi && data.wifi.ssid ? data.wifi.ssid : '';
            document.getElementById('networkSecurity').value = data.wifi && data.wifi.security ? data.wifi.security : 'OPEN';
            document.getElementById('enterpriseIdentity').value = data.wifi && data.wifi.enterprise_identity ? data.wifi.enterprise_identity : '';
            document.getElementById('enterpriseUsername').value = data.wifi && data.wifi.enterprise_username ? data.wifi.enterprise_username : '';
            document.getElementById('enterpriseMethod').value = data.wifi && data.wifi.enterprise_method ? data.wifi.enterprise_method : 'PEAP';
            document.getElementById('enterpriseTtlsPhase2').value = data.wifi && data.wifi.enterprise_ttls_phase2 ? data.wifi.enterprise_ttls_phase2 : 'MSCHAPV2';
            document.getElementById('networkSecurity').disabled = false;
            document.getElementById('securityDetectionHint').textContent = 'Selecione uma rede no scan para detectar automaticamente.';
            updateNetworkSecurityFields();
          }

          async function saveConfig(messageId = 'configMessage') {
            const currentDeviceName = String((state.config && state.config.nome_aparelho) || '').trim();
            const nextDeviceName = String(document.getElementById('nomeAparelho').value.trim() || '');
            const ledAlerta = document.getElementById('ledAlerta');
            const buzzer = document.getElementById('buzzer');
            const payload = {
              nome_aparelho: nextDeviceName,
              temperatura_minima: Number(document.getElementById('tempMin').value),
              temperatura_maxima: Number(document.getElementById('tempMax').value),
              umidade_minima: Number(document.getElementById('umidMin').value),
              umidade_maxima: Number(document.getElementById('umidMax').value),
              gas_adc_critico: Number(document.getElementById('gasCritico').value),
              alarme_temperatura: document.getElementById('alarmeTemp').checked,
              alarme_umidade: document.getElementById('alarmeUmid').checked,
              alarme_gas: document.getElementById('alarmeGas').checked,
              led_alerta: ledAlerta ? ledAlerta.checked : !!(state.config && state.config.led_alerta),
              buzzer: buzzer ? buzzer.checked : !!(state.config && state.config.buzzer),
              intervalo_leitura: Number(document.getElementById('intervaloLeitura').value),
              usuario_pode_ver_temperatura: document.getElementById('permVeTemp').checked,
              usuario_pode_ver_umidade: document.getElementById('permVeUmid').checked,
              usuario_pode_ver_gas: document.getElementById('permVeGas').checked,
              usuario_pode_ver_wifi: document.getElementById('permVeWifi').checked,
              usuario_pode_ver_rede: document.getElementById('permVeRede').checked,
              usuario_pode_ver_logs: document.getElementById('permVeLogs').checked,
              usuario_pode_silenciar_alarme: document.getElementById('permSilenciar').checked,
              visitante_pode_ver_temperatura: document.getElementById('visitanteVeTemp').checked,
              visitante_pode_ver_umidade: document.getElementById('visitanteVeUmid').checked,
              visitante_pode_ver_gas: document.getElementById('visitanteVeGas').checked,
              visitante_pode_ver_wifi: document.getElementById('visitanteVeWifi').checked,
              visitante_pode_ver_rede: document.getElementById('visitanteVeRede').checked,
              visitante_pode_ver_logs: document.getElementById('visitanteVeLogs').checked,
              visitante_pode_silenciar_alarme: document.getElementById('visitanteSilenciar').checked,
              permitir_visitante: document.getElementById('permVisitante').checked,
              data_hora_manual: document.getElementById('manualDateTime').value
            };
            try {
              const result = await request('/api/config', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(payload) });

              if (result.restart_required) {
                const redirectUrl = result.redirect_url || buildDeviceRedirectUrl(nextDeviceName);
                showNetworkRestartOverlay(redirectUrl, `Reiniciando, você será direcionado automaticamente para ${redirectUrl.replace(/^https?:\/\//i, '').replace(/\/$/, '')}`);
                return;
              }

              await loadConfig();
              await loadStatus();
              renderOverview();
              renderPermissions(false);
              document.getElementById(messageId).textContent = result.message || 'Configurações salvas.';
            } catch (error) {
              const message = error.message || 'Não foi possível salvar as configurações.';
              document.getElementById(messageId).textContent = message;
              document.getElementById(messageId).style.color = 'var(--red)';
            }
          }

          async function saveVisitorAccess() {
            try {
              const permitirVisitante = document.getElementById('permVisitante').checked;
              const result = await request('/api/visitor-access', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ permitir_visitante: permitirVisitante })
              });
              if (result.permitir_visitante !== permitirVisitante) {
                throw new Error('O dispositivo não confirmou a configuração solicitada.');
              }
              await loadConfig();
              renderPermissions(false);
              document.getElementById('visitorAccessMessage').textContent = result.message;
            } catch (error) {
              document.getElementById('visitorAccessMessage').textContent = error.message || 'Não foi possível salvar o acesso visitante.';
            }
          }

          async function loadUsers() {
            if (state.role !== 'admin') {
              state.users = [];
              return;
            }
            const data = await request('/api/users');
            state.users = data.users || [];
          }

          async function createUser() {
            const username = document.getElementById('newUserName').value.trim();
            const password = document.getElementById('newUserPassword').value;
            const passwordConfirm = document.getElementById('newUserPasswordConfirm').value;
            const role = document.getElementById('newUserRole').value;
            if (!username || !password) {
              showMessage('Informe usuário e senha.');
              return;
            }
            if (password !== passwordConfirm) {
              showMessage('As senhas não conferem.');
              return;
            }
            await request('/api/users', {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify({ username, password, password_confirm: passwordConfirm, role })
            });
            document.getElementById('newUserName').value = '';
            document.getElementById('newUserPassword').value = '';
            document.getElementById('newUserPasswordConfirm').value = '';
            await loadUsers();
            renderUsers();
          }

          async function deleteUser(username) {
            if (!username) return;
            await request('/api/users', {
              method: 'DELETE',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify({ username })
            });
            await loadUsers();
            renderUsers();
          }

          async function changeUserPassword() {
            const username = document.getElementById('changePasswordUser').value;
            const password = document.getElementById('changePasswordValue').value;
            const confirmation = document.getElementById('changePasswordConfirm').value;
            const message = document.getElementById('changePasswordMessage');
            if (!username || !password) {
              message.textContent = 'Selecione um usuário e informe a nova senha.';
              return;
            }
            if (password !== confirmation) {
              message.textContent = 'As senhas não conferem.';
              return;
            }
            try {
              const result = await request('/api/users/password', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ username, password, password_confirm: confirmation })
              });
              document.getElementById('changePasswordValue').value = '';
              document.getElementById('changePasswordConfirm').value = '';
              message.textContent = result.message || 'Senha alterada com sucesso.';
            } catch (error) {
              message.textContent = error.message || 'Não foi possível alterar a senha.';
            }
          }

          async function loadLogs() {
            if (!['admin', 'user', 'visitor'].includes(state.role)) {
              state.logs = [];
              return;
            }
            const data = await request('/api/logs');
            state.logs = data.logs || [];
          }

          async function login(username, password) {
            const data = await request('/api/login', {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify({ username, password })
            });
            state.token = data.token || '';
            state.user = data.username || username;
            state.role = data.role || 'user';
            localStorage.setItem('labToken', state.token);
            renderAuth();
            renderPermissions();
            await loadDashboard();
          }

          document.getElementById('loginBtn').addEventListener('click', async () => {
            const user = document.getElementById('loginUser').value.trim();
            const password = document.getElementById('loginPassword').value;
            try {
              await login(user, password);
              showMessage('Login realizado com sucesso.', false);
            } catch (error) {
              showMessage(error.message || 'Credenciais inválidas.');
            }
          });

          document.getElementById('visitorBtn').addEventListener('click', async () => {
            try {
              const data = await request('/api/visitor', { method: 'POST' });
              state.token = data.token || '';
              state.user = 'Visitante';
              state.role = 'visitor';
              localStorage.setItem('labToken', state.token);
              renderAuth();
              renderPermissions();
              await loadDashboard();
            } catch (error) {
              showMessage(error.message || 'O acesso de visitante está desativado.');
            }
          });

          document.getElementById('logoutBtn').addEventListener('click', () => {
            state.token = '';
            state.user = null;
            state.role = 'visitor';
            localStorage.removeItem('labToken');
            renderAuth();
            renderPermissions();
          });

          document.getElementById('saveConfigBtn').addEventListener('click', saveConfig);
          document.getElementById('saveDeviceBtn').addEventListener('click', () => saveConfig('deviceConfigMessage'));
          document.getElementById('createUserBtn').addEventListener('click', createUser);
          document.getElementById('changePasswordBtn').addEventListener('click', changeUserPassword);
          document.getElementById('saveVisitorAccessBtn').addEventListener('click', saveVisitorAccess);
          document.getElementById('networkSecurity').addEventListener('change', updateNetworkSecurityFields);
          document.getElementById('enterpriseUsername').addEventListener('input', updateEnterpriseMethodFields);
          document.getElementById('enterpriseMethod').addEventListener('change', updateEnterpriseMethodFields);
          document.getElementById('scanNetworksBtn').addEventListener('click', scanNetworks);
          document.getElementById('saveNetworkBtn').addEventListener('click', saveNetwork);
          document.getElementById('deleteNetworkBtn').addEventListener('click', deleteSavedNetwork);
          document.getElementById('dashboardLinkBtn').addEventListener('click', linkDashboard);
          document.getElementById('dashboardUnlinkBtn').addEventListener('click', unlinkDashboard);
          document.getElementById('dashboardServerUrl').addEventListener('input', () => {
            dashboardServerUrlEdited = true;
            updateDashboardTransportChoice();
          });
          document.getElementById('saveUserPermBtn').addEventListener('click', saveConfig);
          document.getElementById('backupConfigBtn').addEventListener('click', () => {
            request('/api/backup').then((backup) => {
              const blob = new Blob([JSON.stringify(backup, null, 2)], { type: 'application/json' });
              const a = document.createElement('a');
              a.href = URL.createObjectURL(blob);
              a.download = 'backup-monitor-lab.json';
              a.click();
              URL.revokeObjectURL(a.href);
              document.getElementById('maintenanceMessage').textContent = 'Backup criado com sucesso.';
            }).catch((error) => {
              document.getElementById('maintenanceMessage').textContent = error.message || 'Não foi possível criar o backup.';
            });
          });
          document.getElementById('restoreConfigBtn').addEventListener('click', async () => {
            const input = document.getElementById('restoreConfigInput');
            const message = document.getElementById('maintenanceMessage');
            if (!input.files.length) {
              message.textContent = 'Selecione um arquivo de backup primeiro.';
              return;
            }
            try {
              const backup = await input.files[0].text();
              const started = await request('/api/backup/restore', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: backup
              });
              message.textContent = started.message || 'Restaurando backup...';
              const result = await acompanharOperacao(started.operationId, 45000);
              message.textContent = result.message || 'Backup restaurado.';
              if (result.ok) await loadConfig();
            } catch (error) {
              message.textContent = error.message || 'Não foi possível restaurar o backup.';
            }
          });
          document.getElementById('factoryResetBtn').addEventListener('click', async () => {
            if (!window.confirm('Resetar o aparelho para os padrões de fábrica? A rede salva será apagada.')) return;
            try {
              const result = await request('/api/factory-reset', { method: 'POST' });
              document.getElementById('maintenanceMessage').textContent = result.message || 'Reset de fábrica iniciado.';
            } catch (error) {
              document.getElementById('maintenanceMessage').textContent = error.message || 'Não foi possível executar o reset.';
            }
          });
          document.getElementById('syncTimeBtn').addEventListener('click', async () => {
            try {
              let data = await request('/api/time');
              const deadline = Date.now() + 15000;
              while (data.pending && Date.now() < deadline) {
                document.getElementById('timeSourceMessage').textContent = 'Sincronizando o horário pela rede...';
                await new Promise((resolve) => setTimeout(resolve, 500));
                data = await request('/api/time');
              }
              if (data.pending) throw new Error('A sincronização do horário está demorando. Tente novamente.');
              if (!data.datetime) throw new Error('O servidor não retornou uma data válida.');
              document.getElementById('manualDateTime').value = data.datetime;
              document.getElementById('timeSourceMessage').textContent = `Data consultada em ${data.source}.`;
            } catch (error) {
              document.getElementById('timeSourceMessage').textContent = error.message || 'Não foi possível obter a hora da web.';
            }
          });
          document.getElementById('saveTimeBtn').addEventListener('click', async () => {
            const value = document.getElementById('manualDateTime').value;
            state.config.data_hora_manual = value;
            await request('/api/config', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({
              data_hora_manual: value
            }) });
            document.getElementById('timeSourceMessage').textContent = 'Data e hora salvas no aparelho.';
          });
          document.getElementById('silenceAlarmBtn').addEventListener('click', async () => {
            if (state.role === 'visitor') return;
            const novoEstado = !state.status.buzzer_silenciado;
            try {
              const result = await request('/api/status', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ buzzer_silenciado: novoEstado })
              });
              state.status.buzzer_silenciado = novoEstado;
              renderOverview();
              document.getElementById('networkMessage').textContent = result.message || 'Alarme sonoro desativado.';
            } catch (error) {
              document.getElementById('networkMessage').textContent = error.message || 'Não foi possível desativar o alarme sonoro.';
            }
          });
          document.getElementById('clearLogsBtn').addEventListener('click', async () => {
            try {
              await request('/api/logs/clear', { method: 'POST' });
              await loadLogs();
              renderLogs();
              renderOverview();
              document.getElementById('logsContainer').insertAdjacentHTML('afterbegin', '<div class="muted">Logs apagados com sucesso.</div>');
            } catch (error) {
              const container = document.getElementById('logsContainer');
              container.insertAdjacentHTML('afterbegin', `<div class="muted">${error.message || 'Não foi possível apagar os logs.'}</div>`);
            }
          });

          document.querySelectorAll('.nav-btn').forEach((btn) => {
            btn.addEventListener('click', () => setTab(btn.dataset.tab));
          });

          document.querySelectorAll('[data-history-days]').forEach((button) => {
            button.addEventListener('click', async () => {
              state.historyDays = Number(button.dataset.historyDays);
              document.querySelectorAll('[data-history-days]').forEach((filter) => {
                const selected = Number(filter.dataset.historyDays) === state.historyDays;
                filter.classList.toggle('active', selected);
                filter.setAttribute('aria-pressed', String(selected));
              });
              document.getElementById('historyMessage').textContent = state.language === 'en' ? 'Loading history...' : 'Carregando histórico...';
              await loadHistory();
            });
          });

          window.addEventListener('resize', () => requestAnimationFrame(renderHistoryCharts));

          document.querySelectorAll('.submenu-btn').forEach((btn) => {
            btn.addEventListener('click', () => setSubmenu(btn.dataset.submenuGroup, btn.dataset.submenu));
          });

          document.getElementById('languageSelect').addEventListener('change', (event) => {
            state.language = event.target.value;
            localStorage.setItem('labLanguage', state.language);
            applyLanguage();
            renderOverview();
            renderHistoryCharts();
            renderUsers();
            renderLogs();
          });

          document.getElementById('loginLanguageSelect').addEventListener('change', (event) => {
            state.language = event.target.value;
            localStorage.setItem('labLanguage', state.language);
            applyLanguage();
          });

          async function pollStatus() {
            try {
              if (!state.token) return;
              await loadStatus();
              renderOverview();
              if (!document.getElementById('tab-overview').classList.contains('hidden') &&
                  Date.now() - state.historyLoadedAt >= 60000) {
                await loadHistory();
              }
              if (state.role === 'admin' && !document.getElementById('tab-network').classList.contains('hidden')) {
                await loadDashboardLink();
              }
            } catch (error) {}
          }

          applyLanguage();
          hydrateSession();
          setInterval(pollStatus, 3000);
