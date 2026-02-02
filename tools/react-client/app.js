const log = document.getElementById('log');
const cmdInput = document.getElementById('cmd');
const sendBtn = document.getElementById('send');

function addLog(msg) {
  log.textContent += msg + '\n';
  log.scrollTop = log.scrollHeight;
}

// Base URL of the ESP when connected to its AP
const ESP_BASE = 'http://192.168.4.1';

// SSE connection to receive messages
const hostInput = document.getElementById('host');
const connectBtn = document.getElementById('connect');

let ws = null;

function connectWS(url) {
  if (ws) ws.close();
  addLog('Connecting to ' + url);
  try {
    ws = new WebSocket(url);
  } catch (err) {
    addLog('WebSocket error: ' + err);
    ws = null;
    return;
  }
  ws.onopen = () => addLog('[WS] open');
  ws.onmessage = (e) => addLog('[RX] ' + e.data);
  ws.onclose = () => addLog('[WS] closed');
  ws.onerror = (e) => addLog('[WS] error');
}

connectBtn.addEventListener('click', () => {
  const url = hostInput.value.trim() || 'ws://192.168.178.58:8765/';
  connectWS(url);
});

sendBtn.addEventListener('click', () => {
  const cmd = cmdInput.value.trim();
  if (!cmd) return;
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(cmd);
    addLog('[TX] ' + cmd);
  } else {
    addLog('[TX] WebSocket not open');
  }
});

addLog('Client initialized. Enter host and click Connect.');
