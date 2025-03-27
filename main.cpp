#include <algorithm>
#include <bitset>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <vector>

struct MinHeapNode {
  uint8_t data;
  int freq;
  MinHeapNode *left, *right;

  MinHeapNode(uint8_t data, int freq) : data(data), freq(freq), left(nullptr), right(nullptr) {}
};

struct CompareMinHeapNode {
  bool operator()(const MinHeapNode *l, const MinHeapNode *r) const { return l->freq > r->freq; }
};

std::map<uint8_t, std::string> codes;
std::vector<std::pair<uint8_t, std::string>> decompCodes;

std::vector<uint8_t> readFromFile(const std::string &fileName) {
  std::ifstream file(fileName, std::ios::binary | std::ios::ate); // Open at end to get size
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + fileName);
  }
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> data(size);
  file.read(reinterpret_cast<char *>(data.data()), size); // Read directly into vector

  return data;
}

void storeCodes(const MinHeapNode *root, std::string str) {
  if (!root)
    return;
  if (root->data != 0)
    codes[root->data] = str;
  storeCodes(root->left, str + "0");
  storeCodes(root->right, str + "1");
}

void buildHuffmanTree(std::map<uint8_t, int> &freq) {
  auto cmp = [](const MinHeapNode *l, const MinHeapNode *r) { return l->freq > r->freq; };
  std::priority_queue<MinHeapNode *, std::vector<MinHeapNode *>, decltype(cmp)> minHeap(cmp);

  for (auto &[byte, frequency]: freq) {
    minHeap.push(new MinHeapNode(byte, frequency));
  }

  while (minHeap.size() > 1) {
    MinHeapNode *left = minHeap.top();
    minHeap.pop();
    MinHeapNode *right = minHeap.top();
    minHeap.pop();
    MinHeapNode *top = new MinHeapNode(0, left->freq + right->freq);
    top->left = left;
    top->right = right;
    minHeap.push(top);
  }

  storeCodes(minHeap.top(), "");
}

std::string encode(const std::vector<uint8_t> &data) {
  std::string encodedString;
  for (uint8_t byte: data) {
    encodedString += codes[byte];
  }
  return encodedString;
}

void writeBits(std::ofstream &output, const std::string &bits) {
  uint8_t currentByte = 0;
  int bitCount = 0;
  for (char bit: bits) {
    currentByte = (currentByte << 1) | (bit == '1');
    bitCount++;
    if (bitCount == 8) {
      output.put(currentByte);
      currentByte = 0;
      bitCount = 0;
    }
  }
  if (bitCount > 0) {
    currentByte <<= (8 - bitCount);
    output.put(currentByte);
  }
}

void writeHeader(std::ofstream &output, const std::vector<std::pair<std::string, uint8_t>> &invertedMap) {
  std::map<size_t, size_t> lengthCounts;
  for (const auto &[code, byte]: invertedMap) {
    lengthCounts[code.length()]++;
  }

  output.put(static_cast<uint8_t>(invertedMap.size()));

  for (const auto &[length, count]: lengthCounts) {
    output.put(static_cast<uint8_t>(count)); // Number of codes of this length
    output.put(static_cast<uint8_t>(length)); // The length itself
    for (const auto &[code, byte]: invertedMap) {
      if (code.length() == length) {
        output.put(byte);
        writeBits(output, code);
      }
    }
  }
}

std::vector<uint8_t> readCompressedFile(const std::string &filePath) {
  std::ifstream input(filePath, std::ios::binary | std::ios::ate);
  if (!input.is_open())
    throw std::runtime_error("Could not open file: " + filePath);

  std::streamsize fileSize = input.tellg();
  input.seekg(0, std::ios::beg);

  std::vector<uint8_t> compressedData(fileSize);
  input.read(reinterpret_cast<char *>(compressedData.data()), fileSize);
  return compressedData;
}

std::string uint8_to_binary_string(const uint8_t value) {
  std::string binary_string;
  for (int i = 7; i >= 0; --i) {
    // Iterate from the most significant bit to the least significant
    binary_string += ((value >> i) & 1) ? '1' : '0';
  }
  return binary_string;
}

void reconstructCodeMap(const std::vector<uint8_t> &compressedData, std::map<std::string, uint8_t> &codeMap) {
  size_t index = 0;
  while (index < compressedData.size()) {
    uint8_t count = compressedData[index++];
    uint8_t length = compressedData[index++];

    for (int i = 0; i < count; ++i) {
      if (index >= compressedData.size())
        throw std::runtime_error("Invalid compressed data format.");
      uint8_t byte = compressedData[index++];

      if (index >= compressedData.size())
        throw std::runtime_error("Invalid compressed data format.");
      uint8_t codeByte = compressedData[index++];

      std::string code;
      for (int j = 0; j < length; ++j) {
        code += (codeByte & (1 << (7 - j))) ? '1' : '0';
      }
      code = code.substr(0, length); // Truncate to the specified length
      codeMap[code] = byte;
    }
  }
}

std::vector<uint8_t> decompressData(const std::vector<uint8_t> &compressedData,
                                    const std::map<std::string, uint8_t> &codeMap) {
  std::string bitString;
  for (uint8_t byte: compressedData) {
    bitString += uint8_to_binary_string(byte);
  }

  std::vector<uint8_t> decodedData;
  std::string currentCode;
  for (char bit: bitString) {
    currentCode += bit;
    if (codeMap.find(currentCode) != codeMap.end()) {
      decodedData.push_back(codeMap.at(currentCode));
      currentCode.clear();
    }
  }

  return decodedData;
}

bool compareFiles(const std::string &file1, const std::string &file2) {
  std::ifstream f1(file1, std::ios::binary);
  std::ifstream f2(file2, std::ios::binary);

  if (!f1.is_open() || !f2.is_open()) {
    throw std::runtime_error("Could not open one of the files for comparison.");
  }

  std::vector<uint8_t> data1((std::istreambuf_iterator<char>(f1)), std::istreambuf_iterator<char>());
  std::vector<uint8_t> data2((std::istreambuf_iterator<char>(f2)), std::istreambuf_iterator<char>());

  if (data1.size() != data2.size()) {
    return false;
  }

  for (size_t i = 0; i < data1.size(); ++i) {
    if (data1[i] != data2[i]) {
      return false;
    }
  }

  return true;
}

std::vector<std::pair<uint8_t, std::string>> decompressHeader(const std::vector<unsigned char>& compressed) {
  std::vector<std::pair<uint8_t, std::string>> decompCodes;
  int mapLength = compressed[0];
  int i = 1;

  while (decompCodes.size() < mapLength) {
    int numCodes = compressed[i];
    int codeLength = compressed[i + 1];

    for (int j = 0; j < numCodes; ++j) {
      int codeIndex = i + 2 + j * 2;
      int code = compressed[codeIndex];
      unsigned char symbolByte = compressed[codeIndex + 1];

      std::bitset<8> symbolBits(symbolByte);
      std::string symbolString = symbolBits.to_string();
      std::string symbol = symbolString.substr(0, codeLength);

      decompCodes.emplace_back(codeLength, symbol);
    }

    i += 2 + numCodes * 2;
  }

  return decompCodes;
}

int main() {
  try {
    std::string inputFileName = "../public/input.bin";
    std::string outputFileName = "../public/input.bin.comp";
    std::string decompressedFileName = "../public/output.bin";

    std::vector<uint8_t> data = readFromFile(inputFileName);

    std::map<uint8_t, int> freq;
    for (uint8_t byte: data) {
      freq[byte]++;
    }

    buildHuffmanTree(freq);

    std::vector<std::pair<std::string, uint8_t>> invertedMap;
    for (const auto &[byte, code]: codes) {
      invertedMap.emplace_back(code, byte);
    }
    std::ranges::sort(invertedMap, [](const auto &a, const auto &b) { return a.first.length() < b.first.length(); });

    std::ofstream output(outputFileName, std::ios::binary);
    if (!output.is_open()) {
      throw std::runtime_error("Could not open output file: " + outputFileName);
    }

    writeHeader(output, invertedMap);

    std::string encodedData = encode(data);
    std::cout << "encodedData: " << encodedData << std::endl;

    writeBits(output, encodedData);

    output.close();

    std::cout << "Compression complete.\n" << std::endl;


    std::cout << "Reading compressed: " << std::endl;
    std::vector<uint8_t> compressed = readCompressedFile(outputFileName);
    for (const auto &byte: compressed)
      std::cout << static_cast<int>(byte) << " ";
    std::cout << std::endl;

    std::map<std::string, uint8_t> codeMap;
    int mapLength = compressed[0];
    int i = 1;

    while (decompCodes.size() < mapLength) {
      int numCodes = compressed[i];
      int codeLength = compressed[i + 1];

      for (int j = 0; j < numCodes; ++j) {
        int codeIndex = i + 2 + j * 2;
        int code = compressed[codeIndex];
        uint8_t symbolByte = compressed[codeIndex + 1];

        // Convert the symbol byte to a bit string
        std::bitset<8> symbolBits(symbolByte);
        std::string symbolString = symbolBits.to_string();

        // Extract the required number of bits
        std::string symbol = symbolString.substr(0, codeLength);

        decompCodes.emplace_back(code, symbol);
      }

      i += 2 + numCodes * 2;
    }

    std::cout << "decompCodes" << std::endl;
    for (const auto &[fst, snd]: decompCodes) {
      std::cout << static_cast<int>(fst) << ":" << snd << std::endl;
    }
    /*reconstructCodeMap(compressed, codeMap);

    std::vector<uint8_t> decodedData = decompressData(compressed, codeMap);

    std::cout << "Matched codes: " << std::endl;
    for (uint8_t code: decodedData) {
      std::cout << static_cast<int>(code) << std::endl;
    }

    std::ofstream outFile(decompressedFileName, std::ios::binary | std::ios::trunc);
    if (outFile.is_open()) {
      for (uint8_t byte: decodedData) {
        outFile.write(reinterpret_cast<char *>(&byte), sizeof(uint8_t));
      }
      outFile.close();
      std::cout << "Output file successfully saved." << std::endl;
    } else {
      throw std::runtime_error("Could not open output file.");
    }

    // Compare the original and decompressed files
    if (compareFiles(inputFileName, decompressedFileName)) {
      std::cout << "Success: The original and decompressed files match!" << std::endl;
    } else {
      std::cout << "Error: The original and decompressed files do not match!" << std::endl;
    }*/
  } catch (const std::runtime_error &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
