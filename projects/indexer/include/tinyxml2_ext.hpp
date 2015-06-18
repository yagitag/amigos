#ifndef __TINYXML2_EXT_H__
#define __TINYXML2_EXT_H__

#include "../../include/indexer/index_struct.h"
#include "../../../contrib/tinyxml2/tinyxml2.h"

inline const tinyxml2::XMLElement* getNecessaryTag(const tinyxml2::XMLNode* tiElem, const std::string& tag) {
  auto result = tiElem->FirstChildElement(tag.c_str());
  if (!result) {
    throw Index::Exception("There is no tag '" + tag + "'");
  }
  return result;
}


#endif // __TINYXML2_EXT_H__
