#include <iostream>
#include <vector>
#include <string>
#include <stdexcept> 
#include <algorithm>
#include <cctype>   
#include <chrono>    
#include <omp.h>

using namespace std;

const int TAM_TABLERO = 10;
const char JUGADOR_B = 'B'; 
const char JUGADOR_N = 'N'; 
const char REINA_B = 'X';
const char REINA_N = 'Y';
const char VACIO = '.';

class JuegoDamas {
private:
    vector<vector<char>> tablero;
    int movimientos_sin_captura;
    char jugador_actual;

    double ultimo_tiempo_ia_ms;
    int hilos_usados;

    int evaluarPosicion() {
        int puntuacion = 0;
        for (int i = 0; i < TAM_TABLERO; ++i) {
            for (int j = 0; j < TAM_TABLERO; ++j) {
                switch (tablero[i][j]) {
                    case JUGADOR_B: puntuacion += 1; break;
                    case REINA_B:   puntuacion += 3; break;
                    case JUGADOR_N: puntuacion -= 1; break;
                    case REINA_N:   puntuacion -= 3; break;
                }
            }
        }
        return puntuacion;
    }


public:
    JuegoDamas() {
        tablero.resize(TAM_TABLERO, vector<char>(TAM_TABLERO, VACIO));
        inicializarTablero();
        movimientos_sin_captura = 0;
        jugador_actual = JUGADOR_B;
        ultimo_tiempo_ia_ms = 0.0;
        hilos_usados = 0; // Se establecerá al llamar a la IA
    }

    void inicializarTablero() {
        for (int i = 0; i < TAM_TABLERO; i++) {
            for (int j = 0; j < TAM_TABLERO; j++) {
                if ((i + j) % 2 == 1) {
                    if (i < 4) tablero[i][j] = JUGADOR_N;
                    else if (i > 5) tablero[i][j] = JUGADOR_B;
                    else tablero[i][j] = VACIO;
                } else {
                    tablero[i][j] = VACIO;
                }
            }
        }
    }

    string getTableroString() const {
        string s = "";
        for (int i = 0; i < TAM_TABLERO; ++i) {
            for (int j = 0; j < TAM_TABLERO; ++j) {
                s += tablero[i][j];
            }
        }
        return s;
    }

    void imprimirTablero() const {
        cout << "\n   A B C D E F G H I J" << endl;
        cout << "  ---------------------" << endl;
        for (int i = 0; i < TAM_TABLERO; ++i) {
            cout << (TAM_TABLERO - i) << (TAM_TABLERO - i < 10 ? " |" : "|");
            for (int j = 0; j < TAM_TABLERO; ++j) {
                cout << tablero[i][j] << " ";
            }
            cout << "|" << endl;
        }
        cout << "  ---------------------" << endl;
        cout << "   A B C D E F G H I J" << endl;
        cout << "Turno de: " << (jugador_actual == JUGADOR_B ? "Blancas (B/X)" : "Negras (N/Y)") << endl;
    }


    vector<string> generarMovimientos(char jugador, bool solo_capturas) {
        vector<string> movimientos;

        for (int i = 0; i < TAM_TABLERO; i++) {
            for (int j = 0; j < TAM_TABLERO; j++) {
                char pieza = tablero[i][j];
                bool es_reina = (pieza == REINA_B || pieza == REINA_N);

                if (!(pieza == jugador || (es_reina && toupper(pieza) == toupper(jugador)))) {
                    continue;
                }

                for (int di = -2; di <= 2; di += 4) {
                    for (int dj = -2; dj <= 2; dj += 4) {
                        if (!es_reina && ((jugador == JUGADOR_B && di > 0) || (jugador == JUGADOR_N && di < 0))) continue;

                        int ni = i + di, nj = j + dj;
                        int mi = i + di / 2, mj = j + dj / 2;

                        if (ni >= 0 && ni < TAM_TABLERO && nj >= 0 && nj < TAM_TABLERO && tablero[ni][nj] == VACIO) {
                            char pieza_saltada = tablero[mi][mj];
                            bool es_enemigo = false;
                            if (jugador == JUGADOR_B && (pieza_saltada == JUGADOR_N || pieza_saltada == REINA_N)) es_enemigo = true;
                            if (jugador == JUGADOR_N && (pieza_saltada == JUGADOR_B || pieza_saltada == REINA_B)) es_enemigo = true;

                            if (es_enemigo) {
                                movimientos.push_back(string(1, 'A' + j) + to_string(TAM_TABLERO - i) + " " + string(1, 'A' + nj) + to_string(TAM_TABLERO - ni));
                            }
                        }
                    }
                }

                if (!solo_capturas) {
                    for (int di = -1; di <= 1; di += 2) {
                        for (int dj = -1; dj <= 1; dj += 2) {
                            if (!es_reina && ((jugador == JUGADOR_B && di > 0) || (jugador == JUGADOR_N && di < 0))) continue;

                            int ni = i + di, nj = j + dj;
                            if (ni >= 0 && ni < TAM_TABLERO && nj >= 0 && nj < TAM_TABLERO && tablero[ni][nj] == VACIO) {
                                movimientos.push_back(string(1, 'A' + j) + to_string(TAM_TABLERO - i) + " " + string(1, 'A' + nj) + to_string(TAM_TABLERO - ni));
                            }
                        }
                    }
                }
            }
        }
        return movimientos;
    }

    vector<string> getMovimientosLegales() {
        vector<string> movs_captura = generarMovimientos(jugador_actual, true);
        if (!movs_captura.empty()) {
            return movs_captura;
        }
        return generarMovimientos(jugador_actual, false);
    }

    bool realizarMovimiento(const string& mov) {
        if (mov.length() < 5) return false;

        //amplicacion de robustez
        size_t espacio_pos = mov.find(' ');
        if (espacio_pos == string::npos) return false; // No se encontró espacio
        
        string origen_str = mov.substr(0, espacio_pos);
        string destino_str = mov.substr(espacio_pos + 1);
        
        if (origen_str.length() < 2 || destino_str.length() < 2) return false; // Formato inválido
        
        int y1 = toupper(origen_str[0]) - 'A';
        int x1 = TAM_TABLERO - stoi(origen_str.substr(1));
        int y2 = toupper(destino_str[0]) - 'A';
        int x2 = TAM_TABLERO - stoi(destino_str.substr(1));

        char pieza = tablero[x1][y1];
        tablero[x2][y2] = pieza;
        tablero[x1][y1] = VACIO;

        if (abs(x2 - x1) == 2) { 
            tablero[(x1 + x2) / 2][(y1 + y2) / 2] = VACIO;
            movimientos_sin_captura = 0;
        } else {
            movimientos_sin_captura++;
        }

        if (x2 == 0 && tablero[x2][y2] == JUGADOR_B) tablero[x2][y2] = REINA_B;
        if (x2 == TAM_TABLERO - 1 && tablero[x2][y2] == JUGADOR_N) tablero[x2][y2] = REINA_N;

        jugador_actual = (jugador_actual == JUGADOR_B) ? JUGADOR_N : JUGADOR_B;
        return true;
    }

    string getMovimientoAI() {
        auto inicio = chrono::high_resolution_clock::now();

        vector<string> movimientos = getMovimientosLegales();
        if (movimientos.empty()) {
            this->ultimo_tiempo_ia_ms = 0;
            this->hilos_usados = 1;
            return "";
        }

        string mejor_movimiento_global = movimientos[0]; // Empezar con una opción por defecto
        int mejor_puntuacion_global = (jugador_actual == JUGADOR_B) ? -99999 : 99999;

        // 3. Iniciamos la región paralela. Todo lo que está dentro de {} será ejecutado por un equipo de hilos.
        #pragma omp parallel
        {
            // 4. CADA HILO crea sus propias variables locales para no entrar en conflicto con los demás.
            string mejor_movimiento_local = mejor_movimiento_global;
            int mejor_puntuacion_local = mejor_puntuacion_global;

            // 5. El bucle 'for' se distribuye entre los hilos. Cada hilo coge un subconjunto de movimientos.
            #pragma omp for
            for (int i = 0; i < movimientos.size(); ++i) {
                const string& mov = movimientos[i];

                // ---- Este es el "corazón" de tu lógica, ejecutándose en paralelo ----
                JuegoDamas copia_juego = *this;
                copia_juego.realizarMovimiento(mov);
                int puntuacion_actual = copia_juego.evaluarPosicion();
                // --------------------------------------------------------------------

                // Cada hilo compara y actualiza SU PROPIO mejor movimiento local
                if (jugador_actual == JUGADOR_B) {
                    if (puntuacion_actual > mejor_puntuacion_local) {
                        mejor_puntuacion_local = puntuacion_actual;
                        mejor_movimiento_local = mov;
                    }
                } else {
                    if (puntuacion_actual < mejor_puntuacion_local) {
                        mejor_puntuacion_local = puntuacion_actual;
                        mejor_movimiento_local = mov;
                    }
                }
            } // <-- Aquí termina el trabajo de cada hilo

            // 6. SINCRONIZACIÓN FINAL (Reducción)
            // Cada hilo entra aquí uno por uno para comparar su mejor hallazgo local con el mejor global.
            #pragma omp critical
            {
                if (jugador_actual == JUGADOR_B) {
                    if (mejor_puntuacion_local > mejor_puntuacion_global) {
                        mejor_puntuacion_global = mejor_puntuacion_local;
                        mejor_movimiento_global = mejor_movimiento_local;
                    }
                } else {
                    if (mejor_puntuacion_local < mejor_puntuacion_global) {
                        mejor_puntuacion_global = mejor_puntuacion_local;
                        mejor_movimiento_global = mejor_movimiento_local;
                    }
                }
            }
            
            // Un solo hilo (single) se encarga de actualizar el conteo de hilos usados.
            #pragma omp single
            {
                this->hilos_usados = omp_get_num_threads();
            }
        } // <-- Aquí termina la región paralela

        auto fin = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> duracion = fin - inicio;

        // Guardamos las estadísticas en las variables de la clase
        this->ultimo_tiempo_ia_ms = duracion.count();

        // 7. Devolvemos el mejor movimiento encontrado entre todos los hilos.
        return mejor_movimiento_global;
};


extern "C" {
    JuegoDamas* crear_juego() {
        return new JuegoDamas();
    }

    void destruir_juego(JuegoDamas* juego) {
        delete juego;
    }

    const char* get_tablero(JuegoDamas* juego) {
        static string tablero_str;
        tablero_str = juego->getTableroString();
        return tablero_str.c_str();
    }

    bool realizar_movimiento(JuegoDamas* juego, const char* mov) {
        return juego->realizarMovimiento(string(mov));
    }

    const char* get_movimiento_ia(JuegoDamas* juego) {
        static string mov_ia;
        mov_ia = juego->getMovimientoAI();
        return mov_ia.c_str();
    }
    double get_last_ai_time(JuegoDamas* juego) {
        return juego->getUltimoTiempoIA();
    }

    int get_thread_count(JuegoDamas* juego) {
        return juego->getHilosUsados();
    }
}


int main() {
    JuegoDamas juego;
    string mov_humano;

    while (true) {
        juego.imprimirTablero();
        vector<string> movimientos = juego.getMovimientosLegales();

        if (movimientos.empty()) {
            cout << "Juego terminado. El jugador " << juego.getJugadorActual() << " no tiene movimientos." << endl;
            break;
        }

        string mov_elegido;
        if (juego.getJugadorActual() == JUGADOR_B) {
            cout << "Tu movimiento (ej: D6 E5): ";
            getline(cin, mov_humano);

            if (find(movimientos.begin(), movimientos.end(), mov_humano) != movimientos.end()) {
                mov_elegido = mov_humano;
            } else {
                cout << "Movimiento ilegal. Intenta de nuevo." << endl;
                continue;
            }

        } else { 
            cout << "Turno de la IA (Negras)..." << endl;
            mov_elegido = juego.getMovimientoAI();
            cout << "La IA mueve: " << mov_elegido << endl;
        }

        juego.realizarMovimiento(mov_elegido);
    }

    return 0;
}
