const form = document.querySelector("#config-form");
const countInput = document.querySelector("#relay-count");
const settings = document.querySelector("#relay-settings");
const message = document.querySelector("#message");

let relayData = [];

async function request(url, options = {}) {
  const response = await fetch(url, options);

  if (!response.ok) {
    throw new Error(`HTTP ${response.status}`);
  }

  return response.json();
}

function renderRelays() {
  const count = Number(countInput.value);
  settings.replaceChildren();

  for (const relay of relayData) {
    const index = relay.index;
    const section = document.createElement("section");
    section.className = "admin-relay";
    section.hidden = index >= count;

    const heading = document.createElement("h3");
    heading.textContent = `Реле ${index + 1}`;

    const nameLabel = document.createElement("label");
    nameLabel.className = "field";
    nameLabel.textContent = "Короткое имя (до 8 байт)";

    const name = document.createElement("input");
    name.type = "text";
    name.name = `name${index}`;
    name.maxLength = 8;
    name.required = true;
    name.value = relay.name || "";

    nameLabel.append(name);

    const commentLabel = document.createElement("label");
    commentLabel.className = "field";
    commentLabel.textContent = "Комментарий";

    const comment = document.createElement("textarea");
    comment.name = `comment${index}`;
    comment.maxLength = 95;
    comment.value = relay.comment || "";

    commentLabel.append(comment);

    const startupLabel = document.createElement("label");
    startupLabel.className = "checkbox-row";

    const startup = document.createElement("input");
    startup.type = "checkbox";
    startup.name = `startup${index}`;
    startup.checked = Boolean(relay.startupOn);

    startupLabel.append(startup, document.createTextNode(
      "Включать после перезагрузки"
    ));

    section.append(heading, nameLabel, commentLabel, startupLabel);
    settings.append(section);
  }
}

countInput.addEventListener("input", renderRelays);

async function loadConfig() {
  const config = await request("/api/admin/config");
  relayData = config.relays;
  countInput.value = config.relayCount;
  renderRelays();
  message.textContent = "Конфигурация загружена";
}

form.addEventListener("submit", async (event) => {
  event.preventDefault();

  const body = new URLSearchParams();
  body.set("relayCount", countInput.value);

  for (const relay of relayData) {
    const i = relay.index;
    const section = settings.querySelector(
      `.admin-relay:nth-child(${i + 1})`
    );

    if (!section) continue;

    body.set(`name${i}`, section.querySelector(`[name="name${i}"]`).value);
    body.set(
      `comment${i}`,
      section.querySelector(`[name="comment${i}"]`).value
    );
    body.set(
      `startup${i}`,
      section.querySelector(`[name="startup${i}"]`).checked ? "1" : "0"
    );
  }

  try {
    await request("/api/admin/config", {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded"
      },
      body
    });

    message.textContent = "Настройки сохранены";
  } catch (error) {
    message.textContent = `Ошибка сохранения: ${error.message}`;
  }
});

loadConfig().catch((error) => {
  message.textContent =
    `Не удалось загрузить настройки: ${error.message}`;
});
