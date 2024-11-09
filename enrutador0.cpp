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

using namespace std;

class Enrutador {
private:
    unordered_map<string, int> tablaEnrutamiento;
    string nombre;

public:
    explicit Enrutador(const string& nombreEnrutador = "") : nombre(nombreEnrutador) {}

    void establecerNombre(const string& nombreEnrutador) {
        if (nombreEnrutador.empty()) {
            throw invalid_argument("El nombre del enrutador no puede estar vacío");
        }
        nombre = nombreEnrutador;
    }

    const string& obtenerNombre() const { return nombre; }

    // Actualiza una ruta en la tabla de enrutamiento
    void actualizarRuta(const string& destino, int costo) {
        if (costo < 0) {
            throw invalid_argument("El costo no puede ser negativo");
        }
        tablaEnrutamiento[destino] = costo;
    }

    // Elimina una ruta de la tabla de enrutamiento
    void eliminarRuta(const string& destino) {
        auto it = tablaEnrutamiento.find(destino);
        if (it != tablaEnrutamiento.end()) {
            tablaEnrutamiento.erase(it);
        }
    }

    // Obtiene el costo para llegar a un destino
    int obtenerCosto(const string& destino) const {
        auto it = tablaEnrutamiento.find(destino);
        return (it != tablaEnrutamiento.end()) ? it->second : numeric_limits<int>::max();
    }

    const unordered_map<string, int>& obtenerTablaEnrutamiento() const {
        return tablaEnrutamiento;
    }
};

class Red {
private:
    unordered_map<string, Enrutador> enrutadores;

    bool existeEnrutador(const string& nombre) const {
        return enrutadores.find(nombre) != enrutadores.end();
    }

public:
    // Agrega un nuevo enrutador a la red
    void agregarEnrutador(const string& nombre) {
        if (nombre.empty()) {
            throw invalid_argument("El nombre del enrutador no puede estar vacío");
        }
        if (existeEnrutador(nombre)) {
            throw invalid_argument("Ya existe un enrutador con ese nombre");
        }
        enrutadores.emplace(nombre, Enrutador(nombre));
    }

    // Elimina un enrutador de la red
    void eliminarEnrutador(const string& nombre) {
        if (!existeEnrutador(nombre)) {
            throw invalid_argument("Enrutador no encontrado");
        }
        enrutadores.erase(nombre);
        // Eliminar rutas en otros enrutadores que apuntaban al eliminado
        for (auto& [_, enrutador] : enrutadores) {
            enrutador.eliminarRuta(nombre);
        }
    }

    // Actualiza un enlace entre dos enrutadores
    void actualizarEnlace(const string& origen, const string& destino, int costo) {
        if (!existeEnrutador(origen) || !existeEnrutador(destino)) {
            throw invalid_argument("Enrutador origen o destino no existe");
        }
        if (costo < 0) {
            throw invalid_argument("El costo no puede ser negativo");
        }
        enrutadores.at(origen).actualizarRuta(destino, costo);
        enrutadores.at(destino).actualizarRuta(origen, costo);
    }

    // Carga la topología desde un archivo
    void cargarTopologiaDesdeArchivo(const string& nombreArchivo) {
        ifstream archivo(nombreArchivo);
        if (!archivo) {
            throw runtime_error("No se pudo abrir el archivo: " + nombreArchivo);
        }

        string linea;
        int numLinea = 0;
        while (getline(archivo, linea)) {
            numLinea++;
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
            } catch (const exception& e) {
                throw runtime_error("Error en línea " + to_string(numLinea) + ": " + e.what());
            }
        }
    }

    // Encuentra la ruta más corta entre dos enrutadores
    pair<int, vector<string>> encontrarRutaMasCorta(const string& origen, const string& destino) const {
        if (!existeEnrutador(origen) || !existeEnrutador(destino)) {
            throw invalid_argument("Enrutador origen o destino no existe");
        }

        unordered_map<string, int> distancias;
        unordered_map<string, string> anterior;
        set<pair<int, string>> nodosActivos;

        // Inicialización de distancias
        for (const auto& [nombre, _] : enrutadores) {
            distancias[nombre] = numeric_limits<int>::max();
        }
        distancias[origen] = 0;
        nodosActivos.insert({0, origen});

        // Algoritmo de Dijkstra
        while (!nodosActivos.empty()) {
            auto [dist, actual] = *nodosActivos.begin();
            nodosActivos.erase(nodosActivos.begin());

            if (actual == destino) break;

            const auto& enrutadorActual = enrutadores.at(actual);
            for (const auto& [vecino, costo] : enrutadorActual.obtenerTablaEnrutamiento()) {
                int nuevaDist = dist + costo;
                if (nuevaDist < distancias[vecino]) {
                    nodosActivos.erase({distancias[vecino], vecino});
                    distancias[vecino] = nuevaDist;
                    anterior[vecino] = actual;
                    nodosActivos.insert({nuevaDist, vecino});
                }
            }
        }

        // Reconstruir el camino
        vector<string> ruta;
        if (distancias[destino] == numeric_limits<int>::max()) {
            return {-1, ruta}; // No hay ruta disponible
        }

        for (string actual = destino; actual != origen; actual = anterior[actual]) {
            ruta.push_back(actual);
        }
        ruta.push_back(origen);
        reverse(ruta.begin(), ruta.end());

        return {distancias[destino], ruta};
    }

    // Genera una red aleatoria para pruebas
    void generarRedAleatoria(int numEnrutadores, int costoMaximo) {
        if (numEnrutadores <= 0 || costoMaximo <= 0) {
            throw invalid_argument("El número de enrutadores y costo máximo deben ser positivos");
        }

        enrutadores.clear();

        // Crear enrutadores
        for (int i = 0; i < numEnrutadores; ++i) {
            agregarEnrutador("E" + to_string(i));
        }

        // Generar enlaces aleatorios
        vector<string> nombresEnrutadores;
        nombresEnrutadores.reserve(numEnrutadores);
        for (const auto& [nombre, _] : enrutadores) {
            nombresEnrutadores.push_back(nombre);
        }

        for (size_t i = 0; i < nombresEnrutadores.size(); ++i) {
            for (size_t j = i + 1; j < nombresEnrutadores.size(); ++j) {
                if (rand() % 2 == 0) {
                    int costo = rand() % costoMaximo + 1;
                    actualizarEnlace(nombresEnrutadores[i], nombresEnrutadores[j], costo);
                }
            }
        }
    }

    // Imprime la configuración actual de la red
    void imprimirRed() const {
        for (const auto& [nombreEnrutador, enrutador] : enrutadores) {
            cout << "Enrutador " << nombreEnrutador << " tiene enlaces:\n";
            for (const auto& [vecino, costo] : enrutador.obtenerTablaEnrutamiento()) {
                cout << "  -> " << vecino << " con costo " << costo << "\n";
            }
        }
    }

    // Verifica si la red está completamente conectada
    bool esRedConectada() const {
        if (enrutadores.empty()) return true;

        set<string> visitados;
        vector<string> pila;
        
        auto it = enrutadores.begin();
        pila.push_back(it->first);

        while (!pila.empty()) {
            string actual = pila.back();
            pila.pop_back();

            if (visitados.insert(actual).second) {
                const auto& enrutadorActual = enrutadores.at(actual);
                for (const auto& [vecino, _] : enrutadorActual.obtenerTablaEnrutamiento()) {
                    if (visitados.find(vecino) == visitados.end()) {
                        pila.push_back(vecino);
                    }
                }
            }
        }

        return visitados.size() == enrutadores.size();
    }
};

int main() {
    try {
        srand(static_cast<unsigned>(time(nullptr)));

        Red red;

        // Generar una red aleatoria de prueba
        red.generarRedAleatoria(5, 10);
        cout << "Red generada aleatoriamente:\n";
        red.imprimirRed();

        // Verificar conectividad
        if (red.esRedConectada()) {
            cout << "\nLa red está completamente conectada.\n";
        } else {
            cout << "\nAdvertencia: La red no está completamente conectada.\n";
        }

        // Encontrar y mostrar la ruta más corta entre dos enrutadores
        auto [costo, ruta] = red.encontrarRutaMasCorta("E0", "E3");
        
        if (costo != -1) {
            cout << "\nRuta más corta de E0 a E3:\n";
            cout << "Costo total: " << costo << "\nRuta: ";
            for (const auto& nodo : ruta) {
                cout << nodo << " ";
            }
            cout << "\n";
        } else {
            cout << "\nNo existe una ruta entre E0 y E3\n";
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
