const form = document.querySelector("#relay-form");
const countInput = document.querySelector("#relay-count");
const settings = document.querySelector("#relay-settings");
const message = document.querySelector("#message");
let relayData = [];

async function request(url, options = {}) {
  const response = await fetch(url, options);
  let data = {};
  try { data = await response.json(); } catch (_) {}
  if (!response.ok) throw new Error(data.error || `HTTP ${response.status}`);
  return data;
}

function renderRelays() {
  const count = Number(countInput.value);
  settings.replaceChildren();
  for (const relay of relayData) {
    const i = relay.index;
    const section = document.createElement("section");
    section.className = "admin-relay";
    section.hidden = i >= count;
    const heading = document.createElement("h3");
    heading.textContent = `Реле ${i + 1}`;
    const nameLabel = document.createElement("label");
    nameLabel.className = "field";
    nameLabel.append(document.createTextNode("Короткое имя (до 8 байт)"));
    const name = document.createElement("input");
    name.type = "text"; name.name = `name${i}`; name.maxLength = 8; name.required = true; name.value = relay.name || "";
    nameLabel.append(name);
    const commentLabel = document.createElement("label");
    commentLabel.className = "field";
    commentLabel.append(document.createTextNode("Комментарий"));
    const comment = document.createElement("textarea");
    comment.name = `comment${i}`; comment.maxLength = 95; comment.value = relay.comment || "";
    commentLabel.append(comment);
    const startupLabel = document.createElement("label");
    startupLabel.className = "checkbox-row";
    const startup = document.createElement("input");
    startup.type = "checkbox"; startup.name = `startup${i}`; startup.checked = Boolean(relay.startupOn);
    startupLabel.append(startup, document.createTextNode("Включать после перезагрузки"));
    section.append(heading, nameLabel, commentLabel, startupLabel);
    settings.append(section);
  }
}

countInput.addEventListener("input", renderRelays);
async function loadConfig() {
  const config = await request("/api/admin/relays");
  relayData = config.relays;
  countInput.value = config.relayCount;
  renderRelays();
  message.textContent = "Конфигурация реле загружена";
}

form.addEventListener("submit", async (event) => {
  event.preventDefault();
  const body = new URLSearchParams();
  body.set("relayCount", countInput.value);
  for (const relay of relayData) {
    const i = relay.index;
    const section = settings.children[i];
    if (!section || i >= Number(countInput.value)) continue;
    body.set(`name${i}`, section.querySelector(`[name="name${i}"]`).value);
    body.set(`comment${i}`, section.querySelector(`[name="comment${i}"]`).value);
    body.set(`startup${i}`, section.querySelector(`[name="startup${i}"]`).checked ? "1" : "0");
  }
  try {
    await request("/api/admin/relays", { method: "POST", headers: { "Content-Type": "application/x-www-form-urlencoded" }, body });
    message.textContent = "Настройки реле сохранены";
    await loadConfig();
  } catch (error) {
    message.textContent = `Ошибка сохранения: ${error.message}`;
  }
});

loadConfig().catch((error) => { message.textContent = `Не удалось загрузить настройки: ${error.message}`; });
