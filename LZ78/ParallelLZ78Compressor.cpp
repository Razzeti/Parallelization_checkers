#include "ParallelLZ78Compressor.h"
#include <omp.h>
#include <cmath>
#include <vector>

bool ParallelLZ78Compressor::compress(const std::string& inputFilePath, const std::string& outputFilePath) {
    std::ifstream inputFile(inputFilePath, std::ios::binary | std::ios::ate);
    if (!inputFile.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de entrada." << std::endl;
        return false;
    }

    std::streamsize size = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);
    std::vector<char> file_data(size);
    if (!inputFile.read(file_data.data(), size)) {
        std::cerr << "Error al leer el archivo de entrada." << std::endl;
        return false;
    }
    inputFile.close();

    int num_threads = omp_get_max_threads();
    std::streamsize chunk_size = std::ceil(static_cast<double>(size) / num_threads);
    std::vector<std::vector<char>> compressed_chunks(num_threads);

    omp_set_num_threads(1); // FORZAR NUCLEOS

    #pragma omp parallel for
    for (int i = 0; i < num_threads; ++i) {
        auto start = file_data.begin() + i * chunk_size;
        auto end = (i == num_threads - 1) ? file_data.end() : start + chunk_size;
        std::vector<char> chunk(start, end);
        compressed_chunks[i] = compress_chunk(chunk);
    }

    std::ofstream outputFile(outputFilePath, std::ios::binary);
    if (!outputFile.is_open()) {
        std::cerr << "Error: No se pudo crear el archivo de salida." << std::endl;
        return false;
    }

    outputFile.write(reinterpret_cast<const char*>(&num_threads), sizeof(num_threads));
    for (int i = 0; i < num_threads; ++i) {
        size_t block_size = compressed_chunks[i].size();
        outputFile.write(reinterpret_cast<const char*>(&block_size), sizeof(block_size));
    }

    for (int i = 0; i < num_threads; ++i) {
        outputFile.write(compressed_chunks[i].data(), compressed_chunks[i].size());
    }

    outputFile.close();
    return true;
}

bool ParallelLZ78Compressor::decompress(const std::string& inputFilePath, const std::string& outputFilePath) {
    std::ifstream inputFile(inputFilePath, std::ios::binary);
    if (!inputFile.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo comprimido." << std::endl;
        return false;
    }

    int num_chunks;
    inputFile.read(reinterpret_cast<char*>(&num_chunks), sizeof(num_chunks));
    std::vector<size_t> chunk_sizes(num_chunks);
    inputFile.read(reinterpret_cast<char*>(chunk_sizes.data()), num_chunks * sizeof(size_t));

    std::vector<std::vector<char>> compressed_chunks(num_chunks);
    for(int i = 0; i < num_chunks; ++i) {
        compressed_chunks[i].resize(chunk_sizes[i]);
        inputFile.read(compressed_chunks[i].data(), chunk_sizes[i]);
    }
    inputFile.close();

    std::vector<std::string> decompressed_chunks(num_chunks);

    #pragma omp parallel for
    for (int i = 0; i < num_chunks; ++i) {
        decompressed_chunks[i] = decompress_chunk(compressed_chunks[i]);
    }

    std::ofstream outputFile(outputFilePath, std::ios::binary);
    if (!outputFile.is_open()) {
        std::cerr << "Error: No se pudo crear el archivo de salida." << std::endl;
        return false;
    }
    for (const auto& chunk : decompressed_chunks) {
        outputFile << chunk;
    }
    outputFile.close();

    return true;
}

std::vector<char> ParallelLZ78Compressor::compress_chunk(const std::vector<char>& chunk_data) {
    std::unordered_map<std::string, index_t> dictionary;
    std::string buffer;
    index_t dictIndex = 1;
    std::vector<char> compressed_output;

    for (char symbol : chunk_data) {
        std::string combined = buffer + symbol;
        if (dictionary.count(combined) == 0) {
            index_t index = buffer.empty() ? 0 : dictionary[buffer];
            
            // Escribir el índice (2 bytes) y el símbolo (1 byte) directamente al vector de bytes
            compressed_output.insert(compressed_output.end(), reinterpret_cast<char*>(&index), reinterpret_cast<char*>(&index) + sizeof(index_t));
            compressed_output.push_back(symbol);
            
            // Limitar el tamaño del diccionario para evitar uso excesivo de memoria y desbordamiento del índice
            if (dictIndex < 65535) { 
                 dictionary[combined] = dictIndex++;
            }
            buffer.clear();
        } else {
            buffer = combined;
        }
    }

    if (!buffer.empty()) {
        index_t index = dictionary.count(buffer) ? dictionary[buffer] : 0;
        char end_symbol = '\0'; // Usar caracter nulo como indicador de fin de bloque
        compressed_output.insert(compressed_output.end(), reinterpret_cast<char*>(&index), reinterpret_cast<char*>(&index) + sizeof(index_t));
        compressed_output.push_back(end_symbol);
    }
    return compressed_output;
}

std::string ParallelLZ78Compressor::decompress_chunk(const std::vector<char>& compressed_chunk) {
    std::vector<std::string> dictionary = {""};
    std::string result;
    
    const char* data_ptr = compressed_chunk.data();
    const char* end_ptr = data_ptr + compressed_chunk.size();

    while (data_ptr < end_ptr) {
        index_t index;
        // Leer el índice de 2 bytes
        std::copy(data_ptr, data_ptr + sizeof(index_t), reinterpret_cast<char*>(&index));
        data_ptr += sizeof(index_t);

        // Leer el caracter de 1 byte
        char symbol = *data_ptr;
        data_ptr++;

        if (index >= dictionary.size()) {
            continue;
        }
        std::string entry = dictionary[index];
        if (symbol != '\0') {
            entry += symbol;
        }
        
        result += entry;
        
        if (dictionary.size() < 65535) {
             dictionary.push_back(entry);
        }
    }
    return result;
}