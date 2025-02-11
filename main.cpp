#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <bitset>
#include <algorithm>

struct MinHeapNode
{
    uint8_t data;
    int freq;
    MinHeapNode *left, *right;

    MinHeapNode(uint8_t data, int freq) : data(data), freq(freq), left(nullptr), right(nullptr)
    {
    }
};

struct CompareMinHeapNode
{
    bool operator()(const MinHeapNode* l, const MinHeapNode* r) const
    {
        return l->freq > r->freq;
    }
};

std::map<uint8_t, std::string> codes;
std::vector<std::pair<uint8_t, std::string>> decompCodes;

std::vector<uint8_t> readFromFile(const std::string& fileName)
{
    std::ifstream file(fileName, std::ios::binary | std::ios::ate); // Open at end to get size
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open file: " + fileName);
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size); // Read directly into vector

    return data;
}

void storeCodes(const MinHeapNode* root, std::string str)
{
    if (!root) return;
    if (root->data != 0) codes[root->data] = str;
    storeCodes(root->left, str + "0");
    storeCodes(root->right, str + "1");
}

void buildHuffmanTree(std::map<uint8_t, int>& freq)
{
    auto cmp = [](const MinHeapNode* l, const MinHeapNode* r) { return l->freq > r->freq; };
    std::priority_queue<MinHeapNode*, std::vector<MinHeapNode*>, decltype(cmp)> minHeap(cmp);

    for (auto& [byte, frequency] : freq)
    {
        minHeap.push(new MinHeapNode(byte, frequency));
    }

    while (minHeap.size() > 1)
    {
        MinHeapNode* left = minHeap.top();
        minHeap.pop();
        MinHeapNode* right = minHeap.top();
        minHeap.pop();
        MinHeapNode* top = new MinHeapNode(0, left->freq + right->freq);
        top->left = left;
        top->right = right;
        minHeap.push(top);
    }

    storeCodes(minHeap.top(), "");
}


std::string encode(const std::vector<uint8_t>& data)
{
    std::string encodedString;
    for (uint8_t byte : data)
    {
        encodedString += codes[byte];
    }
    return encodedString;
}

std::vector<uint8_t> decode(const std::string& encodedString, const MinHeapNode* root)
{
    std::vector<uint8_t> decodedData;
    const MinHeapNode* current = root;
    for (char bit : encodedString)
    {
        current = (bit == '0') ? current->left : current->right;
        if (!current->left && !current->right)
        {
            decodedData.push_back(current->data);
            current = root;
        }
    }
    return decodedData;
}

void writeBits(std::ofstream& output, const std::string& bits)
{
    uint8_t currentByte = 0;
    int bitCount = 0;
    for (char bit : bits)
    {
        currentByte = (currentByte << 1) | (bit == '1');
        bitCount++;
        if (bitCount == 8)
        {
            output.put(currentByte);
            currentByte = 0;
            bitCount = 0;
        }
    }
    if (bitCount > 0)
    {
        currentByte <<= (8 - bitCount);
        output.put(currentByte);
    }
}

void writeHeader(std::ofstream& output, const std::vector<std::pair<std::string, uint8_t>>& invertedMap)
{
    std::map<size_t, size_t> lengthCounts;
    for (const auto& [code, byte] : invertedMap)
    {
        lengthCounts[code.length()]++;
    }

    for (const auto& [length, count] : lengthCounts)
    {
        output.put(static_cast<uint8_t>(count)); // Number of codes of this length
        output.put(static_cast<uint8_t>(length)); // The length itself
        for (const auto& [code, byte] : invertedMap)
        {
            if (code.length() == length)
            {
                output.put(byte);
                writeBits(output, code);
            }
        }
    }
}


std::vector<uint8_t> readCompressedFile(const std::string& filePath)
{
    std::ifstream input(filePath, std::ios::binary | std::ios::ate);
    if (!input.is_open())throw std::runtime_error("Could not open file: " + filePath);

    std::streamsize fileSize = input.tellg();
    input.seekg(0, std::ios::beg);

    std::vector<uint8_t> compressedData(fileSize);
    input.read(reinterpret_cast<char*>(compressedData.data()), fileSize);
    return compressedData;
}

std::string uint8_to_binary_string(const uint8_t value)
{
    std::string binary_string;
    for (int i = 7; i >= 0; --i)
    {
        // Iterate from the most significant bit to the least significant
        binary_string += ((value >> i) & 1) ? '1' : '0';
    }
    return binary_string;
}

int main()
{
    try
    {
        std::string inputFileName = "../public/input0.bin";
        std::string outputFileName = "../public/output/input0.bin.comp";

        std::vector<uint8_t> data = readFromFile(inputFileName);

        std::map<uint8_t, int> freq;
        for (uint8_t byte : data)
        {
            freq[byte]++;
        }

        buildHuffmanTree(freq);

        std::vector<std::pair<std::string, uint8_t>> invertedMap;
        for (const auto& [byte, code] : codes)
        {
            invertedMap.emplace_back(code, byte);
        }
        std::ranges::sort(invertedMap, [](const auto& a, const auto& b)
        {
            return a.first.length() < b.first.length();
        });

        std::ofstream output(outputFileName, std::ios::binary);
        if (!output.is_open())
        {
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
        for (const auto& byte : compressed) std::cout << static_cast<int>(byte) << " ";
        std::cout << std::endl;

        std::cout << std::endl;
        auto contentBegin = 0;
        for (int i = 2; i <= static_cast<int>(compressed[0]) * 2; i += 2)
        {
            //std::cout << static_cast<int>(compressed[i]) << " : " << static_cast<int>(compressed[i + 1]) << std::endl;
            //std::cout << uint8_to_binary_string(compressed[i + 1]).substr(0, compressed[1]) << std::endl;
            decompCodes.emplace_back(compressed[i], uint8_to_binary_string(compressed[i + 1]).substr(0, compressed[1]));
            contentBegin = i;
            std::cout << contentBegin << std::endl;
        }

        auto content = "";
        for (auto i = contentBegin + 2; i < compressed.size(); ++i)
        {
            content = uint8_to_binary_string(compressed[i]).c_str();
            std::cout << uint8_to_binary_string(compressed[i]) << " ";
        }

        std::string temp = content;
        std::vector<uint8_t> decodedData;

        while (!temp.empty())
        {
            bool matchFound = false;
            for (const auto& pair : decompCodes)
            {
                uint8_t code = pair.first;
                std::string pattern = pair.second;

                size_t pos = temp.find(pattern);
                if (pos == 0)
                {
                    decodedData.push_back(code);
                    temp.erase(0, pattern.length());
                    matchFound = true;
                    break;
                }
            }
            if (!matchFound) temp.erase(0, 1);
        }

        std::cout << "Matched codes: " << std::endl;
        for (uint8_t code : decodedData)
        {
            std::cout << static_cast<int>(code) << std::endl;
        }

        std::ofstream outFile("../public/output0.bin", std::ios::binary | std::ios::trunc);
        if (outFile.is_open())
        {
            for (uint8_t byte : decodedData)
            {
                outFile.write(reinterpret_cast<char*>(&byte), sizeof(uint8_t));
            }
            outFile.close();
            std::cout << "Output file successfully saved." << std::endl;
        }
        else
        {
            throw std::runtime_error("Could not open output file.");
        }
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
