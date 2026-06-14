import requests
import matplotlib.pyplot as plt
from IPython.display import clear_output
import time

# =========================
# CONFIGURAÇÃO FIWARE
# =========================

BASE_URL = "http://SEU_IP_DA_VM:8666"

ENTITY_TYPE = "StepMonitor"
ENTITY_ID = "urn:ngsi-ld:StepMonitor:tenis001"

HEADERS = {
    "fiware-service": "careplus",
    "fiware-servicepath": "/tenis"
}

# =========================
# CONSULTA ATRIBUTO
# =========================

def obter_dados_atributo(atributo, lastN):

    url = (
        f"{BASE_URL}/STH/v1/contextEntities/"
        f"type/{ENTITY_TYPE}/id/{ENTITY_ID}"
        f"/attributes/{atributo}?lastN={lastN}"
    )

    try:

        response = requests.get(
            url,
            headers=HEADERS,
            timeout=10
        )

        if response.status_code == 200:

            data = response.json()

            return (
                data["contextResponses"][0]
                ["contextElement"]["attributes"][0]
                ["values"]
            )

        print(f"Erro {atributo}: {response.status_code}")
        return []

    except Exception as erro:

        print(f"Falha {atributo}: {erro}")
        return []

# =========================
# EXTRAIR SÉRIES
# =========================

def extrair_series(dados):

    valores = []
    tempos = []

    for item in dados:

        try:

            valores.append(float(item["attrValue"]))
            tempos.append(item["recvTime"])

        except:
            pass

    return tempos, valores

# =========================
# PLOT
# =========================

def plotar(tempos, valores, titulo, ylabel):

    if not valores:
        return

    media = sum(valores) / len(valores)

    plt.figure(figsize=(12,5))

    plt.plot(
        tempos,
        valores,
        marker="o"
    )

    plt.axhline(
        media,
        linestyle="--",
        label=f"Média: {media:.2f}"
    )

    plt.title(titulo)
    plt.xlabel("Tempo")
    plt.ylabel(ylabel)

    plt.xticks(rotation=45)

    plt.grid(True)
    plt.legend()

    plt.tight_layout()
    plt.show()

# =========================
# DASHBOARD
# =========================

def atualizar_dashboard(lastN):

    clear_output(wait=True)

    print("👟 DASHBOARD CAREPLUS")
    print("📡 ESP32 + FIWARE")
    print()

    dados_passos = obter_dados_atributo(
        "steps",
        lastN
    )

    dados_intensidade = obter_dados_atributo(
        "intensity",
        lastN
    )

    dados_bateria = obter_dados_atributo(
        "battery",
        lastN
    )

    tp, vp = extrair_series(dados_passos)
    ti, vi = extrair_series(dados_intensidade)
    tb, vb = extrair_series(dados_bateria)

    print("===== RESUMO =====\n")

    if vp:
        print(f"👣 Passos atuais: {int(vp[-1])}")
        print(f"👣 Média: {sum(vp)/len(vp):.1f}")

    if vi:
        print(f"🏃 Intensidade atual: {vi[-1]:.2f}")
        print(f"🏃 Média: {sum(vi)/len(vi):.2f}")

    if vb:
        print(f"🔋 Bateria atual: {vb[-1]:.0f} mV")
        print(f"🔋 Média: {sum(vb)/len(vb):.0f} mV")

    print()

    plotar(
        tp,
        vp,
        "Histórico de Passos",
        "Passos"
    )

    plotar(
        ti,
        vi,
        "Intensidade do Movimento",
        "Intensidade"
    )

    plotar(
        tb,
        vb,
        "Nível da Bateria",
        "mV"
    )

# =========================
# ENTRADAS
# =========================

lastN = int(
    input("Quantidade de registros (1-100): ")
)

intervalo = int(
    input("Atualizar a cada quantos segundos? ")
)

# =========================
# LOOP
# =========================

while True:

    atualizar_dashboard(lastN)

    time.sleep(intervalo)