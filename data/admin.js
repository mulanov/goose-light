const form = document.querySelector("#settings-form");
const message = document.querySelector("#message");

async function request(url, options = {}) {
  const response = await fetch(url, { cache: "no-store", ...options });
  let data = {};
  try { data = await response.json(); } catch (_) {}
  if (!response.ok) throw new Error(data.error || `HTTP ${response.status}`);
  return data;
}

async function loadSettings() {
  const c = await request("/api/admin/settings");
  form.elements.stationName.value = c.stationName || "GooseLights";
  form.elements.ssid0.value = c.ssid0 || "";
  form.elements.ssid1.value = c.ssid1 || "";
  form.elements.ntp0.value = c.ntp0 || "";
  form.elements.ntp1.value = c.ntp1 || "";
  form.elements.timezone.value = c.timezone || "";
  form.elements.password0.value = "";
  form.elements.password1.value = "";
  form.elements.adminPassword.value = "";
  message.textContent = `Настройки загружены. Пароль Wi-Fi задан: профиль 1 — ${c.passwordSet0 ? "да" : "нет"}; профиль 2 — ${c.passwordSet1 ? "да" : "нет"}.`;
}

for (const i of [0, 1]) {
  form.elements[`clearPassword${i}`].addEventListener("change", (e) => {
    if (e.target.checked) form.elements[`password${i}`].value = "";
  });
  form.elements[`password${i}`].addEventListener("input", (e) => {
    if (e.target.value) form.elements[`clearPassword${i}`].checked = false;
  });
}

form.addEventListener("submit", async (event) => {
  event.preventDefault();
  const body = new URLSearchParams();
  for (const key of ["stationName", "ssid0", "password0", "ntp0", "ntp1", "timezone", "ssid1", "password1", "adminPassword"]) {
    body.set(key, form.elements[key].value);
  }
  body.set("clearPassword0", form.elements.clearPassword0.checked ? "1" : "0");
  body.set("clearPassword1", form.elements.clearPassword1.checked ? "1" : "0");
  try {
    const result = await request("/api/admin/settings", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body
    });
    message.textContent = result.wifiChanged
      ? "Настройки сохранены. Переподключение к Wi-Fi начнётся через 1,5 секунды; адрес устройства может измениться."
      : "Настройки сохранены. Настройки времени применяются автоматически.";
    form.elements.adminPassword.value = "";
    form.elements.password0.value = "";
    form.elements.password1.value = "";
    if (result.wifiChanged) message.textContent += " Найдите новый IP в Serial Monitor или в списке DHCP-клиентов роутера.";
  } catch (error) {
    message.textContent = `Ошибка сохранения: ${error.message}`;
  }
});

loadSettings().catch((error) => {
  message.textContent = `Не удалось загрузить настройки: ${error.message}`;
});
