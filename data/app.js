const relayContainer = document.querySelector("#relays");
const connection = document.querySelector("#connection");
const message = document.querySelector("#message");

async function request(url, options = {}) {
  const response = await fetch(url, options);
  if (!response.ok) {
    throw new Error(`HTTP ${response.status}`);
  }
  return response.json();
}

function render(data) {
  connection.textContent = data.wifiConnected
    ? "Wi-Fi подключён"
    : data.setupMode
      ? "Режим настройки Wi-Fi"
      : "Wi-Fi не подключён";

  relayContainer.replaceChildren();

  for (const relay of data.relays) {
    const card = document.createElement("article");
    card.className = "relay-card";

    const title = document.createElement("h3");
    title.textContent = relay.name || `Реле ${relay.index + 1}`;

    const comment = document.createElement("p");
    comment.className = "relay-comment";
    comment.textContent = relay.comment || "";

    const state = document.createElement("p");
    state.className = "relay-state";
    state.textContent = relay.on ? "Включено" : "Выключено";

    const button = document.createElement("button");
    button.textContent = relay.on ? "Выключить" : "Включить";
    button.disabled = false;

    button.addEventListener("click", async () => {
      button.disabled = true;
      message.textContent = "";

      try {
        await request("/api/relay", {
          method: "POST",
          headers: {
            "Content-Type": "application/x-www-form-urlencoded"
          },
          body: new URLSearchParams({
            index: String(relay.index),
            action: relay.on ? "off" : "on"
          })
        });

        await refresh();
      } catch (error) {
        message.textContent = `Не удалось управлять реле: ${error.message}`;
      } finally {
        button.disabled = false;
      }
    });

    card.append(title, comment, state, button);
    relayContainer.append(card);
  }
}

async function refresh() {
  try {
    render(await request("/api/status"));
  } catch (error) {
    connection.textContent = "Нет связи с контроллером";
    message.textContent = error.message;
  }
}

refresh();
setInterval(refresh, 5000);
