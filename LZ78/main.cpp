// main.cpp - Versión para procesar por lotes con opción de ruta
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <filesystem> // Librería para manejar el sistema de archivos (C++17)
#include "ParallelLZ78Compressor.h"

// Espacio de nombres para el sistema de archivos
namespace fs = std::filesystem;

// Función para obtener el tamaño de un archivo
long getFileSize(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return -1;
    }
    return file.tellg();
}

// Función para procesar un único archivo y mostrar sus resultados
void processFile(const std::string& inputFilePath) {
    std::cout << "======================================================\n";
    std::cout << "Procesando archivo: " << inputFilePath << "\n";
    std::cout << "======================================================\n";

    std::string compressedFileName = "comprimido_" + fs::path(inputFilePath).stem().string() + ".lz78";
    std::string decompressedFileName = "descomprimido_" + fs::path(inputFilePath).filename().string();

    ParallelLZ78Compressor parallelCompressor;
    
    // --- Compresión Paralela ---
    auto start_compress = std::chrono::high_resolution_clock::now();
    bool compress_success = parallelCompressor.compress(inputFilePath, compressedFileName);
    auto end_compress = std::chrono::high_resolution_clock::now();

    if (!compress_success) {
        std::cerr << "Fallo en la compresión para el archivo: " << inputFilePath << std::endl;
        return;
    }
    std::chrono::duration<double, std::milli> compress_time_ms = end_compress - start_compress;
    
    // --- Descompresión Paralela ---
    auto start_decompress = std::chrono::high_resolution_clock::now();
    bool decompress_success = parallelCompressor.decompress(compressedFileName, decompressedFileName);
    auto end_decompress = std::chrono::high_resolution_clock::now();

    if (!decompress_success) {
        std::cerr << "Fallo en la descompresión para el archivo: " << compressedFileName << std::endl;
        return;
    }
    std::chrono::duration<double, std::milli> decompress_time_ms = end_decompress - start_decompress;

    // --- Reporte de Resultados ---
    long originalSize = getFileSize(inputFilePath);
    long compressedSize = getFileSize(compressedFileName);

    if (originalSize > 0 && compressedSize > 0) {
        double ratio = static_cast<double>(originalSize - compressedSize) / originalSize * 100.0;
        
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "-> Archivo Original: " << inputFilePath << " (" << originalSize << " bytes)" << std::endl;
        std::cout << "-> Archivo Comprimido: " << compressedFileName << " (" << compressedSize << " bytes)" << std::endl;
        std::cout << "----------------------------------" << std::endl;
        std::cout << "-> Tiempo de Compresión Paralela: " << compress_time_ms.count() << " ms" << std::endl;
        std::cout << "-> Tiempo de Descompresión Paralela: " << decompress_time_ms.count() << " ms" << std::endl;
        std::cout << "-> Ratio de Compresión: " << ratio << "%" << std::endl;
        std::cout << "======================================================\n\n";

    } else {
        std::cerr << "Error al obtener los tamaños de los archivos para el reporte." << std::endl;
    }
}


int main() {
    std::string directoryPath;
    char choice;

    std::cout << "Seleccione una opción para la ruta de los archivos:\n";
    std::cout << "1. Usar ruta predefinida (LZ78/textos/)\n";
    std::cout << "2. Especificar una ruta manualmente\n";
    std::cout << "Su elección (1 o 2): ";
    std::cin >> choice;

    if (choice == '1') {
        directoryPath = "textos/";
        std::cout << "Usando la ruta predefinida: " << directoryPath << std::endl;
    } else if (choice == '2') {
        std::cout << "Ingrese la ruta a la carpeta que contiene los archivos .txt: ";
        std::cin >> directoryPath;
    } else {
        std::cerr << "Opción inválida. Saliendo del programa." << std::endl;
        return 1;
    }
    
    std::cout << "\nIniciando análisis...\n\n";

    try {
        // Itera sobre todos los archivos en el directorio proporcionado
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            // Verifica si es un archivo regular y si tiene la extensión .txt
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                processFile(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error al acceder al directorio: " << e.what() << std::endl;
        std::cerr << "Asegúrese de que la ruta '" << directoryPath << "' es correcta y existe." << std::endl;
        return 1;
    }

    std::cout << "Análisis de todos los archivos .txt completado." << std::endl;

    return 0;
}