const state = { lastLogs: "" };
async function request(path, options = {}) {
  const response = await fetch(path, { ...options, headers: { ...(options.headers || {}) } });
  const data = await response.json().catch(() => ({}));
  if (!response.ok) throw Error(data.message || `HTTP ${response.status}`);
  return data;
}
async function refreshStatus() {
  try {
    const data = await request('/api/status');
    document.querySelector('.value').textContent = `${Number(data.temperatura || 0).toFixed(1)} °C`;
    document.querySelectorAll('.value')[1].textContent = `${Number(data.umidade || 0).toFixed(1)} %`;
    document.querySelectorAll('.value')[2].textContent = `${data.gas_adc || 0} ADC`;
    const panel = document.querySelector('p strong');
    if (panel) panel.parentElement.textContent = `Status Wi‑Fi: ${data.wifi_ssid ? 'Conectado' : 'Modo de configuração AP'}`;
  } catch (error) {
    console.error(error);
  }
}
setInterval(refreshStatus, 2000);
refreshStatus();
