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



inline std::ostream& operator<<(std::ostream& ofs, Index::Entry& e) {
  writeTo(ofs, e.inZone);
  writeTo(ofs, e.docIdOffset);
  writeTo(ofs, e.postingOffset);
  writeTo(ofs, e.postingsSizeOffset);
  return ofs;
}



inline std::istream& operator>>(std::istream& ifs, Index::Entry& e) {
  readFrom(ifs, &e.inZone);
  readFrom(ifs, &e.docIdOffset);
  readFrom(ifs, &e.postingOffset);
  readFrom(ifs, &e.postingsSizeOffset);
  return ifs;
}


//void writeEntries(std::ofstream& ofs, std::vector<Index::Entry>& entries) {
//  std::vector<uint8_t> byteBuf;
//  writeTo(ofs, static_cast<uint32_t>(entries.size()));
//  for (auto& entry: entries) {
//    bool2byte(entry.inZone, byteBuf);
//    writeTo(ofs, entry.docIdOffset);
//    writeTo(ofs, entry.postingOffset);
//    ofs << byteBuf;
//    byteBuf.clear();
//  }
//}
//
//
//
//void readEntries(std::ifstream& ifs, std::vector<Index::Entry>& entries, size_t tZonesCnt) {
//  uint32_t curSize, size;
//  curSize = entries.size();
//  readFrom(ifs, &size);
//  entries.resize(curSize + size);
//  std::vector<uint8_t> byteBuf;
//  for (size_t i = curSize; i < entries.size(); ++i) {
//    entries[i] = Index::Entry(0,0,std::vector<bool>());
//    readFrom(ifs, &entries[i].docIdOffset);
//    readFrom(ifs, &entries[i].postingOffset);
//    ifs >> byteBuf;
//    byte2bool(byteBuf, entries[i].inZone, tZonesCnt);
//    byteBuf.clear();
//  }
//}



inline uint32_t readFromEnd(std::ifstream& ifs) {
  uint32_t res;
  auto curPos = ifs.tellg();
  ifs.seekg(-4, ifs.end); // sizeof(uint32_t) == 4
  readFrom(ifs, &res);
  ifs.seekg(curPos, ifs.beg);
  return res;
}



#endif //
