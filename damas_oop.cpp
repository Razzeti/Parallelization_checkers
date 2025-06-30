#include <iostream>
#include <vector>
#include <string>
#include <stdexcept> 
#include <algorithm>
#include <cctype>   
#include <chrono>   
#include <omp.h>
#include <iomanip>

using namespace std;

// --- Constantes ---
const int TAM_TABLERO = 10;
const char JUGADOR_B = 'B'; 
const char JUGADOR_N = 'N'; 
const char REINA_B = 'X';
const char REINA_N = 'Y';
const char VACIO = '.';

// --- Definición de la Clase ---
class JuegoDamas {
private:
    vector<vector<char>> tablero;
    int movimientos_sin_captura;
    char jugador_actual;
    long long tiempo_ia_secuencial_ns;
    long long tiempo_ia_paralelo_ns;
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

    int minimax(int profundidad, bool esTurnoMaximizador) {
        // Caso Base: Si llegamos al final de la búsqueda o no hay movimientos
        if (profundidad == 0) {
            return evaluarPosicion();
        }

        vector<string> movimientos = getMovimientosLegales();
        if (movimientos.empty()) {
            return evaluarPosicion();
        }

        if (esTurnoMaximizador) { // Turno de MAX (Nuestra IA)
            int mejorPuntuacion = -99999;
            for (const string& mov : movimientos) {
                JuegoDamas copia_juego = *this;
                copia_juego.realizarMovimiento(mov);
                int puntuacion = copia_juego.minimax(profundidad - 1, false); // El siguiente turno es del oponente (MIN)
                mejorPuntuacion = max(mejorPuntuacion, puntuacion);
            }
            return mejorPuntuacion;
        } else { // Turno de MIN (El Oponente)
            int peorPuntuacion = 99999;
            for (const string& mov : movimientos) {
                JuegoDamas copia_juego = *this;
                copia_juego.realizarMovimiento(mov);
                int puntuacion = copia_juego.minimax(profundidad - 1, true); // El siguiente turno es nuestro (MAX)
                peorPuntuacion = min(peorPuntuacion, puntuacion);
            }
            return peorPuntuacion;
        }
    }
public:
    JuegoDamas() {
        tablero.resize(TAM_TABLERO, vector<char>(TAM_TABLERO, VACIO));
        inicializarTablero();
        movimientos_sin_captura = 0;
        jugador_actual = JUGADOR_B;
        tiempo_ia_secuencial_ns = 0;
        tiempo_ia_paralelo_ns = 0;
        hilos_usados = 1; 
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
    
    // --> FUNCIÓN RESTAURADA, por si se necesita para depurar
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
                
                if ( (jugador == JUGADOR_B && (pieza != JUGADOR_B && pieza != REINA_B)) ||
                     (jugador == JUGADOR_N && (pieza != JUGADOR_N && pieza != REINA_N)) ) {
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
    
    string getMovimientosLegalesComoString() {
        vector<string> movimientos = getMovimientosLegales();
        if (movimientos.empty()) return "";
        string resultado = "";
        for (size_t i = 0; i < movimientos.size(); ++i) {
            resultado += movimientos[i];
            if (i < movimientos.size() - 1) resultado += "|";
        }
        return resultado;
    }

    bool realizarMovimiento(const string& mov) {
        if (mov.length() < 5) return false;
        size_t espacio_pos = mov.find(' ');
        if (espacio_pos == string::npos) return false;
        string origen_str = mov.substr(0, espacio_pos);
        string destino_str = mov.substr(espacio_pos + 1);
        if (origen_str.length() < 2 || destino_str.length() < 2) return false;
        int y1 = toupper(origen_str[0]) - 'A';
        int x1 = TAM_TABLERO - stoi(origen_str.substr(1));
        int y2 = toupper(destino_str[0]) - 'A';
        int x2 = TAM_TABLERO - stoi(destino_str.substr(1));
        char pieza = tablero[x1][y1];
        if (pieza == VACIO) return false;
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

    // --> MÉTODO DE IA #1: SECUENCIAL
    // Lógica con un bucle simple.
    string getMovimientoAI_Secuencial(int profundidad) {
        auto inicio = chrono::high_resolution_clock::now();
        vector<string> movimientos = getMovimientosLegales();
        if (movimientos.empty()) {
            this->tiempo_ia_secuencial_ns = 0;
            return "";
        }
        string mejor_movimiento = movimientos[0];
        int mejor_puntuacion = (jugador_actual == JUGADOR_B) ? -99999 : 99999;

        for (const string& mov : movimientos) {
            JuegoDamas copia_juego = *this;

            copia_juego.realizarMovimiento(mov);
            int puntuacion_movimiento = copia_juego.minimax(profundidad - 1, false);

            if (jugador_actual == JUGADOR_B) {
                if (puntuacion_movimiento > mejor_puntuacion) {
                    mejor_puntuacion = puntuacion_movimiento;
                    mejor_movimiento = mov;
                }
            } else {
                if (puntuacion_movimiento < mejor_puntuacion) {
                    mejor_puntuacion = puntuacion_movimiento;
                    mejor_movimiento = mov;
                }
            }
        }

        auto fin = chrono::high_resolution_clock::now();
        this->tiempo_ia_secuencial_ns = chrono::duration_cast<chrono::nanoseconds>(fin - inicio).count();
        return mejor_movimiento;
    }

    // --> MÉTODO DE IA #2: PARALELO
    // La misma lógica, pero usando OpenMP.
    string getMovimientoAI_Paralelo(int profundidad) {
        auto inicio = chrono::high_resolution_clock::now();
        vector<string> movimientos = getMovimientosLegales();
        if (movimientos.empty()) {
            this->tiempo_ia_paralelo_ns = 0;
            this->hilos_usados = 1;
            return "";
        }
        string mejor_movimiento_global = movimientos[0];
        int mejor_puntuacion_global = (jugador_actual == JUGADOR_B) ? -99999 : 99999;

        #pragma omp parallel
        {
            string mejor_movimiento_local = mejor_movimiento_global;
            int mejor_puntuacion_local = mejor_puntuacion_global;
            #pragma omp for
            for (int i = 0; i < movimientos.size(); ++i) {
                const string& mov = movimientos[i];
                JuegoDamas copia_juego = *this;
                copia_juego.realizarMovimiento(mov);
                int puntuacion_movimiento = copia_juego.minimax(profundidad - 1, false);

                if (jugador_actual == JUGADOR_B) {
                    if (puntuacion_movimiento > mejor_puntuacion_local) {
                        mejor_puntuacion_local = puntuacion_movimiento;
                        mejor_movimiento_local = mov;
                    }
                } else {
                    if (puntuacion_movimiento < mejor_puntuacion_local) {
                        mejor_puntuacion_local = puntuacion_movimiento;
                        mejor_movimiento_local = mov;
                    }
                }
            }
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
            #pragma omp single
            {
                this->hilos_usados = omp_get_num_threads();
            }
        }
        auto fin = chrono::high_resolution_clock::now();
        this->tiempo_ia_paralelo_ns = chrono::duration_cast<chrono::nanoseconds>(fin - inicio).count();
        return mejor_movimiento_global;
    }
    long long getTiempoSecuencialNS() const { return tiempo_ia_secuencial_ns; }
    long long getTiempoParaleloNS() const { return tiempo_ia_paralelo_ns; }
    
    int getHilosUsados() const { return hilos_usados; }
    char getJugadorActual() const { return jugador_actual; }

}; // <-- Fin de la clase JuegoDamas

// --- Interfaz para WebAssembly ---
extern "C" {
    // --- Funciones Generales del Juego ---
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

    // Devuelve una lista de movimientos legales separados por '|'
    const char* get_movimientos_legales_str(JuegoDamas* juego) {
        static string movs_str;
        movs_str = juego->getMovimientosLegalesComoString();
        return movs_str.c_str();
    }

    // Realiza un movimiento en el tablero
    bool realizar_movimiento(JuegoDamas* juego, const char* mov) {
        return juego->realizarMovimiento(string(mov));
    }

    // --- Funciones de IA SECUENCIAL ---
    const char* get_movimiento_ia_secuencial(JuegoDamas* juego, int profundidad) {
        static string mov_ia;
        mov_ia = juego->getMovimientoAI_Secuencial(profundidad);
        return mov_ia.c_str();
    }

    long long get_last_ai_time_secuencial_ns(JuegoDamas* juego) {
        return juego->getTiempoSecuencialNS();
    }

    // --- Funciones de IA PARALELA ---
    const char* get_movimiento_ia_paralelo(JuegoDamas* juego,int profundidad) {
        static string mov_ia;
        mov_ia = juego->getMovimientoAI_Paralelo(profundidad);
        return mov_ia.c_str();
    }

    long long get_last_ai_time_paralelo_ns(JuegoDamas* juego) {
        return juego->getTiempoParaleloNS();
    }

    // El número de hilos solo es relevante para la versión paralela
    int get_thread_count(JuegoDamas* juego) {
        return juego->getHilosUsados();
    }
}
// --- Función main() para pruebas en consola (CORREGIDA Y SINCRONIZADA) ---
int main() {
    JuegoDamas juego;
    string mov_humano;
    int modo_ia = 0;
    int profundidad_busqueda = 0;

    while (modo_ia != 1 && modo_ia != 2) {
        cout << "Elige el modo de IA para jugar:" << endl;
        cout << "1. IA SECUENCIAL" << endl;
        cout << "2. IA PARALELA" << endl;
        cout << "Tu elección (1 o 2): ";
        cin >> modo_ia;
        if (cin.fail() || (modo_ia != 1 && modo_ia != 2)) {
            cout << "Opción inválida." << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            modo_ia = 0;
        }
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    while (profundidad_busqueda <= 0) {
        cout << "\nIntroduce la profundidad de búsqueda de la IA (ej: 2, 3, 4...): ";
        cout << "\n(Advertencia: Profundidades > 4 pueden ser MUY lentas)" << endl;
        cout << "Profundidad: ";
        cin >> profundidad_busqueda;
        if (cin.fail() || profundidad_busqueda <= 0) {
            cout << "Entrada inválida. Introduce un número entero positivo." << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            profundidad_busqueda = 0;
        }
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "\nHas elegido la IA " << (modo_ia == 1 ? "SECUENCIAL" : "PARALELA") << " con profundidad " << profundidad_busqueda << ". ¡Que empiece el juego!" << endl;
    
    while (true) {
        juego.imprimirTablero();
        vector<string> movimientos = juego.getMovimientosLegales();
        if (movimientos.empty()) {
            cout << "\n===================================" << endl;
            cout << "JUEGO TERMINADO." << endl;
            if (juego.getJugadorActual() == JUGADOR_B) {
                cout << "¡Ganan las Negras! Las Blancas no tienen movimientos." << endl;
            } else {
                cout << "¡Ganan las Blancas! Las Negras no tienen movimientos." << endl;
            }
            cout << "===================================" << endl;
            break;
        }

        string mov_elegido;
        if (juego.getJugadorActual() == JUGADOR_B) {
            while(true) {
                cout << "\nTu turno. Introduce el movimiento (ej: C3 D4): ";
                getline(cin, mov_humano);
                if (find(movimientos.begin(), movimientos.end(), mov_humano) != movimientos.end()) {
                    mov_elegido = mov_humano;
                    break; 
                } else {
                    cout << "--> Movimiento '" << mov_humano << "' no es legal. Inténtalo de nuevo." << endl;
                }
            }
        } else { 
            cout << "\nTurno de la IA (Negras)... pensando con profundidad " << profundidad_busqueda << "..." << endl;
            
            if (modo_ia == 1) {
                mov_elegido = juego.getMovimientoAI_Secuencial(profundidad_busqueda);
                juego.getMovimientoAI_Paralelo(profundidad_busqueda);
            } else {
                mov_elegido = juego.getMovimientoAI_Paralelo(profundidad_busqueda);
                juego.getMovimientoAI_Secuencial(profundidad_busqueda);
            }

            long long tiempo_s = juego.getTiempoSecuencialNS();
            long long tiempo_p = juego.getTiempoParaleloNS();
            int hilos = juego.getHilosUsados();

            cout << "La IA (" << (modo_ia == 1 ? "SECUENCIAL" : "PARALELA") << ") mueve: " << mov_elegido << endl;
            cout << "--- Benchmark del Turno ---" << endl;
            cout << "Tiempo Secuencial: " << (double)tiempo_s / 1000000.0 << " ms" << endl;
            cout << "Tiempo Paralelo  : " << (double)tiempo_p / 1000000.0 << " ms (usando " << hilos << " hilos)" << endl;
            if (tiempo_s > 0 && tiempo_p > 0) {
                if (tiempo_p < tiempo_s) {
                    double speedup = (double)tiempo_s / tiempo_p;
                    cout << "Speedup Paralelo: " << fixed << setprecision(2) << speedup << "x más rápido" << endl;
                } else {
                    double slowdown = (double)tiempo_p / tiempo_s;
                    cout << "Speedup Paralelo: " << fixed << setprecision(2) << slowdown << "x más lento" << endl;
                }
            }
            cout << "---------------------------" << endl;
        }
        
        juego.realizarMovimiento(mov_elegido);
    }
    return 0;
}
