import time
import csv
import requests
import uuid
from datetime import datetime, timezone
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# =====================================================================
# CONFIGURACIÓN DE RAINMAKER Y ENSAYO
# =====================================================================
RM_TOKEN = "eyJraWQiOiJxK0Y2Nkk1eE1nOWo2d2pQOWU1c05IeDF2VENVMDVqYjRJTm8vQjNvWXRVPSIsImFsZyI6IlJTMjU2In0.eyJzdWIiOiIwMmFlYWRmMS1mYmMyLTRhMWYtOWYxNi04NmZmZWM0MGFhMTQiLCJjb2duaXRvOmdyb3VwcyI6WyJ1cy1lYXN0LTFfTDl1NFMwTjFpX0dvb2dsZSJdLCJpc3MiOiJodHRwczovL2NvZ25pdG8taWRwLnVzLWVhc3QtMS5hbWF6b25hd3MuY29tL3VzLWVhc3QtMV9MOXU0UzBOMWkiLCJ2ZXJzaW9uIjoyLCJjbGllbnRfaWQiOiIxaDd1anFqczgxNDBuMTd2MGFoYjRuNTFtMiIsInRva2VuX3VzZSI6ImFjY2VzcyIsInNjb3BlIjoiYXdzLmNvZ25pdG8uc2lnbmluLnVzZXIuYWRtaW4gcGhvbmUgb3BlbmlkIHByb2ZpbGUgZW1haWwiLCJhdXRoX3RpbWUiOjE3OTI1NjAyNzYsImV4cCI6MTc5MjU2Mzg3NiwiaWF0IjoxNzkyNTYwMjc2LCJqdGkiOiJlNzg5ZTUzMC05MTY2LTRjOTctYjIxMC1mNmI0YjlkM2MxMTQiLCJ1c2VybmFtZSI6Ikdvb2dsZV8xMTQ2MDU2OTY2MDExNTY2MTA5OTYifQ.d3X2-J1dFtJYPakgkPp93ZyDPqfWklWJelUXGB2baPKduL5RpXdgTpfCUt-pMaWyuaicw0gh6OJELPayVqnvaRrcOJkJn_zLxZrmzTUG9D5pYGcJa171mAVvGtfYGOZKQbPcemw0eCWUQYr0ZwqJbWzrs60zIUstpOM7EVx8174m989zG-ClaSPFoTbMGXmxEd5pGYkrVX9haehnOam3vmy69Z106845xDZBzva_zDBJrgEJ0qr0zQUwoeCIavkWaFBYTpqtYjdhbUayW66sPRdlOLxL_2JZ6IyNzoU7uKiDGIy-rvaJAZbZSanzvzyemLf5ybS70aymlaumEOR4EA" 
NODE_ID = "4RPwDqhfkUghAHuFHpXYNq"
DEVICE_NAME = "Ventilador"       
PARAM_POWER = "Power"            
PARAM_CMD_ID = "cmd_id"          

API_URL = "https://api.rainmaker.espressif.com/v1/user/nodes/params"

TOTAL_ATTEMPTS = 150              # 75 ON y 75 OFF
TIMEOUT_S = 10                   
POLL_INTERVAL_S = 0.2           

CSV_FILENAME = "rtt_rainmaker_nube.csv"

# =====================================================================
# FUNCIONES DE MEDICIÓN
# =====================================================================

def realizar_ensayo():
    headers = {
        "Authorization": f"{RM_TOKEN}",
        "Content-Type": "application/json"
    }

    session_id = str(uuid.uuid4())[:8]
    on_confirmed, off_confirmed = 0, 0
    action_attempts = {"ON": 0, "OFF": 0}
    
    with open(CSV_FILENAME, mode='w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f, delimiter=',')
        writer.writerow([
            "session", "sample", "command_id", "clicked_at", "broker", 
            "action", "action_attempt", "result", "elapsed_ms", 
            "on_confirmed", "off_confirmed"
        ])

        print(f"Iniciando ensayo RainMaker Nube. Sesión: {session_id}")
        
        for sample in range(1, TOTAL_ATTEMPTS + 1):
            target_state = True if sample % 2 != 0 else False
            action_str = "ON" if target_state else "OFF"
            action_attempts[action_str] += 1
            current_cmd_id = sample 
            
            # Preparar payload (Formato estricto de la API REST de RainMaker)
            payload = {
                DEVICE_NAME: {
                    PARAM_POWER: target_state,
                    PARAM_CMD_ID: current_cmd_id
                }
            }

            clicked_at = datetime.now(timezone.utc).isoformat()
            t0 = time.perf_counter_ns()
            
            # Enviar comando REST (Fijate que agregamos params={"node_id": NODE_ID} igual que en el GET)
            try:
                params = {"node_id": NODE_ID}
                put_res = requests.put(API_URL, headers=headers, params=params, json=payload, timeout=5)
                put_res.raise_for_status()
            except Exception as e:
                elapsed_ms = (time.perf_counter_ns() - t0) / 1_000_000
                log_result(writer, session_id, sample, current_cmd_id, clicked_at, 
                           action_str, action_attempts[action_str], "publish_error", 
                           elapsed_ms, on_confirmed, off_confirmed)
                print(f"[{sample}/{TOTAL_ATTEMPTS}] Error: {e}")
                time.sleep(2)
                continue

            # Polling esperando confirmación
            # Polling esperando confirmación
            result = "timeout"
            while (time.perf_counter_ns() - t0) / 1_000_000_000 < TIMEOUT_S:
                try:
                    params = {"node_id": NODE_ID}
                    get_res = requests.get(API_URL, headers=headers, params=params, timeout=5)
                    
                    if get_res.status_code == 200:
                        data = get_res.json()
                        
                        # --- DEPURACIÓN: Vemos qué responde AWS ---
                        # print(f"    -> JSON Recibido: {data}")
                        # -------------------------------------------
                        
                        reported_cmd_id = data.get(DEVICE_NAME, {}).get(PARAM_CMD_ID)
                        reported_power = data.get(DEVICE_NAME, {}).get(PARAM_POWER)
                        
                        if reported_cmd_id == current_cmd_id and reported_power == target_state:
                            result = "confirmed"
                            if target_state: on_confirmed += 1
                            else: off_confirmed += 1
                            break
                    else:
                        print(f"    -> Error en GET: {get_res.status_code}")
                            
                except requests.exceptions.RequestException as e:
                    print(f"    -> Error de conexión: {e}")
                    
                time.sleep(POLL_INTERVAL_S)
            # result = "timeout"
            # while (time.perf_counter_ns() - t0) / 1_000_000_000 < TIMEOUT_S:
            #     try:
            #         params = {"node_id": NODE_ID}
            #         get_res = requests.get(API_URL, headers=headers, params=params, timeout=5)
                    
            #         if get_res.status_code == 200:
            #             data = get_res.json()
            #             reported_cmd_id = data.get(NODE_ID, {}).get(DEVICE_NAME, {}).get(PARAM_CMD_ID)
            #             reported_power = data.get(NODE_ID, {}).get(DEVICE_NAME, {}).get(PARAM_POWER)
                        
            #             if reported_cmd_id == current_cmd_id and reported_power == target_state:
            #                 result = "confirmed"
            #                 if target_state: on_confirmed += 1
            #                 else: off_confirmed += 1
            #                 break
                            
            #     except requests.exceptions.RequestException:
            #         pass
            #     time.sleep(POLL_INTERVAL_S)

            t1 = time.perf_counter_ns()
            elapsed_ms = (t1 - t0) / 1_000_000

            log_result(writer, session_id, sample, current_cmd_id, clicked_at, 
                       action_str, action_attempts[action_str], result, 
                       elapsed_ms, on_confirmed, off_confirmed)
            
            print(f"[{sample}/{TOTAL_ATTEMPTS}] {action_str} | ID: {current_cmd_id} | Tiempo: {elapsed_ms:.1f} ms")
            time.sleep(1)

    print("Ensayo finalizado. Generando gráfico...")

def log_result(writer, session, sample, cmd_id, clicked_at, action, attempt, result, elapsed_ms, on_conf, off_conf):
    writer.writerow([session, sample, cmd_id, clicked_at, "AWS RainMaker REST", 
                     action, attempt, result, round(elapsed_ms, 2), on_conf, off_conf])

# =====================================================================
# FUNCIONES DE GRAFICACIÓN (Estilo Matplotlib/Pandas)
# =====================================================================

def generar_grafico():
    df = pd.read_csv(CSV_FILENAME)
    df_conf = df[df['result'] == 'confirmed']
    
    if df_conf.empty:
        print("No hay datos confirmados para graficar.")
        return

    # Cálculos estadísticos
    mediana = df_conf['elapsed_ms'].median()
    p95 = df_conf['elapsed_ms'].quantile(0.95)
    max_val = df_conf['elapsed_ms'].max()
    max_idx = df_conf['elapsed_ms'].idxmax()
    max_row = df_conf.loc[max_idx]
    intentos_totales = len(df)
    confirmadas = len(df_conf)

    # Filtrar datos por acción
    on_data = df_conf[df_conf['action'] == 'ON']
    off_data = df_conf[df_conf['action'] == 'OFF']

    # Configuración de la figura
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8), sharex=True)
    fig.suptitle('Tiempo de respuesta del ventilador (RainMaker - Nube)', fontsize=16, fontweight='bold', x=0.1, ha='left')
    plt.figtext(0.1, 0.92, 'Desde la petición HTTP REST en Python hasta la confirmación MQTT del ESP32 · recorrido completo de ida y vuelta', fontsize=10, color='gray')
    plt.figtext(0.1, 0.88, f'{confirmadas} confirmadas / {intentos_totales} intentos   |   Mediana: {mediana:.1f} ms   |   P95: {p95:.1f} ms   |   Máximo: {max_val:.1f} ms', fontsize=11)

    # Colores y estilos
    c_on = '#1f77b4' # Azul
    c_off = '#d65f12' # Naranja oscuro

    # --- PANEL A: Escala Completa ---
    ax1.set_title('A. Todas las mediciones · escala completa', loc='left', fontweight='bold')
    ax1.scatter(on_data['sample'], on_data['elapsed_ms'], color=c_on, label=f'ON · Encender (n={len(on_data)})', marker='o', s=25, zorder=3)
    ax1.scatter(off_data['sample'], off_data['elapsed_ms'], color=c_off, label=f'OFF · Apagar (n={len(off_data)})', marker='s', s=25, zorder=3)
    ax1.axhline(mediana, color='gray', linestyle='--', linewidth=1, label=f'Mediana global ({mediana:.1f} ms)', zorder=1)
    
    # Anotación del valor máximo
    ax1.annotate(f'Máximo: {max_val:.1f} ms\nIntento {max_row["sample"]} · {max_row["action"]}', 
                 xy=(max_row['sample'], max_val), xytext=(0, 10), textcoords='offset points',
                 ha='center', va='bottom', color=c_off if max_row['action'] == 'OFF' else c_on, fontsize=9)
    
    ax1.set_ylim(bottom=0, top=max_val * 1.20)
    ax1.set_ylabel('Tiempo clic -> confirmación (ms)')
    ax1.grid(axis='y', linestyle='-', alpha=0.3, zorder=0)
    ax1.legend(loc='upper right', framealpha=1)

    # --- PANEL B: Rango Habitual ---
    limite_y = p95 * 1.5 # Dinámico según el P95 para hacer el zoom
    puntos_fuera = len(df_conf[df_conf['elapsed_ms'] > limite_y])
    
    ax2.set_title(f'B. Detalle del rango habitual · 0-{int(limite_y)} ms ({puntos_fuera} puntos superiores visibles en A)', loc='left', fontweight='bold')
    ax2.scatter(on_data['sample'], on_data['elapsed_ms'], color=c_on, marker='o', s=25, zorder=3)
    ax2.scatter(off_data['sample'], off_data['elapsed_ms'], color=c_off, marker='s', s=25, zorder=3)
    ax2.axhline(mediana, color='gray', linestyle='--', linewidth=1, zorder=1)
    
    ax2.set_ylim(0, limite_y)
    ax2.set_ylabel('Tiempo clic -> confirmación (ms)')
    ax2.set_xlabel('Número de intento en orden de clic (no es tiempo transcurrido)')
    ax2.set_xticks(np.arange(15, TOTAL_ATTEMPTS + 1, 15))
    ax2.grid(axis='y', linestyle='-', alpha=0.3, zorder=0)

    # --- Footer ---
    plt.figtext(0.1, 0.02, f'Cada punto representa una orden confirmada. Sin suavizado ni eliminación de valores extremos.\nP95: el 95 % de las confirmaciones queda por debajo de ese tiempo. Fuente: {CSV_FILENAME}', color='gray', fontsize=8)

    # Ajustes finales y guardado
    plt.tight_layout(rect=[0, 0.05, 1, 0.85])
    nombre_grafico = "grafico_rtt_rainmaker.png"
    plt.savefig(nombre_grafico, dpi=300, bbox_inches='tight')
    print(f"Gráfico guardado exitosamente como '{nombre_grafico}'.")
    plt.show()

if __name__ == "__main__":
    realizar_ensayo()
    generar_grafico()