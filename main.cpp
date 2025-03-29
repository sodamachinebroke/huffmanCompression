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

auto b(uint8_t v) {
  std::string s(8, '0');
  for (int i = 0; i < 8; ++i)
    s[7 - i] = (v >> i & 1) + '0';
  return s;
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
                                    const std::vector<std::pair<uint8_t, std::string>> &codePairs, size_t dataOffset,
                                    int paddingBits) {
  std::string bitString;
  // Start from dataOffset to skip the header
  for (size_t i = dataOffset; i < compressedData.size(); ++i) {
    uint8_t byte = compressedData[i];
    bitString += b(byte);
  }

  // Remove padding bits if any
  if (paddingBits > 0) {
    bitString = bitString.substr(0, bitString.size() - paddingBits);
  }

  std::vector<uint8_t> decodedData;
  std::string currentCode;
  for (char bit: bitString) {
    currentCode += bit;
    // Search through the code pairs for a matching code
    for (const auto &pair: codePairs) {
      if (pair.second == currentCode) {
        decodedData.push_back(pair.first);
        currentCode.clear();
        break;
      }
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

struct DecompressionInfo {
  std::vector<std::pair<uint8_t, std::string>> codePairs;
  size_t dataOffset;
  int paddingBits;
};

DecompressionInfo decompressHeader(const std::vector<unsigned char> &compressed) {
  std::vector<std::pair<uint8_t, std::string>> decompCodes;
  int padding = compressed[0];
  int mapLength = compressed[1];
  int i = 2;

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

      decompCodes.emplace_back(code, symbol);
    }

    i += 2 + numCodes * 2;
  }

  return {decompCodes, static_cast<size_t>(i), padding};
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

    std::string encodedData = encode(data);
    std::cout << "encodedData: " << encodedData << std::endl;
    std::cout << "encodedData mod 8:" << encodedData.size() % 8 << std::endl;

    output.put(encodedData.size() % 8);
    writeHeader(output, invertedMap);
    writeBits(output, encodedData);


    output.close();
    std::cout << "Compression complete.\n" << std::endl;


    std::cout << "Reading compressed: " << std::endl;
    std::vector<uint8_t> compressed = readCompressedFile(outputFileName);
    for (const auto &byte: compressed)
      std::cout << static_cast<int>(byte) << " ";
    std::cout << std::endl;

    DecompressionInfo info = decompressHeader(compressed);

    std::cout << "decompressed" << std::endl;
    for (const auto &[fst, snd]: info.codePairs) {
      std::cout << static_cast<int>(fst) << ":" << snd << std::endl;
    }

    std::vector<uint8_t> decompData = decompressData(compressed, info.codePairs, info.dataOffset, info.paddingBits);

    for (const auto byte : decompData) {
      std::cout<< static_cast<int>(byte) << " ";
    }

    // Write decompressed data to file
    std::ofstream outFile(decompressedFileName, std::ios::binary);
    outFile.write(reinterpret_cast<const char*>(decompData.data()), decompData.size());
    outFile.close();

  } catch (const std::runtime_error &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
