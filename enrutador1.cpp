#include <iostream>
#include <unordered_map>
#include <vector>
#include <set>
#include <limits>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <iomanip>  // Para mejor formato de salida
#include <chrono>   // Para medir tiempos de respuesta
#include <queue>    // Para implementación alternativa de Dijkstra

using namespace std;

// Estructura para almacenar estadísticas de la red
struct EstadisticasRed {
    int totalEnrutadores;
    int totalEnlaces;
    double costoPromedio;
    int costoMinimo;
    int costoMaximo;
    int gradoMaximo;  // Número máximo de conexiones de un enrutador
};

class Enrutador {
private:
    unordered_map<string, int> tablaEnrutamiento;
    string nombre;
    chrono::system_clock::time_point ultimaActualizacion;
    vector<string> historialCambios;

public:
    explicit Enrutador(const string& nombreEnrutador = "") : 
        nombre(nombreEnrutador), 
        ultimaActualizacion(chrono::system_clock::now()) {}

    // Getters y setters mejorados
    void establecerNombre(const string& nombreEnrutador) {
        if (nombreEnrutador.empty()) {
            throw invalid_argument("El nombre del enrutador no puede estar vacío");
        }
        nombre = nombreEnrutador;
        registrarCambio("Cambio de nombre a: " + nombreEnrutador);
    }

    const string& obtenerNombre() const { return nombre; }

    void actualizarRuta(const string& destino, int costo) {
        if (costo < 0) {
            throw invalid_argument("El costo no puede ser negativo");
        }
        tablaEnrutamiento[destino] = costo;
        ultimaActualizacion = chrono::system_clock::now();
        registrarCambio("Actualización de ruta a " + destino + " con costo " + to_string(costo));
    }

    void eliminarRuta(const string& destino) {
        auto it = tablaEnrutamiento.find(destino);
        if (it != tablaEnrutamiento.end()) {
            tablaEnrutamiento.erase(it);
            registrarCambio("Eliminación de ruta a " + destino);
        }
    }

    int obtenerCosto(const string& destino) const {
        auto it = tablaEnrutamiento.find(destino);
        return (it != tablaEnrutamiento.end()) ? it->second : numeric_limits<int>::max();
    }

    const unordered_map<string, int>& obtenerTablaEnrutamiento() const {
        return tablaEnrutamiento;
    }

    // Nuevos métodos
    int obtenerGrado() const {
        return tablaEnrutamiento.size();
    }

    chrono::system_clock::time_point obtenerUltimaActualizacion() const {
        return ultimaActualizacion;
    }

    const vector<string>& obtenerHistorialCambios() const {
        return historialCambios;
    }

private:
    void registrarCambio(const string& cambio) {
        auto tiempo = chrono::system_clock::to_time_t(chrono::system_clock::now());
        historialCambios.push_back(string(ctime(&tiempo)) + ": " + cambio);
    }
};

class Red {
private:
    unordered_map<string, Enrutador> enrutadores;
    vector<string> historialCambios;
    chrono::system_clock::time_point creacion;

public:
    Red() : creacion(chrono::system_clock::now()) {}

    bool existeEnrutador(const string& nombre) const {
        return enrutadores.find(nombre) != enrutadores.end();
    }

    void agregarEnrutador(const string& nombre) {
        if (nombre.empty()) {
            throw invalid_argument("El nombre del enrutador no puede estar vacío");
        }
        if (existeEnrutador(nombre)) {
            throw invalid_argument("Ya existe un enrutador con ese nombre");
        }
        enrutadores.emplace(nombre, Enrutador(nombre));
        registrarCambio("Agregado nuevo enrutador: " + nombre);
    }

    void eliminarEnrutador(const string& nombre) {
        if (!existeEnrutador(nombre)) {
            throw invalid_argument("Enrutador no encontrado");
        }
        enrutadores.erase(nombre);
        for (auto& [_, enrutador] : enrutadores) {
            enrutador.eliminarRuta(nombre);
        }
        registrarCambio("Eliminado enrutador: " + nombre);
    }

    void actualizarEnlace(const string& origen, const string& destino, int costo) {
        if (!existeEnrutador(origen) || !existeEnrutador(destino)) {
            throw invalid_argument("Enrutador origen o destino no existe");
        }
        if (costo < 0) {
            throw invalid_argument("El costo no puede ser negativo");
        }
        enrutadores.at(origen).actualizarRuta(destino, costo);
        enrutadores.at(destino).actualizarRuta(origen, costo);
        registrarCambio("Actualizado enlace " + origen + " <-> " + destino + " con costo " + to_string(costo));
    }

    void cargarTopologiaDesdeArchivo(const string& nombreArchivo) {
        ifstream archivo(nombreArchivo);
        if (!archivo) {
            throw runtime_error("No se pudo abrir el archivo: " + nombreArchivo);
        }

        // Limpiamos la red actual
        enrutadores.clear();
        registrarCambio("Iniciando carga de topología desde archivo: " + nombreArchivo);

        string linea;
        int numLinea = 0;
        int enlacesCargados = 0;

        while (getline(archivo, linea)) {
            numLinea++;
            // Ignorar líneas vacías y comentarios
            if (linea.empty() || linea[0] == '#') continue;

            stringstream ss(linea);
            string origen, destino;
            int costo;

            if (!(ss >> origen >> destino >> costo)) {
                throw runtime_error("Error en formato de línea " + to_string(numLinea));
            }

            try {
                if (!existeEnrutador(origen)) agregarEnrutador(origen);
                if (!existeEnrutador(destino)) agregarEnrutador(destino);
                actualizarEnlace(origen, destino, costo);
                enlacesCargados++;
            } catch (const exception& e) {
                throw runtime_error("Error en línea " + to_string(numLinea) + ": " + e.what());
            }
        }

        registrarCambio("Topología cargada exitosamente: " + 
                       to_string(enrutadores.size()) + " enrutadores, " + 
                       to_string(enlacesCargados) + " enlaces");
    }

    pair<int, vector<string>> encontrarRutaMasCorta(const string& origen, const string& destino) const {
        if (!existeEnrutador(origen) || !existeEnrutador(destino)) {
            throw invalid_argument("Enrutador origen o destino no existe");
        }

        unordered_map<string, int> distancias;
        unordered_map<string, string> anterior;
        priority_queue<pair<int, string>, vector<pair<int, string>>, greater<>> cola;

        // Inicialización
        for (const auto& [nombre, _] : enrutadores) {
            distancias[nombre] = numeric_limits<int>::max();
        }
        distancias[origen] = 0;
        cola.push({0, origen});

        while (!cola.empty()) {
            auto [dist, actual] = cola.top();
            cola.pop();

            if (actual == destino) break;
            if (dist > distancias[actual]) continue;

            const auto& enrutadorActual = enrutadores.at(actual);
            for (const auto& [vecino, costo] : enrutadorActual.obtenerTablaEnrutamiento()) {
                int nuevaDist = dist + costo;
                if (nuevaDist < distancias[vecino]) {
                    distancias[vecino] = nuevaDist;
                    anterior[vecino] = actual;
                    cola.push({nuevaDist, vecino});
                }
            }
        }

        vector<string> ruta;
        if (distancias[destino] == numeric_limits<int>::max()) {
            return {-1, ruta};
        }

        for (string actual = destino; actual != origen; actual = anterior.at(actual)) {
            ruta.push_back(actual);
        }
        ruta.push_back(origen);
        reverse(ruta.begin(), ruta.end());

        return {distancias[destino], ruta};
    }

    void generarRedAleatoria(int numEnrutadores, int costoMaximo, double densidad = 0.6) {
        if (numEnrutadores <= 0 || costoMaximo <= 0 || densidad <= 0 || densidad > 1) {
            throw invalid_argument("Parámetros inválidos para generación de red");
        }

        enrutadores.clear();
        registrarCambio("Iniciando generación de red aleatoria");

        // Crear enrutadores
        for (int i = 0; i < numEnrutadores; ++i) {
            agregarEnrutador("E" + to_string(i));
        }

        // Asegurar conectividad mínima
        for (int i = 0; i < numEnrutadores - 1; ++i) {
            int costo = rand() % costoMaximo + 1;
            actualizarEnlace("E" + to_string(i), "E" + to_string(i + 1), costo);
        }

        // Agregar enlaces adicionales según la densidad
        int enlacesMaximos = (numEnrutadores * (numEnrutadores - 1)) / 2;
        int enlacesObjetivo = static_cast<int>(enlacesMaximos * densidad);
        int enlacesActuales = numEnrutadores - 1;

        while (enlacesActuales < enlacesObjetivo) {
            int i = rand() % numEnrutadores;
            int j = rand() % numEnrutadores;
            string origen = "E" + to_string(i);
            string destino = "E" + to_string(j);

            if (i != j && enrutadores[origen].obtenerCosto(destino) == numeric_limits<int>::max()) {
                int costo = rand() % costoMaximo + 1;
                actualizarEnlace(origen, destino, costo);
                enlacesActuales++;
            }
        }

        registrarCambio("Red aleatoria generada con " + to_string(enlacesActuales) + " enlaces");
    }

    void imprimirRed() const {
        cout << "\n=== Estado Actual de la Red ===\n";
        cout << "Número total de enrutadores: " << enrutadores.size() << "\n\n";

        for (const auto& [nombreEnrutador, enrutador] : enrutadores) {
            cout << "Enrutador " << nombreEnrutador << ":\n";
            cout << "  Grado: " << enrutador.obtenerGrado() << " conexiones\n";
            cout << "  Enlaces:\n";
            
            for (const auto& [vecino, costo] : enrutador.obtenerTablaEnrutamiento()) {
                cout << "    -> " << setw(5) << vecino << " | Costo: " << setw(3) << costo << "\n";
            }
            cout << "\n";
        }
    }

    EstadisticasRed obtenerEstadisticas() const {
        EstadisticasRed stats{};
        stats.totalEnrutadores = enrutadores.size();
        stats.totalEnlaces = 0;
        stats.costoMinimo = numeric_limits<int>::max();
        stats.costoMaximo = 0;
        double costoTotal = 0;
        stats.gradoMaximo = 0;

        for (const auto& [_, enrutador] : enrutadores) {
            int grado = enrutador.obtenerGrado();
            stats.gradoMaximo = max(stats.gradoMaximo, grado);
            
            for (const auto& [__, costo] : enrutador.obtenerTablaEnrutamiento()) {
                stats.costoMinimo = min(stats.costoMinimo, costo);
                stats.costoMaximo = max(stats.costoMaximo, costo);
                costoTotal += costo;
                stats.totalEnlaces++;
            }
        }
        
        stats.totalEnlaces /= 2; // Cada enlace se cuenta dos veces
        stats.costoPromedio = costoTotal / (stats.totalEnlaces * 2);
        
        return stats;
    }

    void imprimirEstadisticas() const {
        auto stats = obtenerEstadisticas();
        cout << "\n=== Estadísticas de la Red ===\n";
        cout << "Total de enrutadores: " << stats.totalEnrutadores << "\n";
        cout << "Total de enlaces: " << stats.totalEnlaces << "\n";
        cout << "Costo promedio: " << fixed << setprecision(2) << stats.costoPromedio << "\n";
        cout << "Costo mínimo: " << stats.costoMinimo << "\n";
        cout << "Costo máximo: " << stats.costoMaximo << "\n";
        cout << "Grado máximo: " << stats.gradoMaximo << "\n";
    }

    void imprimirHistorial() const {
        cout << "\n=== Historial de Cambios en la Red ===\n";
        for (const auto& cambio : historialCambios) {
            cout << cambio << "\n";
        }
    }

private:
    void registrarCambio(const string& cambio) {
        auto tiempo = chrono::system_clock::to_time_t(chrono::system_clock::now());
        historialCambios.push_back(string(ctime(&tiempo)) + ": " + cambio);
    }
};

// ... [código anterior se mantiene igual hasta ejecutarPruebas] ...

void ejecutarPruebas(Red& red) {
    cout << "\n=== Iniciando Pruebas de la Red ===\n";

    // Prueba 1: Generar red aleatoria
    try {
        cout << "\nPrueba 1: Generando red aleatoria...\n";
        red.generarRedAleatoria(5, 10, 0.7);
        cout << "Red generada exitosamente.\n";
        red.imprimirRed();
    } catch (const exception& e) {
        cout << "Error en Prueba 1: " << e.what() << "\n";
    }

    // Prueba 2: Encontrar rutas más cortas
    try {
        cout << "\nPrueba 2: Probando algoritmo de ruta más corta...\n";
        vector<pair<string, string>> paresAPrueba = {
            {"E0", "E4"},
            {"E1", "E3"},
            {"E2", "E4"}
        };

        for (const auto& [origen, destino] : paresAPrueba) {
            auto [costo, ruta] = red.encontrarRutaMasCorta(origen, destino);
            cout << "\nRuta más corta de " << origen << " a " << destino << ":\n";
            if (costo != -1) {
                cout << "Costo: " << costo << "\n";
                cout << "Ruta: ";
                for (size_t i = 0; i < ruta.size(); ++i) {
                    cout << ruta[i];
                    if (i < ruta.size() - 1) cout << " -> ";
                }
                cout << "\n";
            } else {
                cout << "No existe ruta entre estos enrutadores\n";
            }
        }
    } catch (const exception& e) {
        cout << "Error en Prueba 2: " << e.what() << "\n";
    }

    // Prueba 3: Modificación de enlaces
    try {
        cout << "\nPrueba 3: Modificando enlaces...\n";
        red.actualizarEnlace("E0", "E1", 15);
        red.actualizarEnlace("E1", "E2", 20);
        cout << "Enlaces modificados exitosamente.\n";
        red.imprimirRed();
    } catch (const exception& e) {
        cout << "Error en Prueba 3: " << e.what() << "\n";
    }

    // Prueba 4: Estadísticas de la red
    try {
        cout << "\nPrueba 4: Mostrando estadísticas de la red...\n";
        red.imprimirEstadisticas();
    } catch (const exception& e) {
        cout << "Error en Prueba 4: " << e.what() << "\n";
    }

    // Prueba 5: Historial de cambios
    try {
        cout << "\nPrueba 5: Mostrando historial de cambios...\n";
        red.imprimirHistorial();
    } catch (const exception& e) {
        cout << "Error en Prueba 5: " << e.what() << "\n";
    }
}

int main() {
    srand(time(nullptr));
    Red red;

    try {
        // Menú principal
        while (true) {
            cout << "\n=== Sistema de Gestión de Red ===\n";
            cout << "1. Cargar topología desde archivo\n";
            cout << "2. Generar red aleatoria\n";
            cout << "3. Agregar enrutador\n";
            cout << "4. Eliminar enrutador\n";
            cout << "5. Actualizar enlace\n";
            cout << "6. Encontrar ruta más corta\n";
            cout << "7. Mostrar estado de la red\n";
            cout << "8. Mostrar estadísticas\n";
            cout << "9. Mostrar historial\n";
            cout << "10. Ejecutar pruebas\n";
            cout << "0. Salir\n";
            cout << "Seleccione una opción: ";

            int opcion;
            cin >> opcion;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            switch (opcion) {
                case 1: {
                    cout << "Ingrese nombre del archivo: ";
                    string nombreArchivo;
                    getline(cin, nombreArchivo);
                    red.cargarTopologiaDesdeArchivo(nombreArchivo);
                    break;
                }
                case 2: {
                    cout << "Ingrese número de enrutadores: ";
                    int numEnrutadores;
                    cin >> numEnrutadores;
                    cout << "Ingrese costo máximo: ";
                    int costoMax;
                    cin >> costoMax;
                    cout << "Ingrese densidad (0-1): ";
                    double densidad;
                    cin >> densidad;
                    red.generarRedAleatoria(numEnrutadores, costoMax, densidad);
                    break;
                }
                case 3: {
                    cout << "Ingrese nombre del nuevo enrutador: ";
                    string nombre;
                    getline(cin, nombre);
                    red.agregarEnrutador(nombre);
                    break;
                }
                case 4: {
                    cout << "Ingrese nombre del enrutador a eliminar: ";
                    string nombre;
                    getline(cin, nombre);
                    red.eliminarEnrutador(nombre);
                    break;
                }
                case 5: {
                    cout << "Ingrese enrutador origen: ";
                    string origen;
                    getline(cin, origen);
                    cout << "Ingrese enrutador destino: ";
                    string destino;
                    getline(cin, destino);
                    cout << "Ingrese costo: ";
                    int costo;
                    cin >> costo;
                    red.actualizarEnlace(origen, destino, costo);
                    break;
                }
                case 6: {
                    cout << "Ingrese enrutador origen: ";
                    string origen;
                    getline(cin, origen);
                    cout << "Ingrese enrutador destino: ";
                    string destino;
                    getline(cin, destino);
                    auto [costo, ruta] = red.encontrarRutaMasCorta(origen, destino);
                    if (costo != -1) {
                        cout << "Costo total: " << costo << "\nRuta: ";
                        for (size_t i = 0; i < ruta.size(); ++i) {
                            cout << ruta[i];
                            if (i < ruta.size() - 1) cout << " -> ";
                        }
                        cout << "\n";
                    } else {
                        cout << "No existe ruta entre los enrutadores especificados\n";
                    }
                    break;
                }
                case 7:
                    red.imprimirRed();
                    break;
                case 8:
                    red.imprimirEstadisticas();
                    break;
                case 9:
                    red.imprimirHistorial();
                    break;
                case 10:
                    ejecutarPruebas(red);
                    break;
                case 0:
                    cout << "Saliendo del programa...\n";
                    return 0;
                default:
                    cout << "Opción inválida\n";
            }
        }
    } catch (const exception& e) {
        cerr << "Error fatal: " << e.what() << "\n";
        return 1;
    }
}
