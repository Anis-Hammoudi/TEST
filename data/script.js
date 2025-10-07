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
