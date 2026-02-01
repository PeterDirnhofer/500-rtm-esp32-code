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
const evtSource = new EventSource(ESP_BASE + '/events');
evtSource.onmessage = function(e) {
  addLog('[RX] ' + e.data);
};
evtSource.onerror = function(e) {
  addLog('[SSE] connection error');
};

sendBtn.addEventListener('click', () => {
  const cmd = cmdInput.value.trim();
  if (!cmd) return;
  fetch(ESP_BASE + '/send', {method: 'POST', body: cmd})
      .then(r => r.text())
      .then(t => addLog('[TX] ' + cmd))
      .catch(err => addLog('[TX] error: ' + err));
});

addLog('Client initialized.');
