#ifndef __INDEX_IO_H__
#define __INDEX_IO_H__

#include <fstream>
#include <vector>

#include "../../../include/indexer/index_struct.h"



//void bool2byte(const std::vector<bool>& boolVec, std::vector<uint8_t>& res) {
//  const size_t byteSize = 8;
//  uint8_t byteVal = 0;
//  size_t i = 0;
//  while (i < boolVec.size()) {
//    byteVal = (byteVal << 1) | boolVec[i++];
//    if (i % byteSize == 0) {
//      res.push_back(byteVal);
//      byteVal = 0;
//    }
//  }
//  if (i % byteSize != 0) {
//    res.push_back(byteVal << (byteSize - i%byteSize));
//  }
//}
//
//
//void byte2bool(const std::vector<uint8_t>& byteVec, std::vector<bool>& res, size_t size = 0) {
//  const int byteSize = 8;
//  if (!size) {
//    size = byteVec.size() * byteSize;
//  }
//  res.resize(size);
//  uint8_t mask = 0x80;
//  for (size_t i = 0; i < size; ++i) {
//    res[i] = byteVec[i/byteSize] & mask;
//    mask >>= 1;
//    if (i % byteSize == byteSize - 1) {
//      mask = 0x80;
//    }
//  }
//}



template <typename T>
inline void writeTo(std::ostream &ofs, T val) {
  ofs.write(reinterpret_cast<char*>(&val), sizeof(T));
}



template <typename T>
inline void readFrom(std::istream &ifs, T* val) {
  ifs.read(reinterpret_cast<char*>(val), sizeof(T));
}



template <typename T>
inline std::istream& operator>>(std::istream& ifs, std::vector<T>& vec) {
  uint32_t curSize, size;
  curSize = vec.size();
  readFrom(ifs, &size);
  vec.resize(curSize + size);
  ifs.read(reinterpret_cast<char*>(&vec[curSize]), size*sizeof(T));
  return ifs;
}



template <typename T>
inline std::ostream& operator<<(std::ostream& ofs, std::vector<T>& vec) {
  writeTo(ofs, static_cast<uint32_t>(vec.size()));
  ofs.write(reinterpret_cast<char*>(&vec[0]), vec.size()*sizeof(T));
  return ofs;
}



//inline std::ostream& operator<<(std::ostream& ofs, Index::Entry& e) {
//  ofs << e.zoneTf;
//  writeTo(ofs, e.docIdOffset);
//  writeTo(ofs, e.postingOffset);
//  return ofs;
//}
//
//
//
//inline std::istream& operator>>(std::istream& ifs, Index::Entry& e) {
//  ifs >> e.zoneTf;
//  readFrom(ifs, &e.docIdOffset);
//  readFrom(ifs, &e.postingOffset);
//  return ifs;
//}



inline void writePostingInfo(std::ofstream& ofs, Index::PostingInfo& postingInfo) {
  writeTo(ofs, static_cast<uint8_t>(postingInfo.postingSizes.size()));
  ofs.write(reinterpret_cast<char*>(&postingInfo.postingSizes[0]), postingInfo.postingSizes.size()*sizeof(uint16_t));
  writeTo(ofs, postingInfo.inZone);
  writeTo(ofs, postingInfo.postingOffset);
}



inline void readPostingInfo(std::ifstream& ifs, Index::PostingInfo& postingInfo) {
  uint8_t size;
  readFrom(ifs, &size);
  postingInfo.postingSizes.resize(size);
  ifs.read(reinterpret_cast<char*>(&postingInfo.postingSizes[0]), size*sizeof(uint16_t));
  readFrom(ifs, &postingInfo.inZone);
  readFrom(ifs, &postingInfo.postingOffset);
}



inline void writeEntries(std::ofstream& ofs, std::vector<Index::Entry>& entries) {
  writeTo(ofs, static_cast<uint32_t>(entries.size()));
  for (auto& entry: entries) {
    writePostingInfo(ofs, entry.postingInfo);
    writeTo(ofs, entry.docIdOffset);
  }
}



inline void readEntries(std::ifstream& ifs, std::vector<Index::Entry>& entries) {
  uint32_t curSize, size;
  curSize = entries.size();
  readFrom(ifs, &size);
  entries.resize(curSize + size);
  for (size_t i = curSize; i < entries.size(); ++i) {
    readPostingInfo(ifs, entries[i].postingInfo);
    readFrom(ifs, &entries[i].docIdOffset);
  }
}



inline uint32_t readFromEnd(std::ifstream& ifs) {
  uint32_t res;
  auto curPos = ifs.tellg();
  ifs.seekg(-4, ifs.end); // sizeof(uint32_t) == 4
  readFrom(ifs, &res);
  ifs.seekg(curPos, ifs.beg);
  return res;
}



#endif //
