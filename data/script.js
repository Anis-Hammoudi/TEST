<<<<<<< HEAD
function animateValue(id, newValue, unit) {
  const el = document.getElementById(id);
  if (!el) return;
  if (el.textContent !== newValue + ' ' + unit && !isNaN(newValue)) {
    el.style.color = '#43af4a';
    setTimeout(() => { el.style.color = '#2196f3'; }, 400);
  }
  el.textContent = isNaN(newValue) ? 'Erreur' : newValue + ' ' + unit;
}
function updateData() {
  fetch('/data').then(r => r.json()).then(data => {
    animateValue('temp', data.temperature, '°C');
    animateValue('hum', data.humidity, '%');
  });
}
setInterval(updateData, 2000);
window.onload = updateData;
=======
const statusTexts = {
  optimal: 'Conditions ideales',
  warning: 'A surveiller',
  alert: 'Alerte',
  unknown: 'Inconnu'
};

const buzzerTexts = {
  on: 'Active',
  off: 'Inactif'
};

function animateValue(id, value, suffix) {
  const el = document.getElementById(id);
  if (!el) return;

  if (value === null || value === undefined || Number.isNaN(Number(value))) {
    el.textContent = 'Erreur capteur';
    el.classList.add('value-error');
    return;
  }

  el.classList.remove('value-error');
  const formatted = `${Number(value).toFixed(1)}${suffix}`;
  if (el.textContent !== formatted) {
    el.classList.add('value-blink');
    setTimeout(() => el.classList.remove('value-blink'), 400);
  }
  el.textContent = formatted;
}

function applyStatusBadge(status) {
  const badge = document.getElementById('status');
  if (!badge) return;

  const normalized = status && statusTexts[status] ? status : 'unknown';
  badge.textContent = statusTexts[normalized];
  badge.className = `status-badge status-${normalized}`;
}

function applyBlueButtonState(isOn) {
  const btn = document.getElementById('blueButton');
  if (!btn) return;
  btn.dataset.state = isOn ? 'on' : 'off';
  btn.textContent = isOn ? 'Eteindre la LED bleue' : 'Allumer la LED bleue';
  btn.classList.toggle('btn-on', Boolean(isOn));
}

function applyBuzzerState(isOn) {
  const badge = document.getElementById('buzzer');
  if (!badge) return;
  const active = Boolean(isOn);
  badge.textContent = buzzerTexts[active ? 'on' : 'off'];
  badge.className = `status-badge ${active ? 'status-buzzer-on' : 'status-buzzer-off'}`;
}

async function sendBlueLedCommand(targetState) {
  const btn = document.getElementById('blueButton');
  if (!btn) return;
  try {
    btn.disabled = true;
    const body = `state=${encodeURIComponent(targetState)}`;
    const response = await fetch('/api/blue-led', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body
    });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const payload = await response.json();
    applyBlueButtonState(payload.blueLed);
  } catch (err) {
    console.error('Blue LED command failed', err);
    alert('Impossible de mettre a jour la LED bleue.');
  } finally {
    btn.disabled = false;
  }
}

async function updateData() {
  try {
    const response = await fetch('/data');
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const data = await response.json();

    animateValue('temp', data.temperature, '\u00B0C');
    animateValue('hum', data.humidity, '%');
    applyStatusBadge(data.status);
    applyBlueButtonState(Boolean(data.blueLed));
    applyBuzzerState(Boolean(data.buzzer));
  } catch (err) {
    console.error('Data refresh failed', err);
  }
}

function boot() {
  const btn = document.getElementById('blueButton');
  if (btn) {
    btn.addEventListener('click', () => {
      const target = btn.dataset.state === 'on' ? 'off' : 'on';
      sendBlueLedCommand(target);
    });
  }
  updateData();
  setInterval(updateData, 2000);
}

document.addEventListener('DOMContentLoaded', boot);
>>>>>>> mehdi
