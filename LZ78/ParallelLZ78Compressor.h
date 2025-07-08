/* * ParallelLZ78Compressor.h (Optimizado)
 * Se elimina el struct LZ78Token y se trabaja con un formato binario más eficiente.
 */
#ifndef PARALLEL_LZ78_COMPRESSOR_H
#define PARALLEL_LZ78_COMPRESSOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iostream>

// Usamos un tipo de dato más pequeño para el índice, ya que el diccionario está limitado.
// Un short (2 bytes) puede almacenar hasta 65535 índices, más que suficiente.
using index_t = unsigned short; 

class ParallelLZ78Compressor {
public:
    bool compress(const std::string& inputFilePath, const std::string& outputFilePath);
    bool decompress(const std::string& inputFilePath, const std::string& outputFilePath);

private:
    // La función ahora devuelve un vector de bytes (char) que es la representación binaria comprimida.
    std::vector<char> compress_chunk(const std::vector<char>& chunk_data);
    
    // La función ahora toma un vector de bytes como entrada.
    std::string decompress_chunk(const std::vector<char>& compressed_chunk);
};

#endif // PARALLEL_LZ78_COMPRESSOR_H